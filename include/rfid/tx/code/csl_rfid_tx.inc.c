
#include "translib.h"
#include "compat_error.h"


#include "rfid_structs.h"


#define CALCULATE_PADDING(l, a)	(((a)-((l)%(a)))%(a))

#define CALCULATE_32BIT_PADDING(l)	CALCULATE_PADDING(l, 4)

#define RFID_PACKET_PKT_BYTE_LEN(pktlen)	((INT32U) ((pktlen) * 4))


TransStatus GetTransportCharacteristics (TransHandle transHandle, RFID_VERSION* pDriverVersion)
{
	TransStatus        status;
	INT32U             bufferSize = 0;
	TransportCharacteristics transportCharacteristics;

	status = RfTrans_GetRadioTransportCharacteristics(transHandle, NULL, &bufferSize);
	if ( status != CPL_ERROR_BUFFERTOOSMALL )
		return status;

	if ( sizeof(TransportCharacteristics) != bufferSize )
		return CPL_ERROR_INVALID;

	status = RfTrans_GetRadioTransportCharacteristics(transHandle, &transportCharacteristics, &bufferSize);
	if ( status != CPL_SUCCESS )
		return status;

	pDriverVersion->major = transportCharacteristics.transportVersion.major;
	pDriverVersion->minor = transportCharacteristics.transportVersion.minor;
	pDriverVersion->maintenance = transportCharacteristics.transportVersion.maintenance;
	pDriverVersion->release = transportCharacteristics.transportVersion.release;

	printf("maxTransfer = %d, maxPacket = %d\n", transportCharacteristics.maxTransfer, transportCharacteristics.maxPacket );

	return CPL_SUCCESS;
}

TransStatus EnumerateAttachedRadios( RFID_RADIO_ENUM *pEnum )
{
	TransStatus        status;
	EnumAction         action = EA_FirstRadio;

	TransHandle        transportHandle;
	INT8U serialNumber[12];
	INT32U             serialNumberLength;
	RadioEnumContext context;

	INT8U* pBuffer = (INT8U *)(pEnum + 1) + (100 * sizeof(RFID_RADIO_INFO *));

	pEnum->length      = sizeof(RFID_RADIO_ENUM);
	pEnum->totalLength = sizeof(RFID_RADIO_ENUM) + 100 * sizeof(RFID_RADIO_INFO *);
	pEnum->countRadios = 0;
	pEnum->ppRadioInfo = (RFID_RADIO_INFO **)(pEnum + 1);


	serialNumberLength=0;
	status = RfTrans_EnumerateRadios(&transportHandle,
										 NULL,
										 &serialNumberLength,
										 &context,
										 EA_FirstRadio);
	if ( status != CPL_ERROR_BUFFERTOOSMALL )
		return status;

	if ( serialNumberLength != 12 )
		return status;

	do
	{
		status = RfTrans_EnumerateRadios(&transportHandle,
										 serialNumber,
										 &serialNumberLength,
										 &context,
										 action);
		switch (status)
		{
			case CPL_WARN_ENDOFLIST:
			{
				break;
			}
			case CPL_ERROR_BUFFERTOOSMALL:
			{
				printf("CPL_ERROR_BUFFERTOOSMALL.  RC = %d\n", status);
				action = EA_SameRadio;
				break;
			}
			case CPL_ERROR_DEVICEGONE:
			{
				action = EA_NextRadio;
				break;
			}
			case CPL_SUCCESS:
			{
				pEnum->totalLength += sizeof(RFID_RADIO_INFO) +
										serialNumberLength +
										CALCULATE_32BIT_PADDING(serialNumberLength);

				RFID_RADIO_INFO *info = (RFID_RADIO_INFO *)pBuffer;
				pBuffer += sizeof(RFID_RADIO_INFO);

				pEnum->ppRadioInfo[pEnum->countRadios++] = info;

				info->cookie        = transportHandle;
				GetTransportCharacteristics(transportHandle,&info->driverVersion);
				info->idLength      = serialNumberLength;
				info->pUniqueId     = pBuffer;

				info->length = sizeof(RFID_RADIO_INFO) +
								info->idLength +
								CALCULATE_32BIT_PADDING(info->idLength);

				memcpy(pBuffer, serialNumber, serialNumberLength);
				pBuffer += (info->idLength + CALCULATE_32BIT_PADDING(info->idLength));

				action = EA_NextRadio;
				break;
			}
			default:
			{
				printf("RC = %d\n", status);
				break;
			}
		}
	}
	while (CPL_WARN_ENDOFLIST != status);

	return CPL_SUCCESS;
}

