/**************************************************************************************************
  Filename:       SerialApp.c
  Revised:        $Date: 2009-03-29 10:51:47 -0700 (Sun, 29 Mar 2009) $
  Revision:       $Revision: 19585 $

  Description -   Serial Transfer Application (no Profile).


  Copyright 2004-2009 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This sample application is basically a cable replacement
  and it should be customized for your application. A PC
  (or other device) sends data via the serial port to this
  application's device.  This device transmits the message
  to another device with the same application running. The
  other device receives the over-the-air message and sends
  it to a PC (or other device) connected to its serial port.
				
  This application doesn't have a profile, so it handles everything directly.

  Key control:
    SW1:
    SW2:  initiates end device binding
    SW3:
    SW4:  initiates a match description request
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "AF.h"
#include "OnBoard.h"
#include "OSAL_Tasks.h"
#include "SerialApp.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "hal_drivers.h"
#include "hal_key.h"
#if defined ( LCD_SUPPORTED )
  #include "hal_lcd.h"
#endif
#include "hal_led.h"
#include "hal_uart.h"
#include  "hal_adc.h"
#include "stdio.h"


/*********************************************************************
 * 本设置的sid，nid，设置
 */
#define sid  001
#define nid  001

/*********************************************************************
 * CONSTANTS
 */

#if !defined( SERIAL_APP_PORT )
#define SERIAL_APP_PORT  0
#endif

#if !defined( SERIAL_APP_BAUD )
//#define SERIAL_APP_BAUD  HAL_UART_BR_38400
#define SERIAL_APP_BAUD  HAL_UART_BR_115200
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( SERIAL_APP_THRESH )
#define SERIAL_APP_THRESH  64
#endif

#if !defined( SERIAL_APP_RX_SZ )
#define SERIAL_APP_RX_SZ  128
#endif

#if !defined( SERIAL_APP_TX_SZ )
#define SERIAL_APP_TX_SZ  128
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( SERIAL_APP_IDLE )
#define SERIAL_APP_IDLE  6
#endif

// Loopback Rx bytes to Tx for throughput testing.
#if !defined( SERIAL_APP_LOOPBACK )
#define SERIAL_APP_LOOPBACK  FALSE
#endif

// This is the max byte count per OTA message.
#if !defined( SERIAL_APP_TX_MAX )
#define SERIAL_APP_TX_MAX  80
#endif

#define SERIAL_APP_RSP_CNT  4

// This list should be filled with Application specific Cluster IDs.
const cId_t SerialApp_ClusterList[SERIALAPP_MAX_CLUSTERS] =
{
  SERIALAPP_CLUSTERID1,
  SERIALAPP_CLUSTERID2
};

const SimpleDescriptionFormat_t SerialApp_SimpleDesc =
{
  SERIALAPP_ENDPOINT,              //  int   Endpoint;
  SERIALAPP_PROFID,                //  uint16 AppProfId[2];
  SERIALAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SERIALAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SERIALAPP_FLAGS,                 //  int   AppFlags:4;
  SERIALAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)SerialApp_ClusterList,  //  byte *pAppInClusterList;
  SERIALAPP_MAX_CLUSTERS,          //  byte  AppNumOutClusters;
  (cId_t *)SerialApp_ClusterList   //  byte *pAppOutClusterList;
};

const endPointDesc_t SerialApp_epDesc =
{
  SERIALAPP_ENDPOINT,
 &SerialApp_TaskID,
  (SimpleDescriptionFormat_t *)&SerialApp_SimpleDesc,
  noLatencyReqs
};

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

uint8 SerialApp_TaskID;    // Task ID for internal task/event processing.

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static uint8 SerialApp_MsgID;

static afAddrType_t SerialApp_TxAddr;
static uint8 SerialApp_TxSeq;
static uint8 SerialApp_TxBuf[SERIAL_APP_TX_MAX+1];
static uint8 SerialApp_TxLen;

