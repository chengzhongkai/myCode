/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "ell_task.h"
#include "Config.h"

#ifdef APP_ENL_MODBUS

//=============================================================================
typedef struct {
	MESSAGE_HEADER_STRUCT header;

	/*** Application Dependent ***/
	uint32_t eoj;
	uint8_t slaveId;
	uint8_t cmd_id;
	uint8_t edt;
} ELL_AdpReqMsg_t;


typedef struct {
    MQX_FILE *fd;
    int baudrate;
    char *dev_file;
} UARTHandle_t;

typedef struct {
	uint8_t cmd;
	uint16_t addr;
	uint16_t loopPlus;
	uint16_t length;
	uint8_t epc;
	uint8_t (*fun)(void *);
} MODBUS_Address_t;

typedef struct {
	uint8_t settingStatus[64];
	uint8_t eoj[64];
	uint16_t settingTemperature[64];
	uint16_t operationMode[64];
} MAP_TOSHIBA;
#define STATUS_BIT  0x01
#define CONNECT_BIT 0x02
#define STATUS_VALUE_BIT 0x04
#define TEMPERATURE_VALUE_BIT 0x08
#define MODE_VALUE_BIT 0x10
#define GETTED_ALL_PARA (STATUS_VALUE_BIT | TEMPERATURE_VALUE_BIT | MODE_VALUE_BIT)


typedef struct {
	MESSAGE_HEADER_STRUCT header;

	/*** Application Dependent ***/
	uint32_t eoj;
	uint8_t cmd_id;
} ELL_AdpReqMsg_t;

typedef struct {
	uint8_t msg[255];
	uint16_t length;
	uint8_t *pEnd;
} MODBUS_MSG;

UARTHandle_t gUART;
static LWEVENT_STRUCT gPOLLTaskSyncEvent;
static MODBUS_MSG gPollReceivedMsg;
static MODBUS_MSG gPollSendMsg;

#define RS485_DEVIVCE_NAME "ittyb:"

#define POLL_IDLE 0x0001
#define POLL_WAITING_REPLY 0x0002
#define POLL_RECEIVED 0x0004


#define ADU_SIZE_MAX     256 /*!< Maximum size of a ADU. */
#define ADU_SIZE_MIN     4   /*!< Function Code */
#define ADU_SLAVEID_OFF     0   /*!< Offset of function code in PDU. */
#define ADU_FUNC_OFF     1   /*!< Offset of function code in PDU. */
#define ADU_DATA_OFF     2   /*!< Offset for response data in PDU. */

#define FUN_READ_COIL 1
#define FUN_READ_DISC 2
#define FUN_READ_HOLDING 3
#define FUN_READ_REGISTER 4
#define FUN_WRITE_COIL 5
#define FUN_WRITE_ 6



static _pool_id gELLAdpPoolID = 0;
static _queue_id gELLAdpReqQID = 0;
ELL_Object_t *gProfileObj;
//ELL_Object_Mapping_t gMapping[100];

uint8_t findSlaveId(uint32_t eoj);

uint8_t ELL_ConstructNewObject(uint16_t eojClass, const uint8_t *define);
uint8_t ELL_ConstructNewAirconIndoor();
bool_t ELL_ConstructNewAirconOutdoor();
uint8_t con80(void *v);
uint8_t conB0(void *v);
uint8_t conB3(void *v);
void getMsg();
void setMemoryMapping();
bool_t ReadDisc(uint8_t slaveId, uint16_t address, uint16_t len);
bool_t ReadHolding(uint8_t slaveId, uint16_t address, uint16_t len);
uint32_t  uart1_idle_callback(void* parameter);

void initTimer();


static MODBUS_Address_t TOSHIBA[] ={
	{0x02,0x01,152,1,0x80,con80},
	{0x03,0x01,156,1,0xb3,conB3},
	{0x03,0x07,156,1,0xb0,conB0},
	{0,0,0,0,0}
};
static MAP_TOSHIBA gMapToshiba;


int gCurMsg=-1;
int gSlaveId;
uint32_t   gTimeout;


uint8_t con80(void *v){
	if(*((uint8_t*)v)==0){
          return 0x31;
        }else{
          return 0x30;
        }
}

