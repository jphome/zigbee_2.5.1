/**************************************************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Sample Application (no Profile).


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends it's messages either as broadcast or
  broadcast filtered group messages.  The other (more normal)
  message addressing is unicast.  Most of the other sample
  applications are written to support the unicast message model.

  Key control:
    SW1:  Sends a flash command to all devices in Group 1.
    SW2:  Adds/Removes (toggles) this device in and out
          of Group 1.  This will enable and disable the
          reception of the flash command.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;

aps_Group_t SampleApp_Group;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendFlashMessage( uint16 flashTime );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void SampleApp_Init( uint8 task_id )
{
	SampleApp_TaskID = task_id;
	SampleApp_NwkState = DEV_INIT;
	SampleApp_TransID = 0;

	HalLedBlink( HAL_LED_1, 0, 50, 500 );

	// Device hardware initialization can be added here or in main() (Zmain.c).
	// If the hardware is application specific - add it here.
	// If the hardware is other parts of the device add it in main().

#if 0
#if defined ( BUILD_ALL_DEVICES )
	/**
	 * 通过在编译的时候加 BUILD_ALL_DEVICES 宏可以将同一套代码编译成协调、路由节点
	 */
	// The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
	// We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
	// together - if they are - we will start up a coordinator. Otherwise,
	// the device will start as a router.
	if ( readCoordinatorJumper() )
	{
		zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
	}
	else
	{
		zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
	}
#endif // BUILD_ALL_DEVICES
#endif

	zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;

#if 0
#if defined ( HOLD_AUTO_START )
	// HOLD_AUTO_START is a compile option that will surpress ZDApp
	//  from starting the device and wait for the application to
	//  start the device.
	ZDOInitDevice(0);