static afAddrType_t SerialApp_RxAddr;
static uint8 SerialApp_RspBuf[SERIAL_APP_RSP_CNT];
uint16  adc;
devStates_t SerialApp_NwkState;
/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void SerialApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void SerialApp_HandleKeys( uint8 shift, uint8 keys );
static void SerialApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt );
static void SerialApp_Send(void);
static void SerialApp_Resp(void);
static void SerialApp_CallBack(uint8 port, uint8 event);
void HexToAsc(void);
static void GetZnck_Body(char *p,char *s);
/*********************************************************************
 * @fn      SerialApp_Init
 *
 * @brief   This is called during OSAL tasks' initialization.
 *
 * @param   task_id - the Task ID assigned by OSAL.
 *
 * @return  none
 */
void SerialApp_Init( uint8 task_id )
{
  halUARTCfg_t uartConfig;

  SerialApp_TaskID = task_id;

  afRegister( (endPointDesc_t *)&SerialApp_epDesc );

  RegisterForKeys( task_id );

  uartConfig.configured           = TRUE;              // 2x30 don't care - see uart driver.
  uartConfig.baudRate             = SERIAL_APP_BAUD;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = SERIAL_APP_THRESH; // 2x30 don't care - see uart driver.
  uartConfig.rx.maxBufSize        = SERIAL_APP_RX_SZ;  // 2x30 don't care - see uart driver.
  uartConfig.tx.maxBufSize        = SERIAL_APP_TX_SZ;  // 2x30 don't care - see uart driver.
  uartConfig.idleTimeout          = SERIAL_APP_IDLE;   // 2x30 don't care - see uart driver.
  uartConfig.intEnable            = TRUE;              // 2x30 don't care - see uart driver.
  uartConfig.callBackFunc         = SerialApp_CallBack;
  HalUARTOpen (SERIAL_APP_PORT, &uartConfig);
  
  
  uint8 *sendBuf="SerialApp_Init\n";  
  HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
  
  P1SEL &= ~0x02;   //设P1.0为普通I/O功能
  P1DIR |= 0x01;    //设P1.0为输出方向
  
  P1_1 ^=1;
  
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SerialApp", HAL_LCD_LINE_2 );
#endif

  ZDO_RegisterForZDOMsg( SerialApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( SerialApp_TaskID, Match_Desc_rsp );
}

/*********************************************************************
 * @fn      SerialApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events   - Bit map of events to process.
 *
 * @return  Event flags of all unprocessed events.
 */
UINT16 SerialApp_ProcessEvent( uint8 task_id, UINT16 events )
{
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    afIncomingMSGPacket_t *MSGpkt;

    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SerialApp_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
      case ZDO_CB_MSG:        
        SerialApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
        break;

      case KEY_CHANGE:        
        SerialApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
        break;

      case AF_INCOMING_MSG_CMD:        
        SerialApp_ProcessMSGCmd( MSGpkt );
        break;

      case ZDO_STATE_CHANGE:
              
          SerialApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if(SerialApp_NwkState == DEV_ROUTER)
          {
              uint8 *sendBuf="DEV_ROUTER\n";  
              HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
              
              HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
              //osal_start_timerEx( SerialApp_TaskID, SERIALAPP_SAMPLE_EVT, 6000);  //拿掉此行为串口透明传输, 否则为定时采样数据
          }
          if ( SerialApp_NwkState == DEV_ZB_COORD)
          {
              uint8 *sendBuf="DEV_ZB_COORD\n";  
              HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
              
              HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          break;

      default:
        break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    return ( events ^ SYS_EVENT_MSG );
  }

  if ( events & SERIALAPP_SAMPLE_EVT )
  {
    
    uint8 *sendBuf="SERIALAPP_SAMPLE_EVT\n";  
    HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
    
    osal_memset(SerialApp_TxBuf, 0, SERIAL_APP_TX_MAX);
        
    adc = GetAdc();
    
    uint8 SerialApp_Tx[SERIAL_APP_TX_MAX+1]={0};
    sprintf(SerialApp_Tx, "{ck001002%2d.%2d}", 53, 53); //%4.3fV DEVID, adc   
    HalUARTWrite(SERIAL_APP_PORT,SerialApp_Tx,osal_strlen(SerialApp_Tx));
    
    SerialApp_TxAddr.addrMode =(afAddrMode_t) Addr16Bit;
    SerialApp_TxAddr.addr.shortAddr = 0xFFFF;//0x0000
    SerialApp_TxAddr.endPoint = SERIALAPP_ENDPOINT;
    AF_DataRequest(&SerialApp_TxAddr, (endPointDesc_t *)&SerialApp_epDesc,
                  SERIALAPP_CLUSTERID1, osal_strlen(SerialApp_Tx), SerialApp_Tx,  &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS);

    osal_start_timerEx( SerialApp_TaskID, SERIALAPP_SAMPLE_EVT, 6000);
    HAL_TOGGLE_LED1();
    return ( events ^ SERIALAPP_SAMPLE_EVT );
  }



  if ( events & SERIALAPP_SEND_EVT )
  {
    
    uint8 *sendBuf="SERIALAPP_SEND_EVT\n";  
    HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
  
    SerialApp_Send();
    return ( events ^ SERIALAPP_SEND_EVT );
  }

  if ( events & SERIALAPP_RESP_EVT )
  {    
    SerialApp_Resp();
    return ( events ^ SERIALAPP_RESP_EVT );
  }

  return ( 0 );  // Discard unknown events.
}

/*********************************************************************
 * @fn      SerialApp_ProcessZDOMsgs()
 *
 * @brief   Process response messages
 *
 * @param   none
 *
 * @return  none
 */
static void SerialApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  uint8 *sendBuf="SerialApp_ProcessZDOMsgs\n";  
  HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
  
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // Light LED
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
#if defined(BLINK_LEDS)
      else
      {
        // Flash LED to show failure
        HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
      }
#endif
      break;

    case Match_Desc_rsp:
      {
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        if ( pRsp )
        {
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            SerialApp_TxAddr.addrMode = (afAddrMode_t)Addr16Bit;
            SerialApp_TxAddr.addr.shortAddr = pRsp->nwkAddr;
            // Take the first endpoint, Can be changed to search through endpoints
            SerialApp_TxAddr.endPoint = pRsp->epList[0];

            // Light LED
            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );
        }
      }
      break;
  }
}