TransStatus AbortRadio(TransHandle transHandle)
{
	TransStatus status = RfTrans_AbortRadio(transHandle);
	switch (status)
	{
		case CPL_SUCCESS:
			printf("abort success\n");
			break;
		case CPL_ERROR_NOTFOUND:
		case CPL_ERROR_DEVICEGONE:
			printf("abort detach RC = %d\n", status);
			break;
		case CPL_WARN_CANCELLED:
			printf("abort no respond RC = %d\n", status);
			break;
		default:
			printf("abort RC = %d\n", status);
			break;
	}
	return status;
}

TransStatus CleanReadRadio(TransHandle transHandle)
{
	TransStatus        status;
	INT32U bytesAvailable;
	INT8U  Buffer[10000];
	int i = 0;
	for(;;)
	{
		INT32U bytesRetrieved = 0;
		status = RfTrans_ReadRadio(transHandle,NULL,&bytesRetrieved,&bytesAvailable,0);
		if (CPL_SUCCESS != status )
			return status;
		printf("c (%d,%d)\n",bytesRetrieved,bytesAvailable);
		if(!bytesAvailable)
			return status;
		bytesRetrieved=bytesAvailable;
		status = RfTrans_ReadRadio(transHandle,Buffer,&bytesRetrieved,&bytesAvailable,0);
		switch (status)
		{
			case CPL_SUCCESS:
				printf("d (%d,%d)\n",bytesRetrieved,bytesAvailable);
				break;
			case CPL_ERROR_ACCESSDENIED:
			case CPL_ERROR_NOTFOUND:
			case CPL_ERROR_DEVICEGONE:
				printf("read detach RC = %d\n", status);
				break;
			case CPL_WARN_CANCELLED:
				printf("read cancel RC = %d\n", status);
				break;
			case CPL_ERROR_RXOVERFLOW:
				printf("read overflow RC = %d\n", status);
				AbortRadio(transHandle);
				break;
			default:
				printf("read RC = %d\n", status);
			break;
		}
		if ( i++ > 5 )
		{
			printf("clearing too long, try abort\n");
			status = AbortRadio(transHandle);
			i = 0;
		}
		//CPL_MillisecondSleep(1000);
		//usleep(1000*1000);
		sleep(1);
	}
}

TransStatus RadioOpen(TransHandle transHandle)
{
	TransStatus        status;
	status = RfTrans_OpenRadio(transHandle);
    switch (status)
    {
		case CPL_SUCCESS:
			printf("open success\n");
#ifdef DRIVER_HAVE_BUG
{	INT8U b[8] = {0,0,0,0,0,0,0,0};
	int i;
	for ( i = 0; i < 64; i++ )
	{
		INT32U j = 0, k;
		usleep(10*1000);
		RfTrans_WriteRadio(transHandle,b,8);
		usleep(10*1000);
		RfTrans_ReadRadio(transHandle,NULL,&j,&k,0);
		if ( k > 24 )
		{
			printf("bug %d ok\n", i);
			break;
		}
	}
}
#else
//	usleep(10*1000);
#endif
			status = CleanReadRadio(transHandle);
			break;
		case CPL_ERROR_BUSY:
			printf("open busy\n");
			break;
		case CPL_ERROR_NOTFOUND:
			printf("open not found\n");
			break;
		case CPL_ERROR_DEVICEGONE:
			printf("open device gone\n");
			break;
		default:
			printf("open RC = %d\n", status);
			break;
    }
	return status;
}

TransStatus RadioClose(TransHandle transHandle)
{
	TransStatus        status;
	status = RfTrans_CloseRadio(transHandle);
	switch (status)
	{
		case CPL_SUCCESS:
			printf("close success\n");
			break;
		default:
			printf("close RC = %d\n", status);
			break;
	}
	return status;
}

