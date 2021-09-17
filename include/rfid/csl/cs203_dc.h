#ifndef __CS203_DC_H__
#define __CS203_DC_H__

// control commands
/*#define CS203DC_GETVERSION					"80000013"
#define CS203DC_GETVERSION_RSP			"81010413XXXXXXXX"
#define CS203DC_TURNOFFRFID				"80000002"
#define CS203DC_TURNOFFRFID_RSP			"8101010200"
#define CS203DC_TURNONRFID					"80000001"
#define CS203DC_TURNONRFID_RSP			"8101010100"
#define CS203DC_DISABLEKEEPALIVE			"8000011500"
#define CS203DC_DISABLEKEEPALIVE_RSP	"8101011500"
#define CS203DC_ENABLECRCFILTER			"8000011601"
#define CS203DC_ENABLECRCFILTER_RSP		"8101011600"
#define CS203DC_DISABLECRCFILTER			"8000011600"
#define CS203DC_DISABLECRCFILTER_RSP	"8101011600"
#define CS203DC_ENABLETCPNOTIFY			"8000011701"
#define CS203DC_ENABLETCPNOTIFY_RSP		"8101011700"
#define CS203DC_DISABLETCPNOTIFY			"8000011700"
#define CS203DC_DISABLETCPNOTIFY_RSP	"8101011700"
*/

// intel commands
/*#define CS203DC_ABORTCMD			"4003000000000000"
#define CS203DC_ABORTCMD_RSP		"4003BFFCBFFCBFFC"*/
//#define CS203DC_HSTANTCYCLES		"7001000701000000"
/*#define CS203DC_STARTINVENTORY01	"70010007FFFF0000"
#define CS203DC_STARTINVENTORY02	"7001000900000000"
#define CS203DC_STARTINVENTORY03	"7001020901000000"
#define CS203DC_STARTINVENTORY04	"7000030900000000"
#define CS203DC_STARTINVENTORY05	"70010309F7005003"
#define CS203DC_STARTINVENTORY09	"7001010901000000"
#define CS203DC_STARTINVENTORY10	"7000010900000000"
#define CS203DC_STARTINVENTORY11	"7001010901400000"*/
#define CS203DC_STARTINVENTORY14	"700100F00F000000"
//#define CS203DC_COMMANDBEGIN_RSP	"02010080020000000F000000881D0200"
#define CS203DC_HSTANTDESCSEL		"7001010700000000"
#define CS203DC_MACERROR			"7000050000000000"
#define CS203DC_MACERROR_RSP		"70000500XXXXXXXX"

#define CS203DC_OEMADDR_02			"7001000502000000"
#define CS203DC_OEMADDR_A4			"70010005A4000000"
#define CS203DC_HST_READ_OEM		"700100f003000000"

//#define CS203DC_CURRENT_PROFILE		"700100f003000000"	// rubbish define
#define CS203DC_HST_CHG_PROFILE		"700100f019000000"


//#define CS203DC_TCPNOTIFY_RSP		"9898989898989898"

#define MAX_EPCLEN	64	// in byte, max buffer size for EPC
#define MAX_TAGBUFLEN	50	// number of tag to be stored for duplicate elimination

// Reader Model
#define READER_MODEL_CS101_TEXT			"CS101"
#define READER_MODEL_CS103_TEXT			"CS103"
#define READER_MODEL_CS203_TEXT			"CS203"
#define READER_MODEL_CS209_TEXT			"CS209"
#define READER_MODEL_CS333_TEXT			"CS333"
#define READER_MODEL_CS463_TEXT			"CS463"
#define READER_MODEL_CS468_TEXT			"CS468"
#define READER_MODEL_CS468INT_TEXT		"CS468INT"
#define READER_MODEL_CS469_TEXT			"CS469"


typedef enum _CS203_COUNTRY
{
	CTY_FCC = 0,
	CTY_ETSI,
	CTY_IN,
	CTY_G800,
	CTY_AU,
	CTY_BR1,
	CTY_BR2,
	CTY_HK,
	CTY_TH,
	CTY_SG,
	CTY_MY,
	CTY_ZA,
	CTY_ID,
	CTY_CN,
	CTY_CN1,
	CTY_CN2,
	CTY_CN3,
	CTY_CN4,
	CTY_CN5,
	CTY_CN6,
	CTY_CN7,
	CTY_CN8,
	CTY_CN9,
	CTY_CN10,
	CTY_CN11,
	CTY_CN12,
	CTY_TW,
	CTY_JP
} CS203DC_COUNTRY;