/*********************************************************************
 * @fn      SerialApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys  - bit field for key events.
 *
 * @return  none
 */
void SerialApp_HandleKeys( uint8 shift, uint8 keys )
{
  zAddrType_t txAddr;
  
  uint8 sendBuf[20]={0};
  sprintf(sendBuf, "SerialApp_HandleKeys:%d\n", keys);
  HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));  
  
  HAL_TOGGLE_LED1();
  if ( keys == HAL_KEY_SW_6 )
  {
    HAL_TOGGLE_LED2();
    
    osal_memset(SerialApp_TxBuf, 0, SERIAL_APP_TX_MAX);
    uint8 *sendBuf="{ck00100250.60}\n";  
    sprintf(SerialApp_TxBuf, "%s\n", sendBuf);
    //adc = GetAdc();
    //sprintf(SerialApp_TxBuf, "%2d----%4.3fV\n", DEVID, adc);
    //sprintf(SerialApp_TxBuf, "%s\n", "HAL_KEY_SW_6");
    
    SerialApp_TxAddr.addrMode =(afAddrMode_t)AddrBroadcast;
         SerialApp_TxAddr.addr.shortAddr = 0xFFFF;
         SerialApp_TxAddr.endPoint = SERIALAPP_ENDPOINT;
    AF_DataRequest(&SerialApp_TxAddr, (endPointDesc_t *)&SerialApp_epDesc,
                  SERIALAPP_CLUSTERID1, osal_strlen(SerialApp_TxBuf), SerialApp_TxBuf,  &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS);
       
        
  }
  if ( keys == HAL_KEY_SW_7 )
  {
    HAL_TOGGLE_LED3();
    
    osal_memset(SerialApp_TxBuf, 0, SERIAL_APP_TX_MAX);
    uint8 *sendBuf="{ck00100250.70}\n";  
    sprintf(SerialApp_TxBuf, "%s\n", sendBuf);
    
    SerialApp_TxAddr.addrMode =(afAddrMode_t)AddrBroadcast;
         SerialApp_TxAddr.addr.shortAddr = 0xFFFF;
         SerialApp_TxAddr.endPoint = SERIALAPP_ENDPOINT;
    AF_DataRequest(&SerialApp_TxAddr, (endPointDesc_t *)&SerialApp_epDesc,
                  SERIALAPP_CLUSTERID1, osal_strlen(SerialApp_TxBuf), SerialApp_TxBuf,  &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS);
    
  }
  
  
  if ( shift )  {
    
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

      // Initiate an End Device Bind Request for the mandatory endpoint
      txAddr.addrMode = Addr16Bit;
      txAddr.addr.shortAddr = 0x0000; // Coordinator
      ZDP_EndDeviceBindReq( &txAddr, NLME_GetShortAddr(),
                            SerialApp_epDesc.endPoint,
                            SERIALAPP_PROFID,
                            SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                            SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                            FALSE );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
    }

    if ( keys & HAL_KEY_SW_4 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

      // Initiate a Match Description Request (Service Discovery)
      txAddr.addrMode = AddrBroadcast;
      txAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &txAddr, NWK_BROADCAST_SHORTADDR,
                        SERIALAPP_PROFID,
                        SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                        SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                        FALSE );
    }
  }
}