uint8_t conB0(void *v){
	switch(*((uint16_t*)v)){
		case 01:
			return 0x43;
			break;
		case 02:
			return 0x42;
			break;
		case 03:
			return 0x44;
			break;
		case 04:
			return 0x45;
			break;
		default:
			return 0x41;
			break;
	}
		
			
}

uint8_t conB3(void *v){
	return *((uint16_t*)v)/10;
}



/* Table of CRC values for high?order byte */
static unsigned char auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40
} ;

/* Table of CRC values for low?order byte */
static char auchCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
0x40
};


//=============================================================================
const uint8_t gNodeProfileEPC[] = {
    0x80,  1, 0x30, (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO),
    0x82,  4, 0x01, 0x0a, 0x01, 0x00, EPC_FLAG_RULE_GET,
    0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
              0x0e, 0xf0, 0x01, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,
    
    0xd3,  3, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET, // N of Instances
    0xd4,  2, 0x00, 0x01, EPC_FLAG_RULE_GET, // N of Classes (includes Node Prof)
    0xd5,  1, 0x00, 
              EPC_FLAG_RULE_ANNO, // Instance List Notification
    0xd6,  1, 0x00,
              EPC_FLAG_RULE_GET, // Node Instance List
    0xd7,  1, 0x00, EPC_FLAG_RULE_GET, // Node Class List


    0x00 /*** Terminater ***/
};

//=============================================================================
const uint8_t gAirconIndoorEPC[] = {
    0x80,  1, 0x31,
    EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
    | EPC_FLAG_RULE_SETUP,
	0xb0,  1, 0x41,
    EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
    | EPC_FLAG_RULE_SETUP,
	0xb3,  1, 0x00,
    EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
    | EPC_FLAG_RULE_SETUP,
    0xe3,  2, 0xfe, 0x0c,
    EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
    | EPC_FLAG_RULE_SETUP,
    0x81,  1, 0x00,
              EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
              | EPC_FLAG_RULE_SETUP | EPC_FLAG_RULE_GETUP,
    0x82,  4, 0x00, 0x00, 0x44, 0x00, EPC_FLAG_RULE_GET,
    0x88,  1, 0x42, EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,

    0x00 /*** Terminater ***/
};
const uint8_t gAirconOutdoorEPC[] = {
    0x80,  1, 0x31,
    EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
    | EPC_FLAG_RULE_SETUP,
	0xd1,  1, 0x42,
    EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
    | EPC_FLAG_RULE_SETUP,
	
    0x81,  1, 0x00,
              EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
              | EPC_FLAG_RULE_SETUP | EPC_FLAG_RULE_GETUP,
    0x82,  4, 0x00, 0x00, 0x44, 0x00, EPC_FLAG_RULE_GET,
    0x88,  1, 0x42, EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,

    0x00 /*** Terminater ***/
};


#if (ELL_USE_FIXED_PDC != 0)
//=============================================================================
static uint8_t gInstallationLocation[2][17];
#endif

/* The function returns the CRC as a unsigned short type */

uint16_t getCRC ( uint8_t *puchMsg, uint16_t usDataLen ) 
{
if(0){
  uint8_t uchCRCHi = 0xFF ; /* high byte of CRC initialized */
  uint8_t uchCRCLo = 0xFF ; /* low byte of CRC initialized */
  uint16_t uIndex ; /* will index into CRC lookup table */
  
  while (usDataLen--){ /* pass through message buffer */
    uIndex = uchCRCLo ^ *puchMsg++ ; /* calculate the CRC */
    uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex] ;
    uchCRCHi = auchCRCLo[uIndex] ;
  }
  return (uchCRCHi << 8 | uchCRCLo) ;

}


uint8_t hi = 0xFF;
uint8_t lo = 0xFF;
uint8_t i;

for (int j = 0, l = usDataLen; j < l; ++j) {

	i = lo^*(puchMsg+j);
	lo = hi^auchCRCHi[i];
	hi = auchCRCLo[i];
}


return (lo << 8 | hi);

  
}


//=============================================================================
// Init Application
//=============================================================================
bool_t ELL_InitApp(void)
{
	return (TRUE);
}
bool_t ELL_ConstructProFileObject(void)
{
	uint8_t id_number[17];
	_enet_address mac;

    // -------------------------------------- Construct Node Profile Object ---
    gProfileObj = ELL_RegisterObject(0x0ef001, gNodeProfileEPC);
    if (gProfileObj == NULL) return (FALSE);

	// ------------------------------------ Init ID Property (Node Profile) ---
	ELL_GetProperty(gProfileObj, 0x83, id_number, 17);
	ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
	MEM_Copy(&id_number[11], (uint8_t *)mac, 6);
	ELL_InitProperty(gProfileObj, 0x83, 17, id_number);
         return (TRUE);
}