TransStatus MacReset(TransHandle transHandle)
{
	TransStatus status = RfTrans_ResetRadio(transHandle,RESETRADIO_SOFT);
	//TransStatus status = RfTrans_ResetRadio(transHandle,RESETRADIO_TO_BL);
	switch (status)
	{
		case CPL_SUCCESS:
			printf("abort success\n");
			break;
		case CPL_ERROR_NOTFOUND:
		case CPL_ERROR_DEVICEGONE:
			printf("reset detach RC = %d\n", status);
			break;
		default:
			printf("reset RC = %d\n", status);
			break;
	}
	return status;
}

TransStatus RawReadRadio(TransHandle transHandle, INT8U *pBuffer, INT32U  bufferSize)
{
	INT32U bytesAvailable = 0;
	int i;
	for ( i = 0; i < 100; i++ )
	{
		TransStatus status;
		INT32U bytesRetrieved = bytesAvailable >= bufferSize ? bufferSize : 0;
		status = RfTrans_ReadRadio(transHandle,pBuffer,&bytesRetrieved,&bytesAvailable,0);
		switch (status)
		{
			case CPL_SUCCESS:
				//printf("(%d->%d,%d)",bytesRetrieved,bufferSize,bytesAvailable);
				break;
			case CPL_ERROR_ACCESSDENIED:
			case CPL_ERROR_NOTFOUND:
			case CPL_ERROR_DEVICEGONE:
				printf("read detach RC = %d\n", status);
				break;
			case CPL_WARN_CANCELLED:
				printf("read cancel RC = %d\n", status);
				break;
			case CPL_ERROR_RXOVERFLOW:
				printf("read overflow RC = %d\n", status);
				AbortRadio(transHandle);
				break;
			default:
				printf("read RC = %d\n", status);
				break;
		}
		if ( bytesRetrieved || CPL_SUCCESS != status )
		{
			//printf("\n");
			return status;
		}
		usleep ( 1000 );
	}
	return CPL_WARN_TIMEOUT;
}

TransStatus WriteRadio(TransHandle transHandle, const INT8U *pBuffer, INT32U bufferSize)
{
	TransStatus        status;
	status = RfTrans_WriteRadio(transHandle,(INT8U *)pBuffer,bufferSize);
	switch (status)
	{
		case CPL_SUCCESS:
			//printf("(w%d)",bufferSize);
			break;
		case CPL_ERROR_ACCESSDENIED:
		case CPL_ERROR_NOTFOUND:
		case CPL_ERROR_DEVICEGONE:
			printf("write detach RC = %d\n", status);
			break;
		case CPL_WARN_CANCELLED:
			printf("write cancel RC = %d\n", status);
			break;
		default:
			printf("write RC = %d\n", status);
			break;
	}
	return status;
}





struct rfid_reader_info
{
	const RFID_RADIO_INFO *pRadioInfo;
	char reader_name[100];
};

struct rfid_reader_list
{
	RFID_RADIO_ENUM *pEnum;
	struct rfid_reader_info reader_info[100];
	int reader_count;
};