/*********************************************************************
 * @fn      SerialApp_ProcessMSGCmd
 *
 * @brief   Data message processor callback. This function processes
 *          any incoming data - probably from other devices. Based
 *          on the cluster ID, perform the intended action.
 *
 * @param   pkt - pointer to the incoming message packet
 *
 * @return  TRUE if the 'pkt' parameter is being used and will be freed later,
 *          FALSE otherwise.
 */
void SerialApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt )
{
  uint8 *sendBuf="SerialApp_ProcessMSGCmd\n";  
  HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
  
  uint8 delay;
  
  switch ( pkt->clusterId )
  {
  // A message with a serial data block to be transmitted on the serial port.
      case SERIALAPP_CLUSTERID1:    
           //osal_memset(pkt->cmd.Data, 0, pkt->cmd.DataLength);
           osal_memcpy(&SerialApp_RxAddr, &(pkt->srcAddr), sizeof( afAddrType_t ));
           //HalUARTWrite(SERIAL_APP_PORT, pkt->cmd.Data, (pkt->cmd.DataLength)); 
           
           char body[80]={0};
           GetZnck_Body(pkt->cmd.Data,body);
           if( osal_strlen(body)>3)
           {
             if(body[0]=='{' && body[1]=='c' && body[2]=='k')
             {
               HalUARTWrite( SERIAL_APP_PORT, body, osal_strlen(body));               
               
               //uint8 *sendBuf="{cn00100250.80}\n";  
               //HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
               
               HAL_TOGGLE_LED1();
             }
           }
          
           //HalUARTWrite( SERIAL_APP_PORT, pkt->cmd.Data, (pkt->cmd.DataLength));
           
           
           break;

  // A response to a received serial data block.
  case SERIALAPP_CLUSTERID2:
    //uint8 *sendBuf="SERIALAPP_CLUSTERID2\n";  
    HalUARTWrite(SERIAL_APP_PORT,"SERIALAPP_CLUSTERID2\n",21);
             
    if ((pkt->cmd.Data[1] == SerialApp_TxSeq) &&
       ((pkt->cmd.Data[0] == OTA_SUCCESS) || (pkt->cmd.Data[0] == OTA_DUP_MSG)))
    {
      SerialApp_TxLen = 0;
      osal_stop_timerEx(SerialApp_TaskID, SERIALAPP_SEND_EVT);
    }
    else
    {
      // Re-start timeout according to delay sent from other device.
      delay = BUILD_UINT16( pkt->cmd.Data[2], pkt->cmd.Data[3] );
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_SEND_EVT, delay );
    }    
    break;

    default:
      break;
  }
}



/*********************************************************************
 * @fn      SerialApp_Send
 *
 * @brief   Send data OTA.
 *
 * @param   none
 *
 * @return  none
 */