//=============================================================================
// Construct ECHONET Lite Objects
//=============================================================================
bool_t ELL_ConstructObjects(void)
{
     


    ELL_InitObjects();

    // ------------------------------------------ Construct Switch Object 1 ---
	ELL_ConstructNewAirconIndoor();
	ELL_ConstructNewAirconIndoor();
	ELL_ConstructNewAirconOutdoor();
	
    // ------------------------------------------ Construct Switch Object 2 ---



    return (TRUE);
}

uint8_t ELL_ConstructNewObject(uint16_t eojClass, const uint8_t *define)
{

	uint8_t index=0;


    ELL_Object_t *obj;
	uint32_t eoj=eojClass<<8;
	uint8_t eoj1=eojClass>>8;
	uint8_t eoj2=eojClass;

	//get index
	index=ELL_ApplyEOJIndex(eojClass);

	if (gProfileObj == NULL)ELL_ConstructProFileObject();
	if (gProfileObj == NULL) return (FALSE);


	eoj+=index;
	obj = ELL_RegisterObject(eoj, define);
    if (obj == NULL) return (FALSE);


#if (ELL_USE_FIXED_PDC != 0)
    gInstallationLocation[index-1][0] = 0x01;
	gInstallationLocation[index-1][1] = 0xb0;
	gInstallationLocation[index-1][2] = 0xc0;
#else
    uint8_t installation_location[] = { 0x00 };
    ELL_InitProperty(obj, 0x81, 1, installation_location);
#endif

	// Init Power State Property (0x80)
    uint8_t edt[1];
	edt[0] = 0x31;
	ELL_InitProperty(obj, 0x80, 1, edt);


	//update profile property 0xd3~0xd7
	int i;
	uint8_t *pEdt;
	uint8_t pdc;
	pEdt = MEM_Alloc(253);
	MEM_Set(pEdt, 0x00, 253);
	// 0xd3 N of Instances max(3) 
	pdc=ELL_GetProperty(gProfileObj, 0xd3, pEdt, 3);
	(*(pEdt+2))++;
	ELL_SetProperty(gProfileObj, 0xd3, 3, pEdt);
	
	// 0xd7 Node Class List max(17) 1:cnt 2~17:class code(2byte)
	MEM_Set(pEdt, 0x00, 253);
	ELL_GetProperty(gProfileObj, 0xd7, pEdt, 253);
	for(i=0;i<*pEdt;i++){
		if(*(pEdt+i*2+1)==eoj1 && *(pEdt+i*2+2)==eoj2 ){
			break;
		}
	}
	if(i==*pEdt){
		*(pEdt+i*2+1)=eoj1;
		*(pEdt+i*2+2)=eoj2;
		ELL_SetProperty(gProfileObj, 0xd7, *pEdt*2+3,pEdt);
		// 0xd4 N of Classes (includes Node Prof) max(2) 
		MEM_Set(pEdt, 0x00, 253);
		ELL_GetProperty(gProfileObj, 0xd4, pEdt, 2);
		(*(pEdt+1))++;
		ELL_SetProperty(gProfileObj, 0xd4, 2, pEdt);
	}
	// 0xd5 Instance List Notification max(253) 1:cnt 2~253:Instance code(3byte)
	MEM_Set(pEdt, 0x00, 253);
	ELL_GetProperty(gProfileObj, 0xd5, pEdt, 253);
	i=*pEdt;
	(*pEdt)++;
	*(pEdt+i*3+1)=eoj1;
	*(pEdt+i*3+2)=eoj2;
	*(pEdt+i*3+3)=index;
	ELL_SetProperty(gProfileObj, 0xd5, *pEdt*3+4, pEdt);
	// 0xd6 Node Instance List max(253) 1:cnt 2~17:Instance code(3byte)
	MEM_Set(pEdt, 0x00, 253);
	ELL_GetProperty(gProfileObj, 0xd6, pEdt, 253);
	i=*pEdt;
	(*pEdt)++;
	*(pEdt+i*3+1)=eoj1;
	*(pEdt+i*3+2)=eoj2;
	*(pEdt+i*3+3)=index;
	ELL_SetProperty(gProfileObj, 0xd6,*pEdt*3+1, pEdt);
	 return index;
}