typedef enum _CS203DC_State
{
	CS203DCSTATE_IDLE = 0,
	CS203DCSTATE_CONNECTING,
	CS203DCSTATE_CONNECTED,
	CS203DCSTATE_BUSY,
	CS203DCSTATE_STARTINGINV,
	CS203DCSTATE_INV,
	CS203DCSTATE_STOPPINGINV,
	CS203DCSTATE_CLOSING,
	CS203DCSTATE_INVALID
} CS203DC_State;

typedef struct _CS203DC_PacketHeader
{
	unsigned char pkt_ver;
	unsigned char flags;
	unsigned short pkt_type;
	unsigned short pkt_len;
	unsigned short reserved;
} CS203DC_PacketHeader;

typedef struct _CS203DC_TagInfo
{
	char epc[MAX_EPCLEN];
	int len; // in byte
	int count;
} CS203DC_TagInfo;



struct StruPortConfig
{
	long	Active;
	long	Power;
	long	Dwell;
	long	InventoryRounds;
	long	LocalInventory;
	long	Algorithm;
	long	StartQ;
	long	LocalProfile;
	long	Profile;
	long	LocalChannel;
	long	Channel;
};


typedef struct _CS203DC_Handle
{
	const struct rfid_reader_info *rfid_reader_info;
	struct rfid_reader_context rfid_reader_context;
	
	char target_reader[50];
//	char ReaderHostName[50];				// CS203 Reader IP/Host Name
//	unsigned char sMacAdd[6];				// CS203 Mac Address
//	char tzDevName[32];						// CS203 Device Name
	pthread_t threadfd;						// thread ID
//	unsigned int ipv4addr;					// IP Address
	CS203DC_State state;			// State (not use)
//	int intelcmdfd;						// 1515 Port Handler
//	int controlcmdfd;					// 1516 Port Handler
	int iConf_ReadTag;						// 
	int iConf_Forever;						// 
	int iConf_WriteUser;
	CS203DC_COUNTRY country;
	int iConf_channel;
	int profile;				//20161025
	int power;
	int dwellTime;				// Add Dwell Time 2014-05-15
	int num_dBank;				// Number of data bank after inventory  - 2015-08-21
	int tid_length;				// TID Length  - 2015-08-21
	int user_length;			// User Length  - 2015-08-21	
	int multiport;
	int query_session;
	int query_target;
	int toggle;
	int sequenceMode;
	int Algorithm;
	int QValue;
	int MaxQ;
	int MinQ;
	int StartQ;
	int ThreadholdMultipler;
// for tag filter
	CS203DC_TagInfo tag[MAX_TAGBUFLEN]; // ring buffer like, the head is pointed by head
	int tagbeginidx;
	int dupelicount;
	int restartinv;
	int portSequence[16];
	int portSequenceCount;
	int OEM_country;
	int OEM_model;
	
	unsigned int FW_version;
	int flag_decodePacket;
	int decodePacketBuf_offset;
	
	struct StruPortConfig PortConfig[16];
	struct _CS203DC_Handle *NextHandle;	// Next Handler
} CS203DC_Handle;

/*int CS203DC_Open( CS203DC_Handle* handle);
int CS203DC_Close(CS203DC_Handle* handle);
int CS203DC_StartInventory(CS203DC_Handle* handle);
int CS203DC_StopInventory(CS203DC_Handle* handle);
int CS203DC_RestartInventory(CS203DC_Handle* handle);
int CS203DC_DecodePacket(CS203DC_Handle* handle, unsigned char* inbuf, int inlen, int* inprocessed, unsigned char* outbuf, int outlen, int* outused, unsigned int* errorcode); 
int CS203DC_CheckUDP(CS203DC_Handle* handle);
int CS203DC_SetPower(CS203DC_Handle* handle, int power);
int CS203DC_GetMacError(CS203DC_Handle* handle, int* macerror);
int CS203DC_Reboot (unsigned int ipv4addr, int udpport);
int CS203DC_GetMode (unsigned int ipv4addr, int udpport);
int CS203DC_SetMode_High (unsigned int ipv4addr, int udpport);
int CS203DC_SetMode_Low (unsigned int ipv4addr, int udpport);*/

#endif /* __CS203_DC_H__ */
