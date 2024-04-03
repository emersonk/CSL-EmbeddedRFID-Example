/*
 *****************************************************************************
 *                                                                           *
 *                 CS203/468 Integrated RFID Reader                          *
 *                                                                           *
 * This source code is the sole property of Convergence Systems Ltd.         *
 * Reproduction or utilization of this source code in whole or in part is    *
 * forbidden without the prior written consent of Convergence Systems Ltd.   *
 *                                                                           *
 * (c) Copyright Convergence Systems Ltd. 2010. All rights reserved.         *
 *                                                                           *
 *****************************************************************************/

/**
 *****************************************************************************
 **
 ** @file  example.c
 **
 ** @brief Simple example of reader
 **
 **
 ** The steps:
 **     - Connect to the reader (TCP)
 **     - Reader Initialization
 **     - Start Inventory
 **     - Read tag (5s)
 **     - Stop Inventory
 **	- Close
 **
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/times.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <wait.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#include "tx/code/csl_rfid_tx.inc.c"
struct rfid_reader_list rfid_reader_list;

#include "cs203_dc.h"
#include "freqTable.h"
#include "oemTable.h"

#define TIMESTART(x) x = time(NULL)
#define TIMEOUT(x, y) ((time(NULL) - x) > y ? 1 : 0)

#if debug
#define DB(cmd) cmd2
#else
#define DB(cmd)
#define sylog(cmd)
#endif

/*#define DEFAULT_CTRLCMDPORT	1516
#define DEFAULT_INTELCMDPORT	1515
#define DEFAULT_UDPPORT		3041
#define DEFAULT_DUPELICOUNT	0*/

// #define DATABUFSIZE 256		// general data buffer size
#define DATABUFSIZE 1024 // general data buffer size

// UDP command Mode
/*#if 1
#define BOARDCASEADDRESS	ipv4addr
#else
#define BOARDCASEADDRESS	INADDR_BROADCAST
#endif*/

int _strHex2ByteArray(char *in, char *out)
{
	char szTmp[3];
	unsigned int i;
	char *endptr;

	if (in == NULL || out == NULL)
		return -1;

	/* make sure that length of the message is even number */
	if (strlen(in) % 2 != 0)
		return -1;

	for (i = 0; i < strlen(in) / 2; i++)
	{
		memcpy(szTmp, &in[i * 2], 2);
		szTmp[2] = '\0';
		out[i] = strtol(szTmp, &endptr, 16);
	}

	return i;
}

// compare a hex string with a byte array, limit to len <cmplen> in bytes,
// both string and array should be the same size and longer than <cmplen> or -1 will be returned
// return 0 for both string and byte contains same value, otherwise, return -1
int _cmpHexStr2HexByteArray(char *hexstr, char *hexbytearray, int arraydatasize, int cmplen)
{
	char buf[64];
	int datasize;
	int len;
	int i;

	if (arraydatasize <= 0)
		return -1;

	datasize = _strHex2ByteArray(hexstr, buf);

	if (datasize <= 0)
		return -1;

	if (cmplen > 0)
	{
		if (cmplen <= datasize && cmplen <= arraydatasize)
			len = cmplen;
		else
			return -1;
	}
	else
	{
		if (datasize != arraydatasize)
			return -1;
		else
			len = datasize;
	}

	for (i = 0; i < len; i++)
	{
		if (buf[i] != hexbytearray[i])
			return -1;
	}

	return 0;
}

/*void printcurtime ()
{
	time_t   timep;

   timep = time (NULL);
   printf ("%s", (char *)asctime (localtime (&timep)));
}*/

int ClearRecvBuffer(CS203DC_Handle *handle)
{
#ifdef STUPID
	int nrecv;
	unsigned char buf[DATABUFSIZE];
	int cnt = 0;
	int k;

	// Clear TCP Buffers
	do
	{
		nrecv = recv(socket, buf, 32, 0);
		printf("ClearTCP nrecv : %d \n", nrecv);
		for (k = 0; k < 8; k++)
			printf("%2X", buf[k]);
		printf("\n");

		cnt++;
		usleep(3000); // 3 ms

	} while ((nrecv > 0) && (cnt < 20));
#else
	// printf ( "ClearRecvBuffer do nothing , too stupid to call here\n" );
#endif
	return 0;
}

int CS203DC_Close(CS203DC_Handle *handle)
{
	if (rfid_reader_close(&handle->rfid_reader_context))
		return -1;

	handle->state = CS203DCSTATE_IDLE;

	return 0;
}

int CS203DC_OpenConnection(CS203DC_Handle *handle)
{
	int ret = 0;

	if (rfid_reader_open(&handle->rfid_reader_context, handle->rfid_reader_info))
		return -1;

	if (handle->state != CS203DCSTATE_IDLE)
		return -2;

	return ret;
}

int CS203DC_Open(CS203DC_Handle *handle)
{
	int retint1, retint2;
	int ret = 0;
	struct sockaddr_in inet_addrs, cnet_addrs, unet_addrs;
	unsigned char buf1[DATABUFSIZE];
	unsigned char buf2[DATABUFSIZE];
	int buf1_datalen = 0;
	int nsent;
	int nrecv;
	int i;
	int rfidboardinited = 0;
	struct hostent *hp1 = NULL;
	long ipv4addr;

	ret = CS203DC_OpenConnection(handle);
	// printCS203DC_Handle(handle);							// print CS203DC handle

	printf("CS203DC_OpenConnection : %d \n", ret);
	if (ret != 0)
		return ret;

	{
		handle->state = CS203DCSTATE_CONNECTED;
		syslog(LOG_INFO, "%s(): line %d, cs203 opened", __FUNCTION__, __LINE__);
	}

	return ret;
}

int _sendintelcmd(CS203DC_Handle *handle, char *strdata)
{
#ifdef STUPID
	unsigned char buf1[DATABUFSIZE];
	int buf1_datalen = 0;
	int nsent;
	int i;

	usleep(1000); // just to ensure we have at least 1ms of delay
	for (i = 0; i < 10; i++)
	{
		DB(syslog(LOG_INFO, "%s(): line %d, try %d", __FUNCTION__, __LINE__, i));
		buf1_datalen = _strHex2ByteArray(strdata, buf1);
		nsent = send(handle->intelcmdfd, buf1, buf1_datalen, 0);
		usleep(10);
		DB(syslog(LOG_INFO, "%s(): line %d, command %d bytes, sent %d", __FUNCTION__, __LINE__, buf1_datalen, nsent));
		if (nsent < 0)
		{
			if (errno != EAGAIN)
			{
				return errno;
			}
		}
		else
		{
			// assume always sent all data
			break;
		}
	}
	return nsent;
#else
	unsigned char buf1[DATABUFSIZE];
	int buf1_datalen = _strHex2ByteArray(strdata, buf1);
	if (rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen))
		return -1;
	printf("Send: %s\n", strdata);
	return buf1_datalen;
#endif
}

int CS203DC_StartInventory(CS203DC_Handle *handle)
{
	int err;

	if (handle == NULL)
	{
		printf("CS203DC_StartInventory: NULL handle");
		return -2;
	}

	if (handle->state != CS203DCSTATE_CONNECTED)
	{
		printf("CS203DC_StartInventory: handle->state is not CS203DCSTATE_CONNECTED");
		return -2;
	}

	// fcntl(handle->intelcmdfd, F_SETFL, fcntl(handle->intelcmdfd,F_GETFL,0)|O_NONBLOCK);
	if ((err = _sendintelcmd(handle, CS203DC_STARTINVENTORY14)) < 0)
	{
		syslog(LOG_ERR, "%s(): line %d, cannot send STARTINVENTORY14, err %d", __FUNCTION__, __LINE__, err);
		return err;
	}
	ClearRecvBuffer(handle); // Clear Command-End Packet

	// initialized status
	DB(syslog(LOG_INFO, "%s(): line %d, reset buffer (%d)", __FUNCTION__, __LINE__, sizeof(handle->tag)));
	handle->tagbeginidx = 0;
	memset(handle->tag, 0, sizeof(handle->tag));
	handle->state = CS203DCSTATE_INV;
	//	fcntl(handle->intelcmdfd, F_SETFL, fcntl(handle->intelcmdfd,F_GETFL,0)|O_NONBLOCK);

	// Clear flag for inventory start
	handle->flag_decodePacket = 0;
	handle->decodePacketBuf_offset = 0;

	return 0;
}

/// <summary>
/// Read Register
/// </summary>
/*int ReadRegister (CS203DC_Handle* handle, unsigned int add, unsigned int *value)
{
	char CMD[100];
	unsigned char recvbuf[100];
	int nread = 0;

	sprintf (CMD, "7000%02x%02x00000000",
		(unsigned char)add,
			(unsigned char)(add >> 8));

	_sendintelcmd(handle, CMD);

	/ *nread = recv(handle->intelcmdfd, recvbuf, 8, 0);

	if(nread != 8)
	{
		printf ("%s() line %d, response error %d", __FUNCTION__, __LINE__, errno);
		return 0;
	}* /

		*value = (unsigned int)(recvbuf[7] << 24 | recvbuf[6] << 16 | recvbuf[5] << 8 | recvbuf[4]);
	return 1;
}*/

/// <summary>
/// Write Register
/// </summary>
int WriteRegister(CS203DC_Handle *handle, unsigned int add, unsigned int value)
{
	char CMD[100];

	sprintf(CMD, "7001%02x%02x%02x%02x%02x%02x",
			(unsigned char)add,
			(unsigned char)(add >> 8),
			(unsigned char)value,
			(unsigned char)(value >> 8),
			(unsigned char)(value >> 16),
			(unsigned char)(value >> 24));

	_sendintelcmd(handle, CMD);

	return 1;
}

int CS203DC_StopInventory(CS203DC_Handle *handle)
{
#ifdef STUPID
	unsigned char buf1[DATABUFSIZE];
	int buf1_datalen = 0;
	int nsent;
	int i;

	if (handle == NULL)
	{
		return -2;
	}

	if (handle->state == CS203DCSTATE_IDLE)
	{
		return -2;
	}

	for (i = 0; i < 10; i++)
	{
		usleep(10000);
		DB(syslog(LOG_INFO, "%s(): line %d, abort command, try %d", __FUNCTION__, __LINE__, i));
		buf1_datalen = _strHex2ByteArray(CS203DC_ABORTCMD, buf1);
		nsent = send(handle->intelcmdfd, buf1, buf1_datalen, 0);
		DB(syslog(LOG_INFO, "%s(): line %d, command %d bytes, sent %d", __FUNCTION__, __LINE__, buf1_datalen, nsent));
		ClearTCP_RecvBuffer(handle->intelcmdfd); // Clear Command Return Packet
		if (nsent < 0)
		{
			if (errno != EAGAIN)
			{
				return errno;
			}
		}
		else
		{
			break;
		}
	}

	handle->state = CS203DCSTATE_CONNECTED;

	// Clean Read Tag Buffer
	for (i = 0; i < 100; i++)
		usleep(10000);

	return 0;
#else
	int err;

	if (handle->state == CS203DCSTATE_IDLE)
	{
		return -2;
	}

	err = rfid_reader_abort(&handle->rfid_reader_context);
	if (err)
		return -2;

	usleep(10000);

	err = rfid_reader_clean_read(&handle->rfid_reader_context);
	if (err)
		return -2;

	handle->state = CS203DCSTATE_CONNECTED;
	return 0;
#endif
}