bool_t ELL_ConstructNewAirconOutdoor()
{
	uint16_t eojClass =0x0146;
	return ELL_ConstructNewObject(eojClass,gAirconOutdoorEPC);

}

uint8_t ELL_ConstructNewAirconIndoor()
{
	uint16_t eojClass =0x0145;
	return ELL_ConstructNewObject(eojClass,gAirconIndoorEPC);

}

//=============================================================================
// ECHONET Lite Adapter Control Task
//=============================================================================
void ELL_AdpCtrl_Task(uint32_t param)
{
	ELL_AdpReqMsg_t *req_msg;

	// ------------------------------------------------ Create Message Pool ---
	gELLAdpPoolID = _msgpool_create(sizeof(ELL_AdpReqMsg_t), 20, 0, 0);
	if (gELLAdpPoolID == MSGPOOL_NULL_POOL_ID) {
		err_printf("[ELL ADP] Cannot Create Message Pool\n");
		_task_block();
	}

	// ----------------------------------------- Open Request Message Queue ---
	gELLAdpReqQID = _msgq_open(ELL_ADP_CTRL_REQ_QUEUE, 0);
	if (gELLAdpReqQID == MSGQ_NULL_QUEUE_ID) {
		err_printf("[ELL ADP] Cannot Open Request Message Queue\n");
		_task_block();
	}

	// -------------------------------------------------- Notify Task Start ---
	ELL_StartTask(ELL_TASK_START_ADP_CTRL);

	// ---------------------------------------------------------- Main Loop ---
	while (1) {
		req_msg = _msgq_receive(gELLAdpReqQID, 0);
		if (req_msg == NULL) continue;

		switch (req_msg->cmd_id) {

		default:
			break;
		}

		_msg_free(req_msg);
	}
}

static void hwtimer15_callback(void *p)
{
	//NOTHING
}
static void hwtimer35_callback(void *p)
{
	if (_lwevent_set(&gPOLLTaskSyncEvent,POLL_RECEIVED) != MQX_OK) {
		 
	}
}
static void timeout_callback(void *p)
{
int a=1;
	//TODO error deal with
	if (_lwevent_set(&gPOLLTaskSyncEvent,POLL_IDLE) != MQX_OK) {
		 
	}
a=2;

}