static void SerialApp_Send(void)
{
  osal_memset(SerialApp_TxBuf, 0, SERIAL_APP_TX_MAX);
  if (!SerialApp_TxLen && (SerialApp_TxLen = HalUARTRead(SERIAL_APP_PORT, SerialApp_TxBuf, SERIAL_APP_TX_MAX)))
  {
    
    if( osal_strlen(SerialApp_TxBuf)>3)
    {
        char body[80]={0};
        GetZnck_Body(SerialApp_TxBuf,body);
        
       if(body[0]=='{' && body[1]=='c' && body[2]=='k')
       {
         uint8 SerialApp_Tx[SERIAL_APP_TX_MAX+1]={0};
         sprintf(SerialApp_Tx, "%s", body);         
    
         //sprintf(SerialApp_TxBuf, "%s", body);
         
         SerialApp_TxAddr.addrMode =(afAddrMode_t)AddrBroadcast;
         SerialApp_TxAddr.addr.shortAddr = 0xFFFF;
         SerialApp_TxAddr.endPoint = SERIALAPP_ENDPOINT;
         uint8 Status_t=AF_DataRequest(&SerialApp_TxAddr, (endPointDesc_t *)&SerialApp_epDesc,
                        SERIALAPP_CLUSTERID1, osal_strlen(SerialApp_Tx), SerialApp_Tx,
                        &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS);
         
         uint8 sendBuf[85]={0};
         sprintf(sendBuf, "SerialApp_Send:%d\n", Status_t);
         //sprintf(sendBuf, "SerialApp_Send:%s/%d\n", SerialApp_TxBuf, Status_t);
         HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
        
         HAL_TOGGLE_LED1();
       }
    }
     
     
     
     //HAL_TOGGLE_LED2();//P1_1 ^= 1;
     //HAL_TOGGLE_LED3();
     //HAL_TOGGLE_LED4();     
               
     
  }
  SerialApp_TxLen= 0;
}

/*********************************************************************
 * @fn      SerialApp_Resp
 *
 * @brief   Send data OTA.
 *
 * @param   none
 *
 * @return  none
 */
static void SerialApp_Resp(void)
{
  uint8 *sendBuf="SerialApp_Resp\n";  
  HalUARTWrite(SERIAL_APP_PORT,sendBuf,osal_strlen(sendBuf));
  
//  if (afStatus_SUCCESS != AF_DataRequest(&SerialApp_RxAddr,
//                                         (endPointDesc_t *)&SerialApp_epDesc,
//                                          SERIALAPP_CLUSTERID2,
//                                          SERIAL_APP_RSP_CNT, SerialApp_RspBuf,
//                                         &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS))
//  {
//    osal_set_event(SerialApp_TaskID, SERIALAPP_RESP_EVT);
//  }
}

/*********************************************************************
 * @fn      SerialApp_CallBack
 *
 * @brief   Send data OTA.
 *
 * @param   port - UART port.
 * @param   event - the UART port event flag.
 *
 * @return  none
 */
static void SerialApp_CallBack(uint8 port, uint8 event)
{
  (void)port;

  if ((event & (HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT)) &&
#if SERIAL_APP_LOOPBACK
      (SerialApp_TxLen < SERIAL_APP_TX_MAX))
#else
      !SerialApp_TxLen)
#endif
  {
    SerialApp_Send();
  }
}



static void GetZnck_Body(char *p,char *s){
  
  char rechar[81]={0};
  int bufi=0;
  
  bool isend=false;
  int charnum=0;    
  
  for(bufi=0;bufi<osal_strlen(p);bufi++){
    //Serial.print(p[bufi]);
    
    if(p[bufi]=='{'){
      isend=true;
    }
    if(p[bufi]=='}' && isend==true){
      isend=false;
      rechar[charnum]=p[bufi];
      break;
    }
    if(isend){
      if(charnum<80){
        rechar[charnum]=p[bufi];
        charnum++;        
      }
    }
    
  }  
  //memcpy(s,rechar,17);
  sprintf(s,"%s",rechar);
}

/*********************************************************************
*********************************************************************/