int EnumReader ( struct rfid_reader_list *reader_list )
{
	RFID_RADIO_ENUM *pEnum;
	TransStatus status;
	int index, i;
	
	/* Allocate a buffer for radio enumeration */
	//too small -> Segmentation fault
//	pEnum = (RFID_RADIO_ENUM *) malloc(1000);
	pEnum = (RFID_RADIO_ENUM *) malloc(460);
	if (NULL == pEnum)
	{
		printf("Failed to allocate memory\n");
		exit (1);
	}

	/* Initialize the enumeration buffer */
	status = EnumerateAttachedRadios ( pEnum );

	/* Verify that the enumeration was successful */
	if (CPL_SUCCESS != status )
	{
		printf("EnumerateAttachedRadios failed:  RC = %d\n", status);
		//exit (1);
	}

	printf("RFID_RADIO_ENUM.length      = %u\n", pEnum->length);
	printf("RFID_RADIO_ENUM.totalLength = %u\n", pEnum->totalLength);
	printf("RFID_RADIO_ENUM.countRadios = %u\n", pEnum->countRadios);

	/* For each radio in the list, open it, try to get its MAC version,    */
	/* try to perform an inventory, then close it                          */
	for (index = 0; index < pEnum->countRadios; ++index)
	{
		RFID_VERSION macVersion;

		printf("Radio %u:\n", index + 1);
		printf("\tStructure length: %u\n", pEnum->ppRadioInfo[index]->length);
		printf("\tCookie: 0x%.8x\n", pEnum->ppRadioInfo[index]->cookie);
		printf("\tDriver Version:\n");
		printf("\t\tMajor: %u\n",
			   pEnum->ppRadioInfo[index]->driverVersion.major);
		printf("\t\tMinor: %u\n",
			   pEnum->ppRadioInfo[index]->driverVersion.minor);
		printf("\t\tMaintenance: %u\n",
			   pEnum->ppRadioInfo[index]->driverVersion.maintenance);
		printf("\t\tRelease: %u\n",
			   pEnum->ppRadioInfo[index]->driverVersion.release);
		printf("\tUnique ID Length: %u\n", pEnum->ppRadioInfo[index]->idLength);
		printf("\tUnique ID: \n");
		printf("\t\tHEX: ");
		for(i=0;i<pEnum->ppRadioInfo[index]->idLength;i++)
			printf("%02x",pEnum->ppRadioInfo[index]->pUniqueId[i]);
		printf(".\n");
		printf("\t\tTEXT: ");
		for(i=0;i<pEnum->ppRadioInfo[index]->idLength;i++)
			printf("%c",isprint(pEnum->ppRadioInfo[index]->pUniqueId[i])?pEnum->ppRadioInfo[index]->pUniqueId[i]:'?');
		printf("\n");

		reader_list->reader_info[index].pRadioInfo = pEnum->ppRadioInfo[index];
		for(i=0;i<reader_list->reader_info[index].pRadioInfo->idLength;i++)
			reader_list->reader_info[index].reader_name[i] = isprint(reader_list->reader_info[index].pRadioInfo->pUniqueId[i]) ? reader_list->reader_info[index].pRadioInfo->pUniqueId[i] : '?';
		reader_list->reader_info[index].reader_name[i] = '\0';
	}
	reader_list->pEnum = pEnum;
	reader_list->reader_count = pEnum->countRadios;

	return 0;
}

struct rfid_reader_context
{
	TransHandle transHandle;
	TransStatus status;
};

int rfid_status_error ( TransStatus status )
{
	return CPL_SUCCESS != status;
}

const char *rfid_status_string ( TransStatus status, char *buf )
{
	switch ( status )
	{
		case CPL_SUCCESS:
			strcpy ( buf, "CPL_SUCCESS" );
			break;
		default:
			sprintf ( buf, "TransStatus = %d", status );
			break;
	}
	return buf;
}

int rfid_reader_open ( struct rfid_reader_context *ctx, const struct rfid_reader_info *reader_info )
{
	ctx->transHandle = reader_info->pRadioInfo->cookie;
	ctx->status = RadioOpen( ctx->transHandle );
	return CPL_SUCCESS != ctx->status;
}

int rfid_reader_close ( struct rfid_reader_context *ctx )
{
	ctx->status = RadioClose(ctx->transHandle);
	return CPL_SUCCESS != ctx->status;
}

int rfid_reader_abort ( struct rfid_reader_context *ctx )
{
	ctx->status = AbortRadio(ctx->transHandle);
	return CPL_SUCCESS != ctx->status;
}

int rfid_reader_clean_read ( struct rfid_reader_context *ctx )
{
	ctx->status = CleanReadRadio(ctx->transHandle);
	return CPL_SUCCESS != ctx->status;
}

int rfid_reader_write ( struct rfid_reader_context *ctx, const void *data, int count )
{
	ctx->status = WriteRadio ( ctx->transHandle, data, count );
	return CPL_SUCCESS != ctx->status;
}

int rfid_reader_read ( struct rfid_reader_context *ctx, void *data, int count )
{
	ctx->status = RawReadRadio ( ctx->transHandle, data, count );
	return CPL_SUCCESS != ctx->status;
}