//=============================================================================
// ECHONET Lite Adapter Sync Task
//=============================================================================
void ELL_AdpSync_Task(uint32_t param)
{
	int send_size;
	uint32_t result;
        int err;
        char enable_idleline = 0x01;
		 bool disable_rx = TRUE;

	// -------------------------------------------------- Notify Task Start ---
	ELL_StartTask(ELL_TASK_START_ADP_SYNC);



        //set uart
        // ------------------------------------------- Try to Close Before Open ---
        if (gUART.fd >= 0) {
          fclose(gUART.fd);
        }
	// ---------------------------------------------------------- Open UART ---
        //gUART.fd = fopen(RS485_DEVIVCE_NAME, (void *)IO_SERIAL_HW_485_FLOW_CONTROL);
        gUART.fd = fopen(RS485_DEVIVCE_NAME, (void *)IO_SERIAL_NON_BLOCKING);
        if (gUART.fd == NULL) {
          // return (FALSE);
        }
	// ------------------------------------------------------- Set Baudrate ---
	 result = (uint32_t)9600;
	err = ioctl(gUART.fd, IO_IOCTL_SERIAL_SET_BAUD, &result);
	if (err != MQX_OK) {
		fclose(gUART.fd);
		gUART.fd = NULL;
		//return (FALSE);
	}
	// ---------------------------------------------------- Set Parity Even ---
	result = IO_SERIAL_PARITY_EVEN;
	err = ioctl(gUART.fd, IO_IOCTL_SERIAL_SET_PARITY, &result);
	if (err != MQX_OK) {
		fclose(gUART.fd);
		gUART.fd = NULL;
		//return (FALSE);
	}

	// ---------------------------------------------------- Set idle callback function ---
	err = ioctl(gUART.fd, IO_IOCTL_SERIAL_IDLE_CALLBACK, (void *)uart1_idle_callback);
		if (err != MQX_OK) {
			fclose(gUART.fd);
			gUART.fd = NULL;
			//return (FALSE);
		}


	/* wait for transfer complete flag */
	result = ioctl( gUART.fd, IO_IOCTL_SERIAL_ENABLE_IDLE, &enable_idleline  );
   if( result == IO_ERROR_INVALID_IOCTL_CMD )
   {
      /* ioctl not supported, use newer MQX version */
      _task_block();
   }
   //_int_install_isr(INT_UART1_RX_TX, UART1_RX_ISR,NULL); 



	//   
	/* create lwevent group */
   if (_lwevent_create(&gPOLLTaskSyncEvent,0) != MQX_OK) {
      printf("\nMake event failed");
      _task_block();
   }	
   if (_lwevent_set(&gPOLLTaskSyncEvent,POLL_IDLE) != MQX_OK) {
        printf("\nSet Event failed");
        _task_block();
   }
   
	HWTIMER hwtimer1; 
	initTimer();

    //T15
	hwtimer_init(&hwtimer1, &pit_devif,0,(BSP_DEFAULT_MQX_HARDWARE_INTERRUPT_LEVEL_MAX+1));
    hwtimer_set_period(&hwtimer1, BSP_HWTIMER1_SOURCE_CLK, 4010);//4.01ms on 9600
    
    hwtimer_callback_reg(&hwtimer1,(HWTIMER_CALLBACK_FPTR)hwtimer15_callback, NULL);

	// ---------------------------------------------------------- Main Loop ---
    while (0){
      gPollSendMsg.length= read(gUART.fd, gPollSendMsg.msg,100);
        if (gPollSendMsg.length <= 0) {
            _time_delay(30);
        }else{
        gPollSendMsg.length=gPollSendMsg.length;
        }
    }
	while (1) {
	  if (_lwevent_wait_ticks(&gPOLLTaskSyncEvent,POLL_IDLE,TRUE,0) != MQX_OK) {
            printf("\nEvent Wait failed");
            _task_block();
          }

          if (_lwevent_clear(&gPOLLTaskSyncEvent,POLL_IDLE) != MQX_OK) {
            printf("\nEvent Clear failed");
            _task_block();
          }

	   result = ioctl( gUART.fd, IO_IOCTL_SERIAL_DISABLE_RX, &disable_rx );
            if( result == IO_ERROR_INVALID_IOCTL_CMD )
              {
              _task_block();
            }

		//TODO SEND MESSAGE
		getMsg();
		send_size = write(gUART.fd, gPollSendMsg.msg, gPollSendMsg.length);
		if (send_size != gPollSendMsg.length) {
        	//TODO error
    	}	
		//fflush( gUART.fd );
		result = ioctl( gUART.fd, IO_IOCTL_SERIAL_WAIT_FOR_TC, NULL );
   if( result == IO_ERROR_INVALID_IOCTL_CMD )
   {
      /* ioctl not supported, use newer MQX version */
      _task_block();
   }

      disable_rx = FALSE;
   ioctl( gUART.fd, IO_IOCTL_SERIAL_DISABLE_RX, &disable_rx ) ;
   
		//TODO SET TIMEOUT
		//gTimeout=_timer_start_oneshot_after(timeout_callback,NULL,(1),6000);
		
	  if (_lwevent_set(&gPOLLTaskSyncEvent,POLL_WAITING_REPLY) != MQX_OK) {
        printf("\nSet Event failed");
        _task_block();
      }
	}
}