#endif
#endif

	// 设置定时器中断的目的地址(Setup for the periodic message's destination address)
	// 广播到所有节点(Broadcast to everyone)
	SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
	SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
	SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

	// Setup for the flash command's destination address - Group 1
	SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
	SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
	SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;

	// Fill out the endpoint description.
	SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
	SampleApp_epDesc.task_id = &SampleApp_TaskID;
	SampleApp_epDesc.simpleDesc
	= (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
	SampleApp_epDesc.latencyReq = noLatencyReqs;

	// 在AF层登记应用对象(Register the endpoint description with the AF)
	afRegister( &SampleApp_epDesc );

	// 登记可用的OSAL或HAL系统服务(Register for all key events - This app will handle all key events)
	RegisterForKeys( SampleApp_TaskID );

	// 设备默认加到Group 1(By default, all devices start out in Group 1)
	SampleApp_Group.ID = 0x0001;
	osal_memcpy( SampleApp_Group.name, "Group 1", 7  );
	aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );

	HalLedBlink( HAL_LED_1, 0, 50, 500 );
	//HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
}

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   应用事件处理函数
 * 			Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
	afIncomingMSGPacket_t *MSGpkt;
	(void)task_id;  // Intentionally unreferenced parameter

	/* 系统事件 */
	if ( events & SYS_EVENT_MSG )
	{
		/* 循环处理消息 */
		MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
		while ( MSGpkt )
		{
			switch ( MSGpkt->hdr.event )
			{
				// 按键消息(Received when a key is pressed)
				case KEY_CHANGE:
					SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
					break;

				// RF接收到数据(Received when a messages is received (OTA) for this endpoint)
				case AF_INCOMING_MSG_CMD:
					SampleApp_MessageMSGCB( MSGpkt );
					break;

				// 在网络中状态改变(Received whenever the device changes state in the network)
				case ZDO_STATE_CHANGE:
					SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
					if ( (SampleApp_NwkState == DEV_ZB_COORD)
						|| (SampleApp_NwkState == DEV_ROUTER)
						|| (SampleApp_NwkState == DEV_END_DEVICE) )
					{
						// Start sending the periodic message in a regular interval.
						osal_start_timerEx( SampleApp_TaskID,
					          SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
					          SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
					}
					else
					{
						;// Device is no longer in the network
					}
					break;

				// 匹配描述符请求响应,如自动匹配
				case ZDO_NEW_DSTADDR:
					break;

				// AF_DataRequest()数据请求调用的响应
				case AF_DATA_CONFIRM_CMD:
					break;
					
				default:
					break;
			}

			// Release the memory
			osal_msg_deallocate( (uint8 *)MSGpkt );

			// Next - if one is available
			MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
		}

		// 返回没处理的事件(return unprocessed events)
		return (events ^ SYS_EVENT_MSG);
	}

	/* 定时器中断事件 */
	/* 初始化的位置在SampleApp_Init()(setup in SampleApp_Init()). */
	if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
	{
		// Send the periodic message
		SampleApp_SendPeriodicMessage();

		// 重新设置定时器(Setup to send message again in normal period (+ a little jitter))
		osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
		(SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT + (osal_rand() & 0x00FF)) );

		// 返回没处理的事件(return unprocessed events)
		return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
	}

	// Discard unknown events
	return 0;
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      SampleApp_HandleKeys
 *
 * @brief   按键消息处理(Handles all key events for this device.)
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
	(void)shift;  // Intentionally unreferenced parameter

	if ( keys & HAL_KEY_SW_1 )
	{
		/* This key sends the Flash Command is sent to Group 1.
		* This device will not receive the Flash Command from this
		* device (even if it belongs to group 1).
		*/
		/**
		 * 按键1: 发送一个消息到组1的设备
		 */
		SampleApp_SendFlashMessage( SAMPLEAPP_FLASH_DURATION );
	}

	if ( keys & HAL_KEY_SW_2 )
	{
		/* The Flashr Command is sent to Group 1.
		* This key toggles this device in and out of group 1.
		* If this device doesn't belong to group 1, this application
		* will not receive the Flash command sent to group 1.
		*/
		/**
		 * 按键2: 退出/加入组1
		 */
		aps_Group_t *grp;
		grp = aps_FindGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
		if ( grp )
		{
			// Remove from the group
			aps_RemoveGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
		}
		else
		{
			// Add to the flash group
			aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
		}
	}
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
	uint16 flashTime;

	switch ( pkt->clusterId )
	{
		case SAMPLEAPP_PERIODIC_CLUSTERID:
			// 周期性收到广播数据
			break;

		case SAMPLEAPP_FLASH_CLUSTERID:
			flashTime = BUILD_UINT16(pkt->cmd.Data[1], pkt->cmd.Data[2] );
			HalLedBlink( HAL_LED_4, 4, 50, (flashTime / 4) );
			break;
	}
}

/*********************************************************************
 * @fn      SampleApp_SendPeriodicMessage
 *
 * @brief   定时器中断周期性(T=5s)发送广播信息
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_SendPeriodicMessage( void )
{
	if ( AF_DataRequest( &SampleApp_Periodic_DstAddr, &SampleApp_epDesc,
							SAMPLEAPP_PERIODIC_CLUSTERID,
							1,
							(uint8*)&SampleAppPeriodicCounter,
							&SampleApp_TransID,
							AF_DISCV_ROUTE,
							AF_DEFAULT_RADIUS ) 
								== afStatus_SUCCESS )
	{
		;		// Successfully requested to be sent.
	}
	else
	{
		;		// Error occurred in request to send.
	}
}

/*********************************************************************
 * @fn      SampleApp_SendFlashMessage
 *
 * @brief   Send the flash message to group 1.
 *
 * @param   flashTime - in milliseconds
 *
 * @return  none
 */
void SampleApp_SendFlashMessage( uint16 flashTime )
{
  uint8 buffer[3];
  buffer[0] = (uint8)(SampleAppFlashCounter++);
  buffer[1] = LO_UINT16( flashTime );
  buffer[2] = HI_UINT16( flashTime );

  if ( AF_DataRequest( &SampleApp_Flash_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_FLASH_CLUSTERID,
                       3,
                       buffer,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
*********************************************************************/