int SetFreq_None(CS203DC_Handle *handle)
{
	char sPacketCommand[50];
	int i;

	// for (i = 4; i < 50; i++)
	for (i = 0; i < 50; i++)
	{
		sprintf(sPacketCommand, "7001010C%02X000000", i); // Select Channel
		_sendintelcmd(handle, sPacketCommand);
		// printf ("Set Frequency none : %s \n", sPacketCommand);
		sprintf(sPacketCommand, "7001020C00000000"); // Disable Channel
		_sendintelcmd(handle, sPacketCommand);
	}
}

unsigned int swapMSBLSB32bit(unsigned int in_32bit)
{
	unsigned int t_shift[] = {0, 8, 16, 24};
	unsigned int t_tmpVal[4];
	unsigned int out_32bit;
	int j;

	out_32bit = 0;
	for (j = 0; j < 4; j++)
	{
		t_tmpVal[j] = (in_32bit >> t_shift[j]) & 0xff;
		out_32bit |= t_tmpVal[j] << t_shift[3 - j];
	}

	return out_32bit;
}

int SetFreq_Table(CS203DC_Handle *handle, const unsigned int const *freqTable, int num_channel)
{
	char sPacketCommand[50];
	int i;
	unsigned int t_freqVal;

	//	SetFreq_None (handle);

	for (i = 0; i < num_channel; i++)
	{
		sprintf(sPacketCommand, "7001010C%02X000000", i); // Select Channel
		_sendintelcmd(handle, sPacketCommand);

		// printf("freqTable %d, %08X \n", i, freqTable[i]);
		t_freqVal = swapMSBLSB32bit(freqTable[i]);
		// printf("t_freqVal %08X \n", t_freqVal);

		sprintf(sPacketCommand, "7001030C%08X", t_freqVal); // Set Freq
		_sendintelcmd(handle, sPacketCommand);

		sprintf(sPacketCommand, "7001020C01000000"); // Enable Channel
		_sendintelcmd(handle, sPacketCommand);
	}

	return 0;
}

// check with the duplication elimination buffer
void check_duplication(CS203DC_Handle *handle, int epcbytelen, unsigned char *curinbuf)
{
#if 0
	if(handle->dupelicount > 0)
	{
		int i;
		CS203DC_TagInfo* tag = NULL;
		for(i = 0; i < MAX_TAGBUFLEN; i++)
		{
			if(epcbytelen == handle->tag[i].len)
			{
				if(memcmp(&curinbuf[2], handle->tag[i].epc, epcbytelen) == 0)
				{
					// tag existed
					if(handle->tag[i].count <= 0)
					{
						// count down to 0 already, reset the entry and consider as not found
						memset(handle->tag[i].epc, 0, MAX_EPCLEN);
						handle->tag[i].len = 0;
						handle->tag[i].count = 0;
					}
					else
					{
						handle->tag[i].count--;
						tag = &(handle->tag[i]);
						break;
					}
				}
			}
		}

		// add to buffer if it is a new tag
		if(tag == NULL)
		{
			handle->tag[handle->tagbeginidx].len = epcbytelen;
			handle->tag[handle->tagbeginidx].count = handle->dupelicount;
			memcpy(handle->tag[handle->tagbeginidx].epc, &curinbuf[2], epcbytelen);
			handle->tagbeginidx++;
			if(handle->tagbeginidx >= MAX_TAGBUFLEN)
			{
				handle->tagbeginidx = 0;
			}
		}
	}
#else
	{
		int newtag_old = handle->newTag;
		int i;
		CS203DC_TagInfo *tag = NULL;
		for (i = 0; i < handle->tagbeginidx && i < MAX_TAGBUFLEN; i++)
		{
			if (handle->tagbeginidx >= MAX_TAGBUFLEN)
				break;
			if (epcbytelen == handle->tag[i].len)
			{
				if (memcmp(&curinbuf[2], handle->tag[i].epc, epcbytelen) == 0)
				{
					tag = &(handle->tag[i]);
					break;
				}
			}
		}

		// add to buffer if it is a new tag
		if (tag == NULL)
		{
			handle->uTag++;
			handle->newTag++;
			handle->tag[handle->tagbeginidx].len = epcbytelen;
			memcpy(handle->tag[handle->tagbeginidx].epc, &curinbuf[2], epcbytelen);
			handle->tagbeginidx++;
			if (handle->tagbeginidx >= MAX_TAGBUFLEN)
				printf("\Error: %d tag storage buffer is not enough for duplication checking\n", MAX_TAGBUFLEN);
		}
		handle->numTag++;
		// if (handle->newTag != newtag_old) printf("add new tags = %d with epc = %02X%02X\n", handle->newTag - newtag_old, curinbuf[2], curinbuf[3]);
	}
#endif
}