void getMsg(){
	//
	ELL_AdpReqMsg_t *req_msg;
	uint16_t address;
	uint16_t len;
	uint16_t crc;
	//

	req_msg = _msgq_receive(gELLAdpReqQID, 0);
	if (req_msg == NULL){
		gCurMsg++;
		if(TOSHIBA[gCurMsg].fun == 0){
			gCurMsg = 0;
			gSlaveId++;
		}
		if(gSlaveId == 0 || gSlaveId>64){
			gSlaveId = 1;
		}
		address=TOSHIBA[gCurMsg].addr+(gSlaveId-1)*TOSHIBA[gCurMsg].loopPlus;
		len=TOSHIBA[gCurMsg].length;

		gPollSendMsg.msg[ADU_SLAVEID_OFF] = gSlaveId;
		gPollSendMsg.msg[ADU_FUNC_OFF] = TOSHIBA[gCurMsg].cmd;
		gPollSendMsg.msg[ADU_DATA_OFF] = address>>8;
		gPollSendMsg.msg[ADU_DATA_OFF + 1] = address&0x0ff;
		gPollSendMsg.msg[ADU_DATA_OFF + 2] = len>>8;
		gPollSendMsg.msg[ADU_DATA_OFF + 3] = len&0x0ff;

		crc = getCRC(gPollSendMsg.msg,6);
		memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+4], &crc, 2 );
		gPollSendMsg.length=8;
	
	}else{
		gPollSendMsg.msg[ADU_SLAVEID_OFF] = req_msg->slaveId;

		switch (req_msg->cmd_id) {
		case 0x80:
			//gPollSendMsg.msg[ADU_FUNC_OFF] = 
			//gPollSendMsg.msg[ADU_DATA_OFF] =
		case 0xB0:
		case 0xB3:
			
		default:
			break;
		}
		
		_msg_free(req_msg);

	}


	
	
	

}


//=============================================================================
// ECHONET Lite Adapter Sync Task
//=============================================================================
void MOD_ProcessingResponse_Task(uint32_t param)
{
	// -------------------------------------------------- Notify Task Start ---


	// ---------------------------------------------------------- Main Loop ---
	while (1) {
	  //1. wait received event
	  if (_lwevent_wait_ticks(&gPOLLTaskSyncEvent,POLL_RECEIVED,TRUE,0) != MQX_OK) {
         printf("\nEvent Wait failed");
         _task_block();
      }

	  //2. clear event
      if (_lwevent_clear(&gPOLLTaskSyncEvent,POLL_RECEIVED) != MQX_OK) {
          printf("\nEvent Clear failed");
          _task_block();
      }
	  _timer_cancel(gTimeout);

      //3. get message
	  gPollReceivedMsg.length= read(gUART.fd, &gPollReceivedMsg.msg[gPollReceivedMsg.length],100);
      if (gPollReceivedMsg.length >= 6) {
        //CRC chk
		uint16_t crc=getCRC(gPollReceivedMsg.msg,gPollReceivedMsg.length-2);
		if(*(uint16_t*)(gPollReceivedMsg.msg+gPollReceivedMsg.length-2) ^ crc){
		}else{
		  //set value
          setMemoryMapping();
          _lwevent_set(&gPOLLTaskSyncEvent,POLL_IDLE) ;
        }
		
	  }
	
	  //loop end-----------

	}
}