int CS203DC_DecodePacket(CS203DC_Handle *handle, unsigned char *inbuf, int inlen, int *inprocessed, unsigned char *outbuf, int outlen, int *outused, unsigned int *errorcode)
{
	int nproc = 0;
	int nused = 0;
	unsigned char *curinbuf = inbuf;
	int curinlen = inlen;
	unsigned char *curoutbuf = outbuf;
	int curoutlen = outlen;
	int ret = 0;
	int packetlen;
	// static time_t CycleEndPacketTime;

	unsigned int t_tmpval;

	int t1_packetlen;
	int k;

	*inprocessed = 0;
	*outused = 0;
	*curoutbuf = 0;

	char buf_debugMsg[2 * curinlen];
	for (k = 0; k < curinlen; k++)
		sprintf(buf_debugMsg + 2 * k, "%02X", curinbuf[k]);

	if (curinlen < 8)
		printf("Error with inlen = %d < 8: %s\n", curinlen, buf_debugMsg);
	while (curinlen >= 8)
	{
		/*if (_cmpHexStr2HexByteArray(CS203DC_ABORTCMD_RSP, curinbuf, 8, 8) == 0) // check for abort command response, need 8 byte
		{
			printf ("%s(): line %d, abort command response received", __FUNCTION__, __LINE__);
			curinbuf += 8;
			curinlen -= 8;
			nproc += 8;
			*inprocessed = nproc;
		}
		// else if (memcmp (curinbuf, "\x98\x98\x98\x98\x98\x98\x98\x98", 8) == 0)
		else if (_cmpHexStr2HexByteArray(CS203DC_TCPNOTIFY_RSP, curinbuf, 8, 8) == 0)
		{
			printf ("Notification Packet Received! ");
			curinbuf += 8;
			curinlen -= 8;
			nproc += 8;
			*inprocessed = nproc;
		}
		else if ((memcmp (curinbuf, "\x02\x00\x07\x80", 4) == 0) || (memcmp (curinbuf, "\x01\x00\x07\x80", 4) == 0))
		{
			printf ("Antenna Cycle End Packet Received! ");
			curinbuf += 8;
			curinlen -= 8;
			nproc += 8;
			*inprocessed = nproc;
		}
		else*/
		{
			CS203DC_PacketHeader header;

			header.pkt_ver = curinbuf[0];
			header.flags = curinbuf[1];
			header.pkt_type = curinbuf[2] + (curinbuf[3] << 8);
			header.pkt_len = curinbuf[4] + (curinbuf[5] << 8);
			header.reserved = curinbuf[6] + (curinbuf[7] << 8);
			packetlen = 8 + (header.pkt_len * 4);
			if (curinbuf[0] == 4 && (header.pkt_type == 0x8005 || header.pkt_type == 0x0005))
				packetlen = 8 + (header.pkt_len);

			if (curinlen >= packetlen)
			{
				// complete packet is read
				switch (header.pkt_type)
				{
				case 0x8007: // Antenna-Cycle-End Packet
					printf("Receive Antenna Cycle End Packet\n");
					// CycleEndPacketTime = time(NULL);
					break;

				case 0x8005:
				case 0x0005:
					// inventory response
					// DB(syslog(LOG_INFO,"%s(): line %d, inventory response", __FUNCTION__, __LINE__));
					if ((header.flags & 0x01) == 0 && curinbuf[0] == 4)
					{
						// printf("v4 inbuf = %s\n", buf_debugMsg);
						int i = 8, k;
						do
						{
							int epcbytelen;
							CS203DC_TagInfo *tag = NULL;
							epcbytelen = ((curinbuf[i] >> 3) & 0x0000001F) * 2;
							if ((i + 2 + epcbytelen + 1) > packetlen)
							{
								printf("Error with invalid length epc=%d: inbuf = %s\n", epcbytelen, buf_debugMsg);
								break;
							}
							sprintf(curoutbuf, "<TAG=");
							curoutbuf += 5;
							curoutlen -= 5;
							nused += 5;

							sprintf(curoutbuf, "%02X", curinbuf[i++]);
							sprintf(curoutbuf + 2, "%02X,", curinbuf[i++]);
							curoutbuf += 5;
							curoutlen -= 5;
							nused += 5;
							for (k = 0; k < epcbytelen; k++, i++, curoutbuf += 2, curoutlen -= 2, nused += 2)
								sprintf(curoutbuf, "%02X", curinbuf[i]);
							check_duplication(handle, epcbytelen, curinbuf + i - epcbytelen - 2);

							double d_mantissa = (double)(curinbuf[i] % 8); //& 0x7);
							double d_exponent = (double)(curinbuf[i++] / 8);
							float f_tmpvalb = 1.0 + ((float)d_mantissa / 8.0);
							float f_tmpvala = f_tmpvalb * (float)pow(2.0, d_exponent);
							float f_tmpval = 200 * log10f(f_tmpvala);
							t_tmpval = (unsigned int)f_tmpval;
							sprintf(curoutbuf, " RSSI=%03d.%01d", (t_tmpval / 10), (t_tmpval % 10));
							curoutbuf += 11;
							curoutlen -= 11;
							nused += 11;

							int portNumber = curinbuf[6];
							sprintf(curoutbuf, " Port=%02d>", portNumber);
							curoutbuf += 9;
							curoutlen -= 9;
							nused += 9;
						} while (i < packetlen);
					}
					else if ((header.flags & 0x01) == 0 && curinbuf[0] == 3)
					{
						int epcbytelen;
						CS203DC_TagInfo *tag = NULL;
						epcbytelen = ((curinbuf[20] >> 3) & 0x0000001F) * 2;

						if ((20 + 2 + epcbytelen + curinbuf[16] + curinbuf[17] + 2) > packetlen)
							printf("Error with invalid length epc=%d, data1=%d, data2=%d: inbuf = %s\n", epcbytelen, curinbuf[16], curinbuf[17], buf_debugMsg);
						// printf("v3 inbuf = %s\n", buf_debugMsg);

						// output only when tag is new and output buffer have enough space
						// if(tag == NULL && curoutlen > (epcbytelen * 2) + 6) // +6 for the output format <EPC:XX...XX>
						if (tag == NULL)
						{
							int i;
							DB(syslog(LOG_INFO, "%s(): line %d, curoutlen %d, *outused %d", __FUNCTION__, __LINE__, curoutlen, *outused));
							sprintf(curoutbuf, "<TAG=");
							curoutbuf += 5;
							curoutlen -= 5;
							nused += 5;

							sprintf(curoutbuf, "%02X", curinbuf[20]);
							sprintf(curoutbuf + 2, "%02X,", curinbuf[21]);
							curoutbuf += 5;
							curoutlen -= 5;
							nused += 5;
							for (i = 0; i < epcbytelen; i++, curoutbuf += 2, curoutlen -= 2, nused += 2)
								sprintf(curoutbuf, "%02X", curinbuf[22 + i]);
							check_duplication(handle, epcbytelen, curinbuf + 20);
							sprintf(curoutbuf, ",");
							curoutbuf += 1;
							curoutlen -= 1;
							nused += 1;
							if ((curinbuf[16] + curinbuf[17]) > (curinlen - 22 - i - 2))
							{
								printf("Error in data lengths: curinlen=%d i=%d remain=%d\n", curinlen, i, curinlen - i - 2);
								sprintf(curoutbuf, ",,");
								curoutbuf += 2;
								curoutlen -= 2;
								nused += 2;
							}
							else
							{
								if (curinbuf[16] != 0)
								{
									for (i = 0; i < curinbuf[16]; i++, curoutbuf += 2, curoutlen -= 2, nused += 2)
										sprintf(curoutbuf, "%02X", curinbuf[22 + i]);
								}
								sprintf(curoutbuf, ",");
								curoutbuf += 1;
								curoutlen -= 1;
								nused += 1;
								if (curinbuf[17] != 0)
								{
									for (i = 0; i < curinbuf[17]; i++, curoutbuf += 2, curoutlen -= 2, nused += 2)
										sprintf(curoutbuf, "%02X", curinbuf[22 + i]);
								}
								sprintf(curoutbuf, ",");
								curoutbuf += 1;
								curoutlen -= 1;
								nused += 1;
								sprintf(curoutbuf, "%02X", curinbuf[22 + i]);
								sprintf(curoutbuf + 2, "%02X", curinbuf[23 + i]);
								curoutbuf += 4;
								curoutlen -= 4;
								nused += 4;
							}

							////////////////////////////////////////////////////////////////
							unsigned int t_size;
							char buf_prtMsg[30];
							// Number of Data Bank
							if (handle->num_dBank)
							{
								sprintf(buf_prtMsg, "\n Data Bank No.: %d", handle->num_dBank);
								strcpy(curoutbuf, buf_prtMsg);
								t_size = strlen(buf_prtMsg);
								curoutbuf += t_size;
								curoutlen -= t_size;
								nused += t_size;
							}

							unsigned int t_tid_dLen = (unsigned int)(handle->tid_length) * 2; // t_tid_dLen = tid_length*2 bytes should be read
							if (handle->tid_length)
							{
								// TID data length
								sprintf(buf_prtMsg, "\n TID length(WORD): %d", handle->tid_length);
								strcpy(curoutbuf, buf_prtMsg);
								t_size = strlen(buf_prtMsg);
								curoutbuf += t_size;
								curoutlen -= t_size;
								nused += t_size;

								// TID value
								sprintf(buf_prtMsg, "\n TID Data: ");
								strcpy(curoutbuf, buf_prtMsg);
								t_size = strlen(buf_prtMsg);
								curoutbuf += t_size;
								curoutlen -= t_size;
								nused += t_size;

								// TID data print
								int tid_idx;
								tid_idx = epcbytelen;
								for (i = tid_idx; i < tid_idx + t_tid_dLen; i++, curoutbuf += 2, curoutlen -= 2, nused += 2)
								{
									sprintf(curoutbuf, "%02X", curinbuf[22 + i]);
									// sprintf(&tzTagID[i << 1], "%02X", curinbuf[22 + i]);
								}
							}

							unsigned int t_user_dLen = (unsigned int)(handle->user_length) * 2; // t_user_dLen = user_length*2 bytes should be read
							if (handle->user_length)
							{
								// User data length
								sprintf(buf_prtMsg, "\n User length(WORD): %d", handle->user_length);
								strcpy(curoutbuf, buf_prtMsg);
								t_size = strlen(buf_prtMsg);
								curoutbuf += t_size;
								curoutlen -= t_size;
								nused += t_size;

								// User value
								sprintf(buf_prtMsg, "\n User Data: ");
								strcpy(curoutbuf, buf_prtMsg);
								t_size = strlen(buf_prtMsg);
								curoutbuf += t_size;
								curoutlen -= t_size;
								nused += t_size;

								// User data print
								int user_idx;
								user_idx = epcbytelen + t_tid_dLen;
								for (i = user_idx; i < user_idx + t_user_dLen; i++, curoutbuf += 2, curoutlen -= 2, nused += 2)
								{
									sprintf(curoutbuf, "%02X", curinbuf[22 + i]);
									// sprintf(&tzTagID[i << 1], "%02X", curinbuf[22 + i]);
								}
							}

							/////////////////////////////////////////////////////////////////////
							double d_mantissa = (double)(curinbuf[13] % 8); //& 0x7);
							double d_exponent = (double)(curinbuf[13] / 8);
							float f_tmpvalb = 1.0 + ((float)d_mantissa / 8.0);
							float f_tmpvala = f_tmpvalb * (float)pow(2.0, d_exponent);
							float f_tmpval = 200 * log10f(f_tmpvala);
							t_tmpval = (unsigned int)f_tmpval;
							// t_tmpval = (unsigned int )(curinbuf[13] * 8);
							// sprintf (curoutbuf, "\n RS=%d.%d", t_tmpval, (unsigned)curinbuf[13]);
							sprintf(curoutbuf, " RSSI=%03d.%01d", (t_tmpval / 10), (t_tmpval % 10));
							curoutbuf += 11;
							curoutlen -= 11;
							nused += 11;

							int portNumber = curinbuf[18] + (curinbuf[19] << 8);
							sprintf(curoutbuf, " Port=%02d>", portNumber);
							curoutbuf += 9;
							curoutlen -= 9;
							nused += 9;

							/*
							sprintf (curoutbuf, ">");

							curoutbuf++;
							curoutlen--;
							nused++;
							*/
						}
						else
						{
							// just discard that tag
							DB(syslog(LOG_INFO, "%s(): line %d, tag discarded, or duplicated", __FUNCTION__, __LINE__));
						}
					}
					else if ((header.flags & 0x01) == 0)
						printf("Error with non-zero flag: inbuf = %s\n", buf_debugMsg);
					else
						printf("Error with package version: inbuf = %s\n", buf_debugMsg);
					break;

					/*case 0x8000:	// command begin
						syslog(LOG_INFO,"%s(): line %d, command begin", __FUNCTION__, __LINE__);
						CycleEndPacketTime = time(NULL);
						break;

					case 0x8001:
					// decode error code
						*errorcode = curinbuf[12] + (curinbuf[13] << 8) + (curinbuf[14] << 16) + (curinbuf[15] << 24);
						printf ("%s(): line %d, curinbuf: 0x%02X 0x%02X 0x%02X 0x%02X", __FUNCTION__, __LINE__, curinbuf[12], curinbuf[13], curinbuf[14], curinbuf[15]);
						printf ("%s(): line %d, command end with error code: 0x%08X", __FUNCTION__, __LINE__, *errorcode);
						syslog(LOG_INFO,"%s(): line %d, command end with error code: 0x%08X", __FUNCTION__, __LINE__, *errorcode);
						if (errorcode != 0)
							ret = 2;
						break;

					case 0x000B:
					// inventory cycle begin
						DB(syslog(LOG_INFO,"%s(): line %d, inventory cycle begin", __FUNCTION__, __LINE__));
						break;

					case 0x000C:
					// inventory cycle end
						DB(syslog(LOG_INFO,"%s(): line %d, inventory cycle end", __FUNCTION__, __LINE__));
						break;

					case 0x1008:
					// inventory cycle end diagnostics
						DB(syslog(LOG_INFO,"%s(): line %d, inventory cycle end disgnostics", __FUNCTION__, __LINE__));
						break;*/

				case 0x0000:
					printf("-Command-Begin: %s\n", buf_debugMsg);
					break;
				case 0x0001:
					printf("-Command-End: %s\n", buf_debugMsg);
					break;
				case 0x0002:
					printf("--Antenna-Cycle-Begin\n");
					break;
				case 0x0007:
					if (handle->simpledisplay == 0)
						printf("--Antenna-Cycle-End: %s\n", buf_debugMsg);
					break;
				case 0x0003:
					printf("---Antenna-Begin\n");
					break;
				case 0x0008:
					printf("---Antenna-End\n");
					break;
				case 0x0004:
					printf("-----ISO 18000-6C Inventory-Round-Begin\n");
					break;
				case 0x0009:
					printf("-----ISO 18000-6C Inventory-Round-End\n");
					break;
				case 0x000A:
					printf("----Inventory-Cycle-Begin\n");
					break;
				case 0x000B:
					printf("----Inventory-Cycle-End\n");
					break;
				case 0x000C:
					printf("-------Carrier Info\n");
					break;
				default:
					printf("unknown packet, type = %x, len = %d -> %d\n", header.pkt_type, header.pkt_len, packetlen);
					// syslog(LOG_WARNING,"%s(): line %d, unknown packet type 0x%04X", __FUNCTION__, __LINE__, header.pkt_type);
					break;
				}

				// consume the packet
				curinbuf += packetlen;
				curinlen -= packetlen;
				nproc += packetlen;
				*inprocessed = nproc;
				*outused = nused;

				// Clear flag if Packet is decoded
				handle->flag_decodePacket = 0;
				handle->decodePacketBuf_offset = 0;
			}
			else // if(curinlen < packetlen)
			{
				// need more data, return normally

				// Set flag if Packet is not decoded
				handle->flag_decodePacket = packetlen;
				handle->decodePacketBuf_offset = curinlen;
				// printf("flag_decodePacket - curinlen : %d \n", handle->decodePacketBuf_offset);

				return ret;
				//				break;
			}
		}
	}

	// if (curinlen)
	{
		// need more data, return normally
		// DB(syslog(LOG_INFO,"%s(): line %d, return curinlen too small %d bytes, header", __FUNCTION__, __LINE__, curinlen));
		ret = 0;
	}
	/*
		if (TIMEOUT(CycleEndPacketTime, 2)) // if no Cycle End Packet receive over 2s
			return 2;
	*/
	return ret;
}

int CS203DC_SetPower(CS203DC_Handle *handle, int power)
{
	int rtn = 0;
	int err;
	unsigned char cmd[DATABUFSIZE];

	if (handle == NULL)
		return -1;

	if (power < 0 || power > 300)
		return -2;

	if (handle->state != CS203DCSTATE_CONNECTED)
		return -3;

	sprintf(cmd, "70010607%02X%02X0000", power & 0x000000FF, (power & 0x0000FF00) >> 8);
	DB(syslog(LOG_ERR, "%s(): line %d, cmd: %s", __FUNCTION__, __LINE__, cmd));

	if ((err = _sendintelcmd(handle, CS203DC_HSTANTDESCSEL)) < 0)
	{
		syslog(LOG_ERR, "%s(): line %d, cannot send CS203DC_HSTANTDESCSEL, err %d", __FUNCTION__, __LINE__, err);
		return err;
	}
	else if ((err = _sendintelcmd(handle, cmd)) < 0)
	{
		syslog(LOG_ERR, "%s(): line %d, cannot set rf power, err %d", __FUNCTION__, __LINE__, err);
		return err;
	}

	return rtn;
}

int RFID_CheckOEM(CS203DC_Handle *handle)
{
	int ret = 0;
	int buf1_datalen = 0;
	unsigned char buf1[DATABUFSIZE];
	unsigned char buf2[DATABUFSIZE];
	int nrecv;
	int i;

	if (handle == NULL)
	{
		printf("RFID_CheckOEM: NULL handle");
		return -1;
	}
	if (handle->state != CS203DCSTATE_CONNECTED)
	{
		printf("RFID_CheckOEM: handle->state is not CS203DCSTATE_CONNECTED");
		return -3;
	}

	printf("Checking Reader Model .... \n");
	// read OEM Country code from reader
	if (ret == 0)
	{
		buf1_datalen = _strHex2ByteArray(CS203DC_OEMADDR_02, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		buf1_datalen = _strHex2ByteArray(CS203DC_HST_READ_OEM, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		ret = rfid_reader_read(&handle->rfid_reader_context, buf2, 48);
		nrecv = 48;

		if (nrecv)
		{
			handle->OEM_country = buf2[28]; // Get OEM Country Code
		}
		ClearRecvBuffer(handle); // Clear Command-End Packet
	}
	printf("RFID_CheckOEM: rfid_reader_read/write error: ret = %i", ret);
	// read OEM Country Version from reader
	if (ret == 0)
	{
		buf1_datalen = _strHex2ByteArray(CS203DC_OEMADDR_8E, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		buf1_datalen = _strHex2ByteArray(CS203DC_HST_READ_OEM, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		ret = rfid_reader_read(&handle->rfid_reader_context, buf2, 48);
		nrecv = 48;

		if (0)
		{
			int x;
			printf("OEM data: (");
			for (x = 0; x < 40; x++)
				printf("%02X", buf2[x]);
			printf(")\n");
		}
		if (nrecv)
		{
			int x;
			for (x = 0; x < 4; x++)
				handle->OEM_countryversion[x] = buf2[31 - x];
			handle->OEM_countryversion[4] = 0;
		}
		ClearRecvBuffer(handle); // Clear Command-End Packet
	}
	// read OEM Frequency Modification Flag from reader
	if (ret == 0)
	{
		buf1_datalen = _strHex2ByteArray(CS203DC_OEMADDR_8F, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		buf1_datalen = _strHex2ByteArray(CS203DC_HST_READ_OEM, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		ret = rfid_reader_read(&handle->rfid_reader_context, buf2, 48);
		nrecv = 48;

		if (0)
		{
			int x;
			printf("OEM data: (");
			for (x = 0; x < 40; x++)
				printf("%02X", buf2[x]);
			printf(")\n");
		}
		if (nrecv)
		{
			handle->OEM_frequencyModification = buf2[28]; // Get OEM Country Code
		}
		ClearRecvBuffer(handle); // Clear Command-End Packet
	}
	// read OEM Model Code from reader
	if (ret == 0)
	{
		buf1_datalen = _strHex2ByteArray(CS203DC_OEMADDR_A4, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		buf1_datalen = _strHex2ByteArray(CS203DC_HST_READ_OEM, buf1);
		ret = rfid_reader_write(&handle->rfid_reader_context, buf1, buf1_datalen);
		ret = rfid_reader_read(&handle->rfid_reader_context, buf2, 48);
		nrecv = 48;

		if (nrecv)
		{
			handle->OEM_model = buf2[28]; // Get OEM Model Code
		}
		ClearRecvBuffer(handle); // Clear Command-End Packet
	}
	// Display Reader Model with country code
	if (ret == 0)
	{
		printf("%s : ", handle->target_reader);
		switch (handle->OEM_model)
		{
		case 0:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS101_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 1:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS203_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 2:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS333_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 3:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS468_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 5:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS468INT_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 6:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS463_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 7:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS469_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 8:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS208_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 9:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS209_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 0x0A:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS103_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 0x0B:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS108_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 0x0C:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS206_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 0x0D:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS468X_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 0x0E:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS203X_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		case 0x0F:
			printf("Reader Model: %s%s-%d", READER_MODEL_CS468XJ_TEXT, handle->OEM_countryversion, handle->OEM_country);
			break;
		default:
			printf("Reader Model: %d(unknown model)%s-%d", handle->OEM_model, handle->OEM_countryversion, handle->OEM_country);
			ret = -17;
			break;
		}
		switch (handle->OEM_country)
		{
		case 1:
			printf("(ETSI)\n");
			break;
		case 2:
			printf("(FCC)\n");
			break;
		case 4:
			printf("(Taiwan)\n");
			break;
		case 6:
			printf("(Korea)\n");
			break;
		case 7:
			printf("(China)\n");
			break;
		case 8:
			printf("(Japan)\n");
			break;
		case 9:
			printf("(ETSI upper band)\n");
			break;
		default:
			printf("(unknown model)\n");
			break;
			break;
		}
	}

	if (ret == 0)
	{
		int t_num = 0;
		const CS203DC_COUNTRY const *t_list;

		printf("%s : Frequency Modifier: %X", handle->target_reader, handle->OEM_frequencyModification);
		if (handle->OEM_frequencyModification == 0xAA)
			printf(": No Frequency Modification (country setting) is allowed in this model.");
		printf("\n");
		switch (handle->OEM_country)
		{
		case 1:
		case 2:
		case 4:
		case 6:
		case 7:
		case 8:
		case 9:
			if (handle->OEM_frequencyModification == 0xAA)
				break;
			t_num = OEM_Country_N_List[handle->OEM_country].country_count; // number of country support
			t_list = OEM_Country_N_List[handle->OEM_country].country_list;

			// printf("OEM_Country_N_List 1 : %d \n", t_num);
			// printf("OEM_Country_N_List 2 : %d \n", t_list[14]);

			for (i = 0; i < t_num; i++)
			{
				if (handle->country == t_list[i])
				{
					printf(" %s : Set Frequency Table for %s \n", handle->target_reader, CountryTxt[handle->country]);
					if (handle->OEM_country == 1 || handle->OEM_country == 8)
					{
						char sPacketCommand[50];
						int i = 0, iFixed = handle->iConf_channel;
						unsigned int t_freqVal;

						if (iFixed < countryFreqTable[handle->country].freq_count)
						{
							sprintf(sPacketCommand, "7001010C%02X000000", i); // Select Channel
							_sendintelcmd(handle, sPacketCommand);

							// printf("freqTable %d, %08X \n", i, freqTable[i]);
							t_freqVal = swapMSBLSB32bit(countryFreqTable[handle->country].freq_table[iFixed]);
							// printf("t_freqVal %08X \n", t_freqVal);

							sprintf(sPacketCommand, "7001030C%08X", t_freqVal); // Set Freq
							_sendintelcmd(handle, sPacketCommand);

							sprintf(sPacketCommand, "7001020C01000000"); // Enable Channel
							_sendintelcmd(handle, sPacketCommand);
						}
					}
					else
						SetFreq_Table(handle, countryFreqTable[handle->country].freq_table, countryFreqTable[handle->country].freq_count);
					break;
				}
			}
			// Country not found in the list OEM_Country_N_List
			if (i == t_num)
			{
				printf(" %s : country choose %s \n", handle->target_reader, CountryTxt[handle->country]);
				printf("The reader with N = -%d does not support selected country!! \n", handle->OEM_country);
				ret = -18;
			}
			break;

		default:
			printf("The OEM country code N of the reader is not identified!! \n");
			ret = -18;
			break;
		}
	}

	return ret;
}

int RFID_SetDwell(CS203DC_Handle *handle, unsigned int dwell)
{
	int rtn = 0;
	int err;
	unsigned char cmd[DATABUFSIZE];

	if (handle == NULL)
		return -1;

	if (handle->state != CS203DCSTATE_CONNECTED)
		return -3;

	// ANT_PORT_DWELL
	sprintf(cmd, "70010507%02X%02X%02X%02X", (unsigned char)(dwell & 0xff), (unsigned char)((dwell >> 8) & 0xff), (unsigned char)((dwell >> 16) & 0xff), (unsigned char)((dwell >> 24) & 0xff));
	DB(syslog(LOG_ERR, "%s(): line %d, cmd: %s", __FUNCTION__, __LINE__, cmd));

	if ((err = _sendintelcmd(handle, CS203DC_HSTANTDESCSEL)) < 0)
	{
		syslog(LOG_ERR, "%s(): line %d, cannot send CS203DC_HSTANTDESCSEL, err %d", __FUNCTION__, __LINE__, err);
		return err;
	}

	else if ((err = _sendintelcmd(handle, cmd)) < 0)
	{
		syslog(LOG_ERR, "%s(): line %d, cannot set rf power, err %d", __FUNCTION__, __LINE__, err);
		return err;
	}

	return rtn;
}

/*
	return 1 = Success Get Data
	return -1 = No Data
	return 0 = 0 Byte Data / Socket Disconnect
*/
int readtag(CS203DC_Handle *handle)
{
	int nprocessed;
	int macerr;
	int nserialused;
	int nrecv;
	int err;
	//	char fromLanBuff[1600];
	//	char toSerialBuff[1600];
	char fromLanBuff[2048];
	char toSerialBuff[2048];

	time_t toc = time(NULL);
	if ((toc - handle->tic) != 0)
	{
		if (handle->tic != 0)
		{
			handle->eTime++;
			printf("Unique Tag # = %d; Tag Rate = %d; New Unique Tag Rate = %d; Elapsed Time %d\n", handle->uTag, handle->numTag, handle->newTag, handle->eTime);
		}
		handle->tic = toc;
		handle->numTag = 0;
		handle->newTag = 0;
	}

	if (handle->flag_decodePacket)
	{
		// printf("\n readtag->recv : %d \n", handle->decodePacketBuf_offset);
		nrecv = handle->flag_decodePacket - handle->decodePacketBuf_offset;
		err = rfid_reader_read(&handle->rfid_reader_context, fromLanBuff + handle->decodePacketBuf_offset, nrecv);
	}
	else
	{
		err = rfid_reader_read(&handle->rfid_reader_context, fromLanBuff, 8);
		nrecv = 8;
	}
	if (err)
		return 0;

	// Unique Tag # = 0; Tag Rate = 0; New Unique Tag Rate = 0; Elapsed Time 1
	if (nrecv > 0)
	{
		/*struct in_addr in;

		printcurtime ();
		in.s_addr = handle->ipv4addr;
		printf ("IP:%s receive %d Bytes, ", inet_ntoa(in), nrecv);*/
		if (handle->flag_decodePacket)
			err = CS203DC_DecodePacket(handle, fromLanBuff, nrecv + handle->decodePacketBuf_offset, &nprocessed, toSerialBuff, sizeof(toSerialBuff), &nserialused, &macerr);
		else
			err = CS203DC_DecodePacket(handle, fromLanBuff, nrecv, &nprocessed, toSerialBuff, sizeof(toSerialBuff), &nserialused, &macerr);

		/*
				if(nprocessed == 36)
					printf ("%s\n", toSerialBuff);
				else
					printf ("\n nprocessed : %d \n", nprocessed);
		*/
		if (*toSerialBuff)
			if (handle->simpledisplay == 0)
				printf("%s\n", toSerialBuff);

		if (err == 2)
			return 2;

		return 1;
	}
	else if (nrecv != -1)
	{
		return 0;
	}

	return -1;
}

void CheckLocalIP()
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[NI_MAXHOST];
	// int ret = 0;

	if (getifaddrs(&ifaddr) == -1)
	{
		printf("getifaddrs\n");
		return;
	}

	/* Walk through linked list, maintaining head pointer so we can free list later */

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		// if (strcmp (ifa->ifa_name, "eth0") == 0 && family == AF_INET)
		if (family == AF_INET)
		{
			s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (s != 0)
			{
				printf("if = %s, %d,\n", ifa->ifa_name, ifa->ifa_addr->sa_family);
			}
			else
			{
				printf("if = %s, %d, %s\n", ifa->ifa_name, ifa->ifa_addr->sa_family, host);
			}
		}
		else
			printf("if = %s, %d\n", ifa->ifa_name, ifa->ifa_addr->sa_family);
	}

	freeifaddrs(ifaddr);

	// return (ret);
}

CS203DC_Handle *handler_alloc()
{
	CS203DC_Handle *fd;

	fd = (CS203DC_Handle *)malloc(sizeof(CS203DC_Handle));

	if (fd != NULL)
	{
		// fd->ipv4addr = 0;
		fd->state = CS203DCSTATE_IDLE;
		// fd->intelcmdfd = 0;
		// fd->controlcmdfd = 0;
		fd->iConf_ReadTag = 0;
		fd->iConf_Forever = 0;
		fd->iConf_WriteUser = 0;
		fd->NextHandle = NULL;
		fd->country = CTY_FCC;
		fd->iConf_channel = 1;
	}
	else
	{
		printf("Insufficient memory, can not allocate handler!\n");
	}

	return fd;
}

int read_country_code(CS203DC_Handle *handle, char *buf)
{
	int i;

	for (i = 0; i < NUM_COUNTRIES; i++)
	{
		if (strncmp(buf, CountryTxt[i], strlen(CountryTxt[i])) == 0)
		{
			handle->country = i; // i = country code in CS203DC_COUNTRY;
			printf("(Country Code : %d, ", handle->country);
			break;
		}
	}

	if (i == NUM_COUNTRIES)
	{
		printf("Country Code Error!!");
		return -1;
	}

	printf("Country : %s)", CountryTxt[handle->country]);
	return 0;
}

int CS_MultiPortconf(char *filename, CS203DC_Handle *handle)
{
	FILE *fin;
	char cvsbuf[12][20];
	int cnt = 0;

	fin = fopen(filename, "rt");
	if (fin == NULL)
	{
		printf("%s : Can not open port config file '%s'\n", handle->target_reader, filename);
		return -1;
	}

	while (fscanf(fin, "%s %s %s %s %s %s %s %s %s %s %s %s\n", cvsbuf[0], cvsbuf[1], cvsbuf[2], cvsbuf[3], cvsbuf[4], cvsbuf[5], cvsbuf[6], cvsbuf[7], cvsbuf[8], cvsbuf[9], cvsbuf[10], cvsbuf[11]) != EOF)
	{
		if (strstr(cvsbuf[0], "PortNumber"))
		{
			printf("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", cvsbuf[0], cvsbuf[1], cvsbuf[2], cvsbuf[3], cvsbuf[4], cvsbuf[5], cvsbuf[6], cvsbuf[7], cvsbuf[8], cvsbuf[9], cvsbuf[10], cvsbuf[11]);
			continue; // skipping the header line of the file
		}

		int port = atoi(cvsbuf[0]);
		if (port < 0 || port > 15)
		{
			return -1;
		}
		handle->PortConfig[port].Active = strcasecmp(cvsbuf[1], "enable") == 0;
		handle->PortConfig[port].Power = atol(cvsbuf[2]);
		handle->PortConfig[port].Dwell = atol(cvsbuf[3]);
		handle->PortConfig[port].InventoryRounds = atol(cvsbuf[4]);
		handle->PortConfig[port].LocalInventory = strcasecmp(cvsbuf[5], "enable") == 0;
		handle->PortConfig[port].Algorithm = atol(cvsbuf[6]);
		handle->PortConfig[port].StartQ = atol(cvsbuf[7]);
		handle->PortConfig[port].LocalProfile = strcasecmp(cvsbuf[8], "enable") == 0;
		handle->PortConfig[port].Profile = atol(cvsbuf[9]);
		handle->PortConfig[port].LocalChannel = strcasecmp(cvsbuf[10], "enable") == 0;
		handle->PortConfig[port].Channel = atol(cvsbuf[11]);

		// Generate sequence
		if (handle->PortConfig[port].Active)
		{
			handle->portSequence[cnt] = port;
			cnt++;
		}

		printf("%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n", port, handle->PortConfig[port].Active, handle->PortConfig[port].Power, handle->PortConfig[port].Dwell,
			   handle->PortConfig[port].InventoryRounds, handle->PortConfig[port].LocalInventory, handle->PortConfig[port].Algorithm, handle->PortConfig[port].StartQ,
			   handle->PortConfig[port].LocalProfile, handle->PortConfig[port].Profile, handle->PortConfig[port].LocalChannel, handle->PortConfig[port].Channel);
	}
	handle->portSequenceCount = cnt;

	fclose(fin);

	return 0;
}

int config_read0(char *linebuf, int isFileLine, CS203DC_Handle *CS203fd)
{
	char *spacepos, *spacepos1;
	char filename[60];

	*filename = 0;
	if (memcmp(CS203fd->target_reader, "USB", 3) != 0)
	{
		// set default values
		CS203fd->profile = 1; // 20161025
		CS203fd->power = 300;
		CS203fd->dwellTime = 2000; // Add Dwell Time 2014-05-15
		CS203fd->num_dBank = 0;	   // Number of data bank after inventory  - 2015-08-21
		CS203fd->tid_length = 0;   // TID Data Length  - 2015-08-10
		CS203fd->user_length = 0;  // User Data Length  - 2015-08-21
		CS203fd->multiport = 0;
		CS203fd->query_session = 0;
		CS203fd->query_target = 0;
		CS203fd->toggle = 1;
		CS203fd->sequenceMode = 0;
		CS203fd->Algorithm = 1; // dynamicQ
		CS203fd->MaxQ = 15;
		CS203fd->StartQ = 7;
		CS203fd->MinQ = 0;
		CS203fd->ThreadholdMultipler = 4;
		// new parameters
		CS203fd->inventorymode = 0;
		CS203fd->tagdelaycompactmode = 7;
		CS203fd->retry = 0;
		CS203fd->tagfocus = 0;
		CS203fd->rflnagain = 1;
		CS203fd->rfhighcompress = 1;
		CS203fd->iflnagain = 24;
		CS203fd->agcgain = -6;
		CS203fd->simpledisplay = 0;

		strncpy(CS203fd->target_reader, "USB", 50);
	}

	if (isFileLine == 0)
		printf("Argument");
	else
		printf("Config");
	printf(" read1 :");

	spacepos = strchr(linebuf, '-');
	if (spacepos != NULL)
		spacepos--;
	while (spacepos != NULL)
	{
		spacepos++;
		if ((isFileLine == 0) && (memcmp(spacepos, "-conf", 5) == 0))
		{
			printf(" -conf");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			strncpy(CS203fd->configFile, spacepos, 50 - 1);
			printf("(%s)", CS203fd->configFile);
			break;
		}
		else if (memcmp(spacepos, "-tag ", 5) == 0)
		{
			printf(" -tag");
			CS203fd->iConf_ReadTag = 1;
		}
		else if (memcmp(spacepos, "-loop", 5) == 0)
		{
			printf(" -loop");
			CS203fd->iConf_Forever = 1; // Test Forever
		}
		else if (memcmp(spacepos, "-write", 5) == 0)
		{
			printf(" -write");
			CS203fd->iConf_WriteUser = 1; // Test Forever
		}
		else if (memcmp(spacepos, "-profile", 8) == 0)
		{
			printf(" -profile");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			int profile;
			profile = atoi(spacepos);
			CS203fd->profile = profile;
		}
		else if (memcmp(spacepos, "-power", 6) == 0)
		{
			printf(" -power");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			int power;
			power = atoi(spacepos);
			CS203fd->power = power;
		}
		else if (memcmp(spacepos, "-dwellTime", 10) == 0)
		{
			printf(" -dwellTime");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			int dwellTime;
			dwellTime = atoi(spacepos);
			CS203fd->dwellTime = dwellTime;
		}
		else if (memcmp(spacepos, "-tid_length", 11) == 0)
		{
			printf(" -tid_length");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			int tid_length;
			tid_length = atoi(spacepos);
			CS203fd->tid_length = tid_length;

			// printf("%s CS203fd->tid_length : %d \n", CS203fd->ReaderHostName, CS203fd->tid_length);
		}
		else if (memcmp(spacepos, "-user_length", 12) == 0)
		{
			printf(" -user_length");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			int user_length;
			user_length = atoi(spacepos);
			CS203fd->user_length = user_length;

			// printf("%s CS203fd->user_length : %d \n", CS203fd->ReaderHostName, CS203fd->user_length);
		}
		else if (memcmp(spacepos, "-portconf", 9) == 0)
		{
			printf(" -portconf");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			char *spacepos1;
			int substringLength;
			spacepos1 = strchr(spacepos, ' ');
			if (spacepos1 == NULL)
				substringLength = strlen(spacepos);
			else
				substringLength = spacepos1 - spacepos;

			memcpy(filename, spacepos, substringLength);
			filename[substringLength] = '\0';

			//	printf("\n%s Portconf : %s \n", CS203fd->target_reader, filename);
			//	CS_MultiPortconf (filename,CS203fd);
			CS203fd->multiport = 1;
		}
		else if (memcmp(spacepos, "-sequence", 9) == 0)
		{
			printf(" -sequence");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->sequenceMode = atoi(spacepos);

			// printf("%s CS203fd->sequenceMode : %d \n", CS203fd->ReaderHostName, CS203fd->sequenceMode);
		}
		else if (memcmp(spacepos, "-country", 8) == 0)
		{
			printf(" -country");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			read_country_code(CS203fd, spacepos);
		}
		else if (memcmp(spacepos, "-freq_ch_no", 11) == 0)
		{
			printf(" -freq_ch_no");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->iConf_channel = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-session", 8) == 0)
		{
			printf(" -session");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->query_session = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-target", 7) == 0)
		{
			printf(" -target");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->query_target = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-toggle", 7) == 0)
		{
			printf(" -toggle");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->toggle = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-algorithm", 10) == 0)
		{
			printf(" -algorithm");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			char buf1[100];
			strncpy(buf1, spacepos, 100 - 1);
			char *pch;
			pch = strtok(buf1, ",");
			if (atoi(pch) == 0) // fixedQ
			{
				CS203fd->Algorithm = 0;
				CS203fd->QValue = atoi(strtok(NULL, ","));
			}
			else // dynamicQ
			{
				CS203fd->Algorithm = 1;
				CS203fd->StartQ = atoi(strtok(NULL, ","));
				CS203fd->MinQ = atoi(strtok(NULL, ","));
				CS203fd->MaxQ = atoi(strtok(NULL, ","));
				CS203fd->ThreadholdMultipler = atoi(strtok(NULL, ","));
			}
		}

		// new parameters
		else if (memcmp(spacepos, "-inventorymode", 14) == 0)
		{
			printf(" -inventorymode");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			if (memcmp(spacepos, "normal", 6) == 0)
				CS203fd->inventorymode = 0;
			else if (memcmp(spacepos, "compact", 7) == 0)
				CS203fd->inventorymode = 1;
		}
		else if (memcmp(spacepos, "-tagdelaycompactmode", 20) == 0)
		{
			printf(" -tagdelaycompactmode");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->tagdelaycompactmode = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-retry", 6) == 0)
		{
			printf(" -retry");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->retry = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-tagfocus", 9) == 0)
		{
			printf(" -tagfocus");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->tagfocus = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-rflnagain", 10) == 0)
		{
			printf(" -rflnagain");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->rflnagain = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-rfhighcompress", 15) == 0)
		{
			printf(" -rfhighcompress");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->rfhighcompress = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-iflnagain", 10) == 0)
		{
			printf(" -iflnagain");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->iflnagain = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-agcgain", 8) == 0)
		{
			printf(" -agcgain");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->agcgain = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-simpledisplay", 14) == 0)
		{
			printf(" -simpledisplay");
			spacepos = strchr(spacepos, ' ');
			spacepos++;
			CS203fd->simpledisplay = atoi(spacepos);
		}
		else if (memcmp(spacepos, "-portn", 6) == 0)
		{
			printf(" -portn");
			spacepos = strchr(spacepos, ' ');
			spacepos++;

			char buf1[100];
			strncpy(buf1, spacepos, 100 - 1);
			int portn = atoi(strtok(buf1, ","));
			if (portn >= 0 && portn <= 15)
			{
				CS203fd->multiport = 1;
				CS203fd->PortConfig[portn].Active = strcasecmp(strtok(NULL, ","), "enable") == 0;
				CS203fd->PortConfig[portn].Power = atol(strtok(NULL, ","));
				CS203fd->PortConfig[portn].Dwell = atol(strtok(NULL, ","));
				CS203fd->PortConfig[portn].InventoryRounds = atol(strtok(NULL, ","));
				CS203fd->PortConfig[portn].LocalInventory = strcasecmp(strtok(NULL, ","), "enable") == 0;
				CS203fd->PortConfig[portn].Algorithm = strcasecmp(strtok(NULL, ","), "DynamicQ") == 0;
				CS203fd->PortConfig[portn].StartQ = atol(strtok(NULL, ","));
				CS203fd->PortConfig[portn].LocalProfile = strcasecmp(strtok(NULL, ","), "enable") == 0;
				CS203fd->PortConfig[portn].Profile = atol(strtok(NULL, ","));
				CS203fd->PortConfig[portn].LocalChannel = strcasecmp(strtok(NULL, ","), "enable") == 0;
				CS203fd->PortConfig[portn].Channel = atol(strtok(NULL, ","));
				printf("(%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld)",
					   portn,
					   CS203fd->PortConfig[portn].Active,
					   CS203fd->PortConfig[portn].Power,
					   CS203fd->PortConfig[portn].Dwell,
					   CS203fd->PortConfig[portn].InventoryRounds,
					   CS203fd->PortConfig[portn].LocalInventory,
					   CS203fd->PortConfig[portn].Algorithm,
					   CS203fd->PortConfig[portn].StartQ,
					   CS203fd->PortConfig[portn].LocalProfile,
					   CS203fd->PortConfig[portn].Profile,
					   CS203fd->PortConfig[portn].LocalChannel,
					   CS203fd->PortConfig[portn].Channel);
			}
		}
		spacepos = strchr(spacepos, ' ');
	}
	printf("\n");
	if (CS203fd->multiport != 0 && filename[0] != 0)
	{
		printf("\n%s Portconf : %s \n", CS203fd->target_reader, filename);
		CS_MultiPortconf(filename, CS203fd);
	}
}

int conf_Init(char *confFile, CS203DC_Handle *CS203fd_first)
{
	CS203DC_Handle *CS203fd;
	int linecnt = 0;
	FILE *fin;
	char linebuf0[200], linebuf[5000], linebuf1[5000];
	int linebuf_index = 0;
	char *spacepos;

	int ac;
	char *av;

	fin = fopen(confFile, "rt");
	if (fin == NULL)
	{
		printf("Can not open port config file '%s'\n", confFile);
		return -1;
	}

	printf("\n", linebuf);
	while (fscanf(fin, "%[^\n]\n", linebuf0) != EOF)
	{
		spacepos = strchr(linebuf0, '\r');
		if (spacepos != NULL)
			*spacepos = 0;
		linebuf[linebuf_index++] = ' ';
		strncpy(linebuf + linebuf_index, linebuf0, strlen(linebuf0));
		linebuf_index += strlen(linebuf0);
	}
	linebuf[linebuf_index] = 0;
	{
		int x, y;
		for (x = 0, y = 0; x < sizeof(linebuf); x++)
		{
			if ((linebuf[x] != ' ') || ((linebuf1[y - 1] != ' ') && y != 0))
				linebuf1[y++] = linebuf[x];
		}
		printf("Conf input : %s\n", linebuf1);

		linecnt++;
		if (linecnt == 1)
		{
			CS203fd = CS203fd_first;
		}
		else
		{
			CS203fd->NextHandle = handler_alloc();
			if (CS203fd->NextHandle == NULL)
				return -1;
			CS203fd = CS203fd->NextHandle;
		}
		config_read0(linebuf1, 1, CS203fd);
	}

	fclose(fin);

	return 0;
}

int CS468_SetPort(CS203DC_Handle *handle)
{
	unsigned char sPacketCommand[32];
	unsigned char buf2[10];
	unsigned int buf;
	unsigned int buf3;
	int nrecv;
	int cnt, cnt2, nread;
	int ret, err;
	unsigned char sRecvBuffer[1024];

	// global variables
	//  CURRENT_PROFILE
	/*sprintf(sPacketCommand, CS203DC_CURRENT_PROFILE);
	_sendintelcmd(handle, sPacketCommand);*/
	printf("%s : Set profile to %d \n", handle->target_reader, handle->profile);
	WriteRegister(handle, 0x0B60, handle->profile); // HST_RFTC_CURRENT_PROFILE
	sprintf(sPacketCommand, CS203DC_HST_CHG_PROFILE);
	_sendintelcmd(handle, sPacketCommand);
	ClearRecvBuffer(handle); // Clear Command-End Packet

	printf("%s : Set bypass rfhighcompress to %d, rflnagain to %d, iflnagain to %d, agcgain to %d\n", handle->target_reader, handle->rfhighcompress, handle->rflnagain, handle->iflnagain, handle->agcgain);
	WriteRegister(handle, 0x0400, 0x0450);
	buf = ((handle->rfhighcompress & 0x01) << 8);
	switch (handle->rflnagain)
	{
	case 1:
		break;
	case 7:
		buf |= 0x80;
		break;
	case 13:
		buf |= 0xC0;
		break;
	}
	switch (handle->iflnagain)
	{
	case 24:
		break;
	case 18:
		buf |= 0x08;
		break;
	case 12:
		buf |= 0x18;
		break;
	case 6:
		buf |= 0x38;
		break;
	}
	switch (handle->agcgain)
	{
	case -12:
		break;
	case -6:
		buf |= 4;
		break;
	case 0:
		buf |= 6;
		break;
	case 6:
		buf |= 7;
		break;
	}
	WriteRegister(handle, 0x0401, buf);
	sprintf(sPacketCommand, CS203DC_HST_CHG_MBP);
	_sendintelcmd(handle, sPacketCommand);
	ClearRecvBuffer(handle); // Clear Command-End Packet

	// QUERY_CFG
	printf("%s : Set query target to %d, query session to %d \n", handle->target_reader, handle->query_target, handle->query_session);
	buf = (handle->query_target << 4 & 0x10) | (handle->query_session << 5 & 0x60);
	sprintf(sPacketCommand, "70010009%02X%02X%02X%02X", (unsigned char)(buf & 0xff), (unsigned char)(buf >> 8 & 0xff), (unsigned char)(buf >> 16 & 0xff), (unsigned char)(buf >> 24 & 0xff));
	_sendintelcmd(handle, sPacketCommand);
	// Set Inventory Algorithm
	//  INV_CFG
	///////////////////////////////////////////////////////////////////////////////////////////
	//	sprintf(sPacketCommand, "70010109%02X00%02X00",handle->Algorithm ? 3 : 0, 1);    // INV_CFG - tag_read = 1 (P.37)
	//	_sendintelcmd(handle, sPacketCommand);
	///////////////////////////////////////////////////////////////////////////////////////////

	int t_acc_bank = 0;
	int t_num_dBank = 0;
	int t_inv_mode = 0;
	if (handle->tid_length) // TID Data Bank
	{
		t_acc_bank |= 2;
		t_num_dBank++;
	}
	if (handle->user_length) // User Data Bank
	{
		t_acc_bank |= 3 << 2;
		t_num_dBank++;
	}
	handle->num_dBank = t_num_dBank;
	if (handle->inventorymode)
		t_inv_mode = 0x04;
	if (handle->tagdelaycompactmode < 32)
	{
		t_num_dBank |= ((handle->tagdelaycompactmode & 0x0F) << 4);
		t_inv_mode += ((handle->tagdelaycompactmode & 0x30) >> 4);
	}

	printf("%s : Set query Algorithm to %d, num_dBank = %d, tagdelaycompactmode %d, inv_mode = %d\n", handle->target_reader, handle->Algorithm, t_num_dBank, handle->tagdelaycompactmode, handle->inventorymode);
	sprintf(sPacketCommand, "70010109%02X00%02X%02X", handle->Algorithm ? 3 : 0, t_num_dBank, t_inv_mode); // INV_CFG - tag_read = 0/1/2 (P.37)
	_sendintelcmd(handle, sPacketCommand);

	printf("%s : Set query acc_bank to %d with offset = 0\n", handle->target_reader, t_acc_bank);
	sprintf(sPacketCommand, "7001020A%02X000000", t_acc_bank); // TAGACC_BANK - include TID bank / User bank data (P.41)
	_sendintelcmd(handle, sPacketCommand);
	sprintf(sPacketCommand, "7001030A%02X000000", 0); // TAGACC_PTR - Specify the offset (16 bit words) in tag memory for tag accesses (P.41)
	_sendintelcmd(handle, sPacketCommand);

	printf("%s : Set acc_length with tid_length to %d, user_length to %d with password = 0\n", handle->target_reader, handle->tid_length, handle->user_length);
	//	sprintf(sPacketCommand, "7001040A02000000");							// TAGACC_CNT - specify the number of 16 bit words that should be accessed when issuing read or write commands.   (P.41)
	sprintf(sPacketCommand, "7001040A%02X%02X0000", handle->tid_length, handle->user_length); // TAGACC_CNT - Read N Words for TID bank, User Bank   (P.41)
	_sendintelcmd(handle, sPacketCommand);
	sprintf(sPacketCommand, "7001060A00000000"); // TAGACC_ACCPWD
	_sendintelcmd(handle, sPacketCommand);

	////////////////////////////////////////////////////////////////////////////////////////////
	printf("%s : Set Algorithm to %d, StartQ to %d, MaxQ to %d, MinQ to %d, ThreadholdMultipler to %d, toggle to %d\n", handle->target_reader, handle->Algorithm, handle->StartQ, handle->MaxQ, handle->MinQ, handle->ThreadholdMultipler, handle->toggle);
	// INV_SEL (FixedQ/DynamicQ)
	sprintf(sPacketCommand, "70010209%02X000000", handle->Algorithm ? 3 : 0);
	_sendintelcmd(handle, sPacketCommand);
	// INV_ALG_PARM_0 (Algorithm parameters)
	buf = (handle->StartQ & 0xF) | (handle->MaxQ << 4 & 0xF0) | (handle->MinQ << 8 & 0xF00) | (handle->ThreadholdMultipler << 12 & 0xF000);
	sprintf(sPacketCommand, "70010309%02X%02X%02X%02X", (unsigned char)(buf & 0xff), (unsigned char)(buf >> 8 & 0xff), (unsigned char)(buf >> 16 & 0xff), (unsigned char)(buf >> 24 & 0xff));
	_sendintelcmd(handle, sPacketCommand);
	//	INV_ALG_PARM_2 (toggle)
	if (handle->query_session == 0)
		sprintf(sPacketCommand, "70010509%02X000000", handle->toggle ? 3 : 2); // INV_ALG_PARM_2
	else
		sprintf(sPacketCommand, "70010509%02X000000", handle->toggle ? 1 : 0); // INV_ALG_PARM_2
	_sendintelcmd(handle, sPacketCommand);

	printf("%s : Set Retry to %d\n", handle->target_reader, handle->retry);
	sprintf(sPacketCommand, "70010409%02X000000", (handle->retry & 0xFF));
	_sendintelcmd(handle, sPacketCommand);

	printf("%s : Set focus to %d\n", handle->target_reader, handle->tagfocus);
	sprintf(sPacketCommand, "70010302%02X000000", ((handle->tagfocus & 0x01) << 4));
	_sendintelcmd(handle, sPacketCommand);

	printf("%s : multiport = %d \n", handle->target_reader, handle->multiport);
	if (handle->multiport)
	{
		// local settings for inidividual ports
		for (cnt = 0; cnt < 16; cnt++)
		{
			printf(" %s : ANT_PORT_CFG %d\n", handle->target_reader, cnt);
			// ANT_PORT_SEL
			sprintf(sPacketCommand, "70010107%02X000000", (unsigned char)(cnt));
			_sendintelcmd(handle, sPacketCommand);

			// ANT_PORT_CFG
			buf = ((handle->PortConfig[cnt].Active != 0) ? 0x00000001 : 0) |
				  ((handle->PortConfig[cnt].LocalInventory != 0) ? 0x00000002 : 0) |
				  ((handle->PortConfig[cnt].Algorithm & 0x03) << 2) |
				  ((handle->PortConfig[cnt].StartQ & 0x0f) << 4) |
				  ((handle->PortConfig[cnt].LocalProfile != 0) ? 0x00000100 : 0) |
				  ((handle->PortConfig[cnt].Profile & 15) << 9) |
				  ((handle->PortConfig[cnt].LocalChannel != 0) ? 0x00002000 : 0) |
				  ((handle->PortConfig[cnt].Channel & 63) << 14);
			sprintf(sPacketCommand, "70010207%02X%02X%02X%02X", (unsigned char)(buf & 0xff), (unsigned char)(buf >> 8 & 0xff), (unsigned char)(buf >> 16 & 0xff), (unsigned char)(buf >> 24 & 0xff));
			_sendintelcmd(handle, sPacketCommand);

			// ANT_PORT_DWELL
			sprintf(sPacketCommand, "70010507%02X%02X%02X%02X", (unsigned char)(handle->PortConfig[cnt].Dwell), (unsigned char)(handle->PortConfig[cnt].Dwell >> 8), (unsigned char)(handle->PortConfig[cnt].Dwell >> 16), (unsigned char)(handle->PortConfig[cnt].Dwell >> 24));
			_sendintelcmd(handle, sPacketCommand);

			// ANT_PORT_POWER
			sprintf(sPacketCommand, "70010607%02X%02X0000", (unsigned char)(handle->PortConfig[cnt].Power), (unsigned char)(handle->PortConfig[cnt].Power >> 8));
			_sendintelcmd(handle, sPacketCommand);

			// ANT_PORT_INV_CNT (hardcode to 0 - run infinitely)
			sprintf(sPacketCommand, "7001070700000000");
			_sendintelcmd(handle, sPacketCommand);
		}
	}

	// ANT_CYCLES (switching ports that are enabled at random pattern if sequence mode is selected)

	// set either normal mode or sequence mode
	buf = 0;
	if (handle->sequenceMode)
	{
		buf |= (handle->portSequenceCount << 18 & 0xFC0000);
		buf |= (1 << 16);
	}
	else
		buf = 0;

	printf("%s : Set Antenna cycle to FFFF\n", handle->target_reader);
	sprintf(sPacketCommand, "70010007FFFF%02X%02X", (unsigned char)(buf >> 16 & 0xff), (unsigned char)(buf >> 24 & 0xff));
	_sendintelcmd(handle, sPacketCommand);

	//	fcntl(handle->intelcmdfd, F_SETFL, fcntl(handle->intelcmdfd,F_GETFL,0)&(~O_NONBLOCK));

	printf("%s : handle->sequenceMode : %d \n", handle->target_reader, handle->sequenceMode);
	// modify OEM table
	if (handle->sequenceMode)
	{
		buf = 0;
		buf3 = 0;
		for (cnt = 0; cnt < 16; cnt++)
		{
			if (handle->portSequenceCount < cnt)
				break;
			if (cnt < 8)
				buf |= (handle->portSequence[cnt] & 0xF) << (28 - (4 * cnt));
			if ((cnt >= 8) && (cnt < 16))
				buf3 |= (handle->portSequence[cnt] & 0xF) << (28 - (4 * (cnt - 8)));
		}
		sprintf(sPacketCommand, "70010005A7000000");
		_sendintelcmd(handle, sPacketCommand);
		sprintf(sPacketCommand, "70010105%02X%02X%02X%02X", (unsigned char)(buf & 0xff), (unsigned char)(buf >> 8 & 0xff), (unsigned char)(buf >> 16 & 0xff), (unsigned char)(buf >> 24 & 0xff));
		//		sprintf(sPacketCommand, "70010105000010F7");		// for test only
		_sendintelcmd(handle, sPacketCommand);
		printf("\n %s : Antenna Sequence Order #1: %s\n", handle->target_reader, sPacketCommand);
		// send HST_CMD
		sprintf(sPacketCommand, "700100F002000000"); // HST_CMD
		_sendintelcmd(handle, sPacketCommand);
		/*{
			int nread = 0;
			unsigned char buf[32];
			// response, 8-byte, no need to check the content
			nread = recv(handle->intelcmdfd, buf, 32, 0);
		}*/
		ClearRecvBuffer(handle); // Clear Command-End Packet

		sprintf(sPacketCommand, "70010005A8000000");
		_sendintelcmd(handle, sPacketCommand);
		sprintf(sPacketCommand, "70010105%02X%02X%02X%02X", (unsigned char)(buf3 & 0xff), (unsigned char)(buf3 >> 8 & 0xff), (unsigned char)(buf3 >> 16 & 0xff), (unsigned char)(buf3 >> 24 & 0xff));
		_sendintelcmd(handle, sPacketCommand);
		printf(" %s : Antenna Sequence Order #2: %s\n", handle->target_reader, sPacketCommand);
		sprintf(sPacketCommand, "700100F002000000"); // HST_CMD
		_sendintelcmd(handle, sPacketCommand);
		/*{
			int nread = 0;
			unsigned char buf[32];
			// response, 32-byte, no need to check the content
			nread = recv(handle->intelcmdfd, buf, 32, 0);
		}*/
		ClearRecvBuffer(handle); // Clear Command-End Packet
	}
	printf("Set port select to 0\n");
	_sendintelcmd(handle, "7001010700000000");

	return 0;
}

int config_read(int ac, char *av[], int isFileLine, CS203DC_Handle *CS203fd)
{
	int cnt;
	int conf_fromfile = 0;

	// set default values
	CS203fd->profile = 1; // 20161025
	CS203fd->power = 300;
	CS203fd->dwellTime = 2000; // Add Dwell Time 2014-05-15
	CS203fd->num_dBank = 0;	   // Number of data bank after inventory  - 2015-08-21
	CS203fd->tid_length = 0;   // Add TID Data Length 2015-08-20
	CS203fd->user_length = 0;  // Add User Data Length 2015-08-21
	CS203fd->multiport = 0;
	CS203fd->query_session = 0;
	CS203fd->query_target = 0;
	CS203fd->toggle = 1;
	CS203fd->sequenceMode = 0;
	CS203fd->Algorithm = 1; // dynamicQ
	CS203fd->MaxQ = 15;
	CS203fd->StartQ = 7;
	CS203fd->MinQ = 0;
	CS203fd->ThreadholdMultipler = 4;

	// new parameters
	CS203fd->inventorymode = 0;
	CS203fd->tagdelaycompactmode = 7;
	CS203fd->retry = 0;
	CS203fd->tagfocus = 0;
	CS203fd->rflnagain = 1;
	CS203fd->rfhighcompress = 1;
	CS203fd->iflnagain = 24;
	CS203fd->agcgain = -6;
	CS203fd->simpledisplay = 0;

	// Check Command Line
	cnt = 1;
	if (isFileLine != 0)
		cnt = 0;
	printf("Argument read:");
	while (cnt < ac)
	{
		if (av[cnt][0] == '-')
		{
			if ((isFileLine == 0) && (strcmp(&av[cnt][1], "conf") == 0))
			{
				printf(" -conf");
				conf_fromfile = 1;
				strncpy(CS203fd->configFile, av[cnt + 1], 50 - 1);
				// conf_Init (av[cnt + 1], CS203fd);
				break;
			}
			else if (strcmp(&av[cnt][1], "tag") == 0)
			{
				CS203fd->iConf_ReadTag = 1;
				printf(" -tag");
			}
			else if (strcmp(&av[cnt][1], "loop") == 0)
			{
				CS203fd->iConf_Forever = 1; // Test Forever
				printf(" -loop");
			}
			else if (strcmp(&av[cnt][1], "write") == 0)
			{
				CS203fd->iConf_WriteUser = 1;
				printf(" -write");
			}
			else if (strcmp(&av[cnt][1], "profile") == 0)
			{
				int profile;
				profile = atoi(av[cnt + 1]);
				CS203fd->profile = profile;
				printf(" -profile");
			}
			else if (strcmp(&av[cnt][1], "power") == 0)
			{
				int power;
				power = atoi(av[cnt + 1]);
				CS203fd->power = power;
				printf(" -power");
			}
			else if (strcmp(&av[cnt][1], "dwellTime") == 0)
			{
				int dwellTime;
				dwellTime = atoi(av[cnt + 1]);
				CS203fd->dwellTime = dwellTime;
				printf(" -dwellTime");
			}
			else if (strcmp(&av[cnt][1], "tid_length") == 0)
			{
				int tid_length;
				tid_length = atoi(av[cnt + 1]);
				CS203fd->tid_length = tid_length;
				printf(" -tid_length");
			}
			else if (strcmp(&av[cnt][1], "user_length") == 0)
			{
				int user_length;
				user_length = atoi(av[cnt + 1]);
				CS203fd->user_length = user_length;
				printf(" -user_length");
			}
			else if ((isFileLine == 0) && (strcmp(&av[cnt][1], "portconf") == 0))
			{
				CS_MultiPortconf(av[cnt + 1], CS203fd);
				CS203fd->multiport = 1;
				printf(" -portconf");
			}
			else if (strcmp(&av[cnt][1], "sequence") == 0)
			{
				CS203fd->sequenceMode = atoi(av[cnt + 1]);
				printf(" -sequence");
			}
			else if (strcmp(&av[cnt][1], "country") == 0)
			{
				printf(" -country");
				read_country_code(CS203fd, av[cnt + 1]);
			}
			else if (strcmp(&av[cnt][1], "freq_ch_no") == 0)
			{
				CS203fd->iConf_channel = atoi(av[cnt + 1]);
				printf(" -freq_ch_no");
			}
			else if (strcmp(&av[cnt][1], "session") == 0)
			{
				CS203fd->query_session = atoi(av[cnt + 1]);
				printf(" -session");
			}
			else if (strcmp(&av[cnt][1], "target") == 0)
			{
				CS203fd->query_target = atoi(av[cnt + 1]);
				printf(" -target");
			}
			else if (strcmp(&av[cnt][1], "toggle") == 0)
			{
				CS203fd->toggle = atoi(av[cnt + 1]);
				printf(" -toggle");
			}
			else if (strcmp(&av[cnt][1], "algorithm") == 0)
			{
				char *pch;
				pch = strtok(av[cnt + 1], ",");
				if (atoi(pch) == 0) // fixedQ
				{
					CS203fd->Algorithm = 0;
					CS203fd->QValue = atoi(strtok(NULL, ","));
				}
				else // dynamicQ
				{
					CS203fd->Algorithm = 1;
					CS203fd->StartQ = atoi(strtok(NULL, ","));
					CS203fd->MinQ = atoi(strtok(NULL, ","));
					CS203fd->MaxQ = atoi(strtok(NULL, ","));
					CS203fd->ThreadholdMultipler = atoi(strtok(NULL, ","));
				}
				printf(" -algorithm");
			}

			// new parameters
			else if (strcmp(&av[cnt][1], "inventorymode") == 0)
			{
				if (strcmp(&av[cnt + 1][0], "normal") == 0)
					CS203fd->inventorymode = 0;
				else if (strcmp(&av[cnt + 1][0], "compact") == 0)
					CS203fd->inventorymode = 1;
				printf(" -inventorymode(%d)", CS203fd->inventorymode);
			}
			else if (strcmp(&av[cnt][1], "tagdelaycompactmode") == 0)
			{
				CS203fd->tagdelaycompactmode = atoi(av[cnt + 1]);
				printf(" -tagdelaycompactmode");
			}
			else if (strcmp(&av[cnt][1], "retry") == 0)
			{
				CS203fd->retry = atoi(av[cnt + 1]);
				printf(" -retry");
			}
			else if (strcmp(&av[cnt][1], "tagfocus") == 0)
			{
				CS203fd->tagfocus = atoi(av[cnt + 1]);
				printf(" -tagfocus");
			}
			else if (strcmp(&av[cnt][1], "rflnagain") == 0)
			{
				CS203fd->rflnagain = atoi(av[cnt + 1]);
				printf(" -rflnagain");
			}
			else if (strcmp(&av[cnt][1], "rfhighcompress") == 0)
			{
				CS203fd->rfhighcompress = atoi(av[cnt + 1]);
				printf(" -rfhighcompress");
			}
			else if (strcmp(&av[cnt][1], "iflnagain") == 0)
			{
				CS203fd->iflnagain = atoi(av[cnt + 1]);
				printf(" -iflnagain");
			}
			else if (strcmp(&av[cnt][1], "agcgain") == 0)
			{
				CS203fd->agcgain = atoi(av[cnt + 1]);
				printf(" -agcgain");
			}
			else if (strcmp(&av[cnt][1], "simpledisplay") == 0)
			{
				CS203fd->simpledisplay = atoi(av[cnt + 1]);
				printf(" -simpledisplay");
			}
		}

		cnt++;
	}
	printf("\n");
	return conf_fromfile;
}

void *run(void *fd)
{
	CS203DC_Handle *CS203fd = fd;
	int ret, i;
	// int iport, cport, uport;
	unsigned int cnt;

	// Open:IP
	printf("\nConnecting : %s, %s\n", CS203fd->target_reader, CS203fd->rfid_reader_info->reader_name);
	ret = CS203DC_Open(CS203fd);
	if (ret != 0)
	{
		printf("Open Error : %d\n", ret);
		return 0;
	}

	printf("\nSetting configuration:\n");
	// Check reader OEM
	ret = RFID_CheckOEM(CS203fd);
	if (ret != 0)
	{
		printf("RFID_CheckOEM Error : %d\n", ret);
		return 0;
	}

	// Set DWell Time to 1s (will receive Antenna Cycle End Packet per second) for single port / CS203 applications

	// RFID_SetDwell (CS203fd, 1000);
	printf("%s : Set Dwell Time to %d in default port 0\n", CS203fd->target_reader, CS203fd->dwellTime);
	RFID_SetDwell(CS203fd, CS203fd->dwellTime);

	// set RF power (intel commands)
	printf("%s : Set Power to %d in default port 0\n", CS203fd->target_reader, CS203fd->power);
	CS203DC_SetPower(CS203fd, CS203fd->power);

	CS468_SetPort(CS203fd);

	// StartInv
	ret = 0;
	if (CS203fd->iConf_ReadTag == 1)
	{
		printf("%s : Start Inventory\n", CS203fd->target_reader);
		ret = CS203DC_StartInventory(CS203fd);
		if (ret != 0)
		{
			printf("%s : StartInv Error : %d\n", CS203fd->target_reader, ret);
			return 0;
		}

		printf("\nStart reading tag:\n");
		// Read Tag and Display
		/*time_t start_time = time (NULL);
		time_t time_tagsuccess = time (NULL);
		time_t time_reconnect;*/

		// Set Time to reconnect when reader is not detected.
		/*		if(CS203fd->FW_version >= 21839)    // If firmware version is 2.18.39 or later
					time_reconnect = 2;				// Time for reconnect : 2 seconds
				else
					time_reconnect = 10;				// Time to reconnect : 10 seconds
		*/

		// Retry reader connection in 15 seconds
		// while ((time (NULL) -  start_time) < 15 || CS203fd->iConf_Forever == 1)
		while (CS203fd->iConf_Forever == 1 || CS203fd->eTime < 20)
		{
			int result = readtag(CS203fd);
		}

		if (ret != 0)
		{
			printf("%s, Connection Failure %d\n", CS203fd->target_reader, ret);
			exit(1);
		}

		// StopInv
		ret = CS203DC_StopInventory(CS203fd);
		if (ret != 0)
		{
			printf("%s, CS203DC_StopInventory Failure %d\n", CS203fd->target_reader, ret);
			exit(1);
		}

		// Read TID and UserData
		// CS203DC_ReadTid (CS203fd, tzTagID);

		// CS203DC_ReadUserData (CS203fd, tzTagID);

		// Write Data
		if (CS203fd->iConf_WriteUser == 1)
		{
			// CS203DC_WriteEPCData (CS203fd, tzTagID);
		}
	}
	// Close
	ret = CS203DC_Close(CS203fd);
	if (ret != 0)
	{
		printf("%s, CS203DC_Close Failure %d\n", CS203fd->target_reader, ret);
		exit(1);
	}

	// Reboot CS203
	// CS203DC_Reboot (CS203fd->ipv4addr, uport);

	return 0;
}

/**
 *****************************************************************************
 **
 ** @brief  Command main routine
 **
 ** Command synopsis:
 **
 **     example READERHOSTNAME
 **
 ** @exitcode   0
 **
 *****************************************************************************/
int main(int ac, char *av[])
{
	CS203DC_Handle *CS203fd_first, *CS203fd;
	int conf_fromfile = 0;
	int rc;
	int cnt;

	/*
	 * Process comand arguments, determine reader name
	 * and verbosity level.
	 */

	system("/opt/rfid_power_off");
	sleep(1);
	system("/opt/rfid_power_on");
	sleep(1);
	setvbuf(stdout, NULL, _IOLBF, 0);

	printf("CSL Demo Program v2.9.0\n");
	if (ac < 2)
	{
		printf("\nUsage: %s [-conf filename] IPaddress [-tag] [-loop] [-country nn/nnn] [-freq_ch_no n] [-power nnn] [-dwellTime nnnn] [-tid_length n] [-user_length n] [-portconf filename] [-session s] [-sequence 0|1] [-target 0|1] [-toggle 0|1] [-algorithm a,p,p,p,p]\n", av[0]);
		printf("-conf : get setting from file, ignore others command line setting (see config.txt)\n");
		printf("-tag : Enable read tag (default : disable read tag)\n");
		printf("-loop : Loop forever (default : Without this [-loop], the reader run inventory for 20 seconds and close)\n");
		printf("-country : set country where the reader operates, [FCC] / [SG] / [ETSI] / [JP] (default country is FCC) \n");
		printf("-freq_ch_no : frequency channel number for -1 and -8 readers only, value ranging from 0-3 (default value is 0)\n");
		printf("-profile : set profile, ranging from 1-4 \n");
		printf("-power : set RF power, ranging from 0-300 \n");
		printf("-dwellTime : set Dwell Time in ms (default is 2000) \n");
		printf("-tid_length : set TID data length in WORD (default is 2) \n");
		printf("-user_length : set User data length in WORD (default is 2) \n");
		printf("-portconf : get port setting from file (for CS468 only)\n");
		printf("-write : write \"112233445566778899001122\" to EPC ID on last access tag (please use -tag)\n");
		printf("-session : session number (0-3)\n");
		printf("-sequence : 0=normal mode, 1=sequence mode\n");
		printf("-target : A=0, B=1\n");
		printf("-toggle : 1=Gen 2 A/B flag will be toggled after all arounds have been run on current target, 0=no toggle\n");
		printf("-algorithm : FixedQ=[0,QValue] DynamicQ=[1,StartQ,MinQ,MaxQ,ThreadholdMultiplier \n");

		// new parameters
		printf("-inventorymode normal // normal/compact (default normal)\n");
		printf("-tagdelaycompactmode 7 // (default 7)\n");
		printf("-retry 0 // (default 0, only meaningful if Target is A/B toggle)\n");
		printf("-tagfocus 0 // (default 0, can be 0 or 1, if 1, must have Session S1 and Target A set)\n");
		printf("-rflnagain 1 // (default 1, can be 1, 7 or 13)\n");
		printf("-rfhighcompress 1 // (default 1, can be 0 or 1, if RF LNA gain is 13, then this MUST be 0)\n");
		printf("-iflnagain 24 // (default 24, can be 24, 18, 12, 6)\n");
		printf("-agcgain -6 // (default -6, can be -12, -6, 0 6)\n");
		printf("-simpledisplay 0 // (default 0), can be 0 or 1\n");
		printf("-portn PortNumber(0-15),ActiveControl(enable/disable),Power(0-300),Dwell(2000),InventoryRounds(65536),LocalInventory(enable/disable),InventoryAlgorithm(DynamicQ),StartQ(7),LocalProfile(enable/disable),Profile(0-3),LocalChannel(enable/disable),Channel \n");

		exit(1);
	}

	CS203fd_first = handler_alloc();
	if (CS203fd_first == NULL)
	{
		printf("Memory Allocation Error!\n");
		exit(2);
	}

	char linebuf[200];
	int k = 0, i;
	for (i = 1; i < ac; i++, linebuf[k - 1] = 0x20)
	{
		strncpy(&linebuf[k], av[i], strlen(av[i]) + 1);
		k += (strlen(av[i]) + 1);
	}
	linebuf[k - 1] = 0;
	printf("Consolidated line = %s\n", linebuf);
	if (1)
		config_read0(linebuf, 0, CS203fd_first);
	else if (0)
		conf_fromfile = config_read(ac, av, 0, CS203fd_first);

	if (CS203fd_first->configFile[0] == 0)
	{
		// strncpy (CS203fd_first->target_reader, av[1], 50 - 1);
		// printf("Argument read: %s\n", CS203fd_first->target_reader);
	}
	else
		conf_Init(CS203fd_first->configFile, CS203fd_first);

	CS203fd = CS203fd_first;
	while (CS203fd != NULL)
	{
		CS203fd->rfid_reader_info = NULL;
		CS203fd = CS203fd->NextHandle;
	}

	// CheckLocalIP ();
	printf("\nEnumReader:\n");
	if (EnumReader(&rfid_reader_list))
	{
		printf("EnumReader error");
		exit(1);
	}
	CS203fd = CS203fd_first;
	cnt = 0;
	while (CS203fd != NULL)
	{
		if (CS203fd->rfid_reader_info)
		{
		}
		else if (strcmp(CS203fd->target_reader, "USB") == 0)
		{
			if (cnt >= rfid_reader_list.reader_count)
			{
				printf("Only %d reader available, not mapping %s\n", cnt, CS203fd->target_reader);
			}
			else
			{
				printf("mapping reader %d to %s\n", cnt, CS203fd->target_reader);
				CS203fd->rfid_reader_info = &rfid_reader_list.reader_info[cnt++];
			}
		}
		else
		{
			printf("unknown reader %s\n", CS203fd->target_reader);
		}
		CS203fd = CS203fd->NextHandle;
	}

	/*
	 * Run application, capture return value for exit status
	 */
	CS203fd = CS203fd_first;
	while (CS203fd != NULL)
	{
		if (CS203fd->rfid_reader_info)
			rc = pthread_create(&(CS203fd->threadfd), NULL, run, (void *)CS203fd);
		CS203fd = CS203fd->NextHandle;
	}

	/*
	 * Check Run application, capture return value for exit status
	 */

	CS203fd = CS203fd_first;
	while (CS203fd != NULL)
	{
		if (CS203fd->rfid_reader_info)
			rc = pthread_join(CS203fd->threadfd, NULL);
		CS203fd = CS203fd->NextHandle;
	}

	printf("INFO: Done\n");

	/*
	 * Exit with the right status.
	 */
	if (rc)
		exit(2);

	exit(0);
}