void setMemoryMapping(){

	//1. check slave device id
	if(gPollReceivedMsg.msg[ADU_SLAVEID_OFF] != gPollSendMsg.msg[ADU_SLAVEID_OFF]){

	}

	//2. check function
	if(gPollReceivedMsg.msg[ADU_FUNC_OFF] != gPollSendMsg.msg[ADU_FUNC_OFF]){

	}

    //3. set value on memory
	if(gPollReceivedMsg.msg[ADU_FUNC_OFF] == FUN_READ_DISC){
		//3.1. device status
		gMapToshiba.settingStatus[gSlaveId-1] |= STATUS_VALUE_BIT;
		//3.2. check if value had changed
		if(gMapToshiba.settingStatus[gSlaveId-1] != gPollReceivedMsg.msg[ADU_DATA_OFF+1]){
			gMapToshiba.settingStatus[gSlaveId-1] = gPollReceivedMsg.msg[ADU_DATA_OFF+1];
			//3.3  set value on echonet lite object
			if(gMapToshiba.eoj[gSlaveId-1] != 0){
                          ELL_Object_t *obj = ELL_FindObject(gMapToshiba.eoj[gSlaveId-1]);
						  uint8_t edt=con80(&gPollReceivedMsg.msg[ADU_DATA_OFF+1]);
			  ELL_SetProperty(obj,0x80,1,&edt);
			}
		}
	}else if(gPollReceivedMsg.msg[ADU_FUNC_OFF] == FUN_READ_HOLDING){
		if(TOSHIBA[gCurMsg].addr==1){
			gMapToshiba.settingStatus[gSlaveId-1] |= TEMPERATURE_VALUE_BIT;
			if(gMapToshiba.settingTemperature[gSlaveId-1] != gPollReceivedMsg.msg[ADU_FUNC_OFF+1]<<8+gPollReceivedMsg.msg[ADU_FUNC_OFF+2]){
				gMapToshiba.settingTemperature[gSlaveId-1] = gPollReceivedMsg.msg[ADU_FUNC_OFF+1]<<8+gPollReceivedMsg.msg[ADU_FUNC_OFF+2];
				if(gMapToshiba.eoj[gSlaveId-1] != 0){
			          ELL_Object_t *obj = ELL_FindObject(gMapToshiba.eoj[gSlaveId-1]);
					  uint8_t edt=conB3(&gMapToshiba.settingTemperature[gSlaveId-1]);
			          ELL_SetProperty(obj,0xB3,1,&edt);
				}
			}
		}else{
			gMapToshiba.settingStatus[gSlaveId-1] |= MODE_VALUE_BIT;
			if(gMapToshiba.operationMode[gSlaveId-1]==gPollReceivedMsg.msg[ADU_FUNC_OFF+1]<<8+gPollReceivedMsg.msg[ADU_FUNC_OFF+2]){
				gMapToshiba.operationMode[gSlaveId-1] = gPollReceivedMsg.msg[ADU_FUNC_OFF+1]<<8+gPollReceivedMsg.msg[ADU_FUNC_OFF+2];
				if(gMapToshiba.eoj[gSlaveId-1] != 0){
					ELL_Object_t *obj = ELL_FindObject(gMapToshiba.eoj[gSlaveId-1]);
					uint8_t edt=conB0(&gMapToshiba.operationMode[gSlaveId-1]);
			        ELL_SetProperty(obj,0xB0,1,&edt);
				}

			}
		}
	}
	if((gMapToshiba.operationMode[gSlaveId-1] & GETTED_ALL_PARA == GETTED_ALL_PARA )
		&& (gMapToshiba.operationMode[gSlaveId-1] & ~CONNECT_BIT)){
		gMapToshiba.operationMode[gSlaveId-1] |= CONNECT_BIT;
		gMapToshiba.eoj[gSlaveId-1] = ELL_ConstructNewAirconIndoor();
		
	}
	
}

bool_t ReadCoil(uint8_t slaveId, uint16_t address, uint16_t len){
	gPollSendMsg.msg[ADU_SLAVEID_OFF] = slaveId;
	gPollSendMsg.msg[ADU_FUNC_OFF] = FUN_READ_COIL;
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF], &address, 2 );
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+2], &len, 2 );
	uint16_t crc= getCRC(gPollSendMsg.msg,6);
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+4], &crc, 2 );
	gPollSendMsg.length=8;
	return (TRUE);
}

bool_t ReadDisc(uint8_t slaveId, uint16_t address, uint16_t len){
	gPollSendMsg.msg[ADU_SLAVEID_OFF] = slaveId;
	gPollSendMsg.msg[ADU_FUNC_OFF] = FUN_READ_DISC;
	gPollSendMsg.msg[ADU_DATA_OFF] = address>>8;
	gPollSendMsg.msg[ADU_DATA_OFF + 1] = address&0x0ff;
	gPollSendMsg.msg[ADU_DATA_OFF + 2] = len>>8;
	gPollSendMsg.msg[ADU_DATA_OFF + 3] = len&0x0ff;
	uint16_t crc= getCRC(gPollSendMsg.msg,6);
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+4], &crc, 2 );
	gPollSendMsg.length=8;
	return (TRUE);
}

bool_t ReadInput(uint8_t slaveId, uint16_t address, uint16_t len){
	gPollSendMsg.msg[ADU_SLAVEID_OFF] = slaveId;
	gPollSendMsg.msg[ADU_FUNC_OFF] = FUN_READ_REGISTER;
	gPollSendMsg.msg[ADU_DATA_OFF] = address>>8;
	gPollSendMsg.msg[ADU_DATA_OFF + 1] = address&0x0ff;
	gPollSendMsg.msg[ADU_DATA_OFF + 2] = len>>8;
	gPollSendMsg.msg[ADU_DATA_OFF + 3] = len&0x0ff;
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+2], &len, 2 );
	uint16_t crc= getCRC(gPollSendMsg.msg,6);
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+4], &crc, 2 );
	gPollSendMsg.length=8;
	return (TRUE);
}

bool_t ReadHolding(uint8_t slaveId, uint16_t address, uint16_t len){
	gPollSendMsg.msg[ADU_SLAVEID_OFF] = slaveId;
	gPollSendMsg.msg[ADU_FUNC_OFF] = FUN_READ_HOLDING;
	gPollSendMsg.msg[ADU_DATA_OFF] = address>>8;
	gPollSendMsg.msg[ADU_DATA_OFF + 1] = address&0x0ff;
	gPollSendMsg.msg[ADU_DATA_OFF + 2] = len>>8;
	gPollSendMsg.msg[ADU_DATA_OFF + 3] = len&0x0ff;

	uint16_t crc= getCRC(gPollSendMsg.msg,6);
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+4], &crc, 2 );
	gPollSendMsg.length=8;
	return (TRUE);
}

bool_t WriteCoil(uint8_t slaveId, uint16_t address, bool value){
	gPollSendMsg.msg[ADU_SLAVEID_OFF] = slaveId;
	gPollSendMsg.msg[ADU_FUNC_OFF] = FUN_WRITE_COIL;
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF], &address, 2 );
	if(FALSE == value){
		gPollSendMsg.msg[ADU_DATA_OFF+2] = 0;
	}else{
		gPollSendMsg.msg[ADU_DATA_OFF+3] = 0xFF;
	}
	gPollSendMsg.msg[ADU_DATA_OFF+3] = 0;
	uint16_t crc= getCRC(gPollSendMsg.msg,6);
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+4], &crc, 2 );
	gPollSendMsg.length=8;
	return (TRUE);
}

//todo
bool_t Writeholding(uint8_t slaveId, uint16_t address, uint16_t value){
	gPollSendMsg.msg[ADU_SLAVEID_OFF] = slaveId;
	gPollSendMsg.msg[ADU_FUNC_OFF] = FUN_WRITE_COIL;
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF], &address, 2 );
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+2], &value, 2 );
	
	gPollSendMsg.msg[ADU_DATA_OFF+3] = 0;
	uint16_t crc= getCRC(gPollSendMsg.msg,6);
	memcpy( &gPollSendMsg.msg[ADU_DATA_OFF+4], &crc, 2 );
	gPollSendMsg.length=8;
	return (TRUE);
}


uint32_t  uart1_idle_callback(void* parameter)
{
  //uint16_t para=*(uint16_t*)parameter;
 // char s=para >>8;
  //char c=para & 0xFF;

   // if (s & UART_S1_RDRF_MASK) {
   //     gPollReceivedMsg.msg[gPollReceivedMsg.length++]=c ;
   // }
   // if (s & UART_S1_IDLE_MASK) {
    	// clear interrupt flag; To clear IDLE, read UART status S1 with IDLE set and then read D
         _lwevent_set(&gPOLLTaskSyncEvent,POLL_RECEIVED);
        
    	
   // }
    


   
	return 0;
}

#define TIMER_TASK_PRIORITY  2
#define TIMER_STACK_SIZE     2000

void initTimer(){
	MQX_TICK_STRUCT ticks;
	MQX_TICK_STRUCT dticks;

	_timer_create_component(TIMER_TASK_PRIORITY, TIMER_STACK_SIZE);
	_time_init_ticks(&dticks, 0);
	


}

uint8_t findSlaveId(uint32_t eoj)
{
    uint8_t cnt;

    for (cnt = 0; cnt < 64; cnt ++) {
        if (gMapToshiba.eoj[cnt] == eoj) {
            return cnt+1;
        }
    }

    return 0;
}


static bool_t AIRCON_SetProperty(ELL_Object_t *obj,
                             uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
	ELL_AdpReqMsg_t *msg;

    if (obj == NULL || edt == NULL || pdc == 0) return (FALSE);

    switch (epc) {
    case 0x80:
	case 0xB0:
	case 0xB3:
		// -------------------------------- Set Power State Property (0x80) ---
        if (!ELL_CheckProperty(obj, epc, pdc, edt)) {
            return (FALSE);
        }
		uint8_t slaveId = findSlaveId(uint32_t obj->eoj);

		msg->slaveId = slaveId;
		msg->cmd_id = epc;
		msg->edt= *edt;
		if (!_msgq_send(msg)) {
			_msg_free(msg);
			return (FALSE);
		}

		return (TRUE);

    default:
        break;
    }

    return (FALSE);
}



#endif /* APP_ENL_MODBUS */

/******************************** END-OF-FILE ********************************/
