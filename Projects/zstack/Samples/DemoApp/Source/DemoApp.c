/**@file DemoApp.c
 * @note 2012-2015 HangZhou xxx Co., Ltd. All Right Reserved.
 * @brief 
 * 
 * @author 	 jiangpenghai
 * @date     2014/7/12
 * 
 * @note 
 * @note 历史记录: 
 * @note    2014/7/12 V1.0 jiangpenghai 创建
 * 
 * @warning  
 */

#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "DemoApp.h"

#include "OnBoard.h"

#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"


const cId_t DemoApp_ClusterList[DemoApp_MAX_CLUSTERS] =
{
	DemoApp_1_CLUSTERID,
	DemoApp_2_CLUSTERID
};

const SimpleDescriptionFormat_t DemoApp_SimpleDesc =
{
	DemoApp_ENDPOINT,
	DemoApp_PROFID,
	DemoApp_DEVICEID,
	DemoApp_DEVICE_VERSION,
	DemoApp_FLAGS,
	DemoApp_MAX_CLUSTERS,
	(cId_t *)DemoApp_ClusterList,
	DemoApp_MAX_CLUSTERS,
	(cId_t *)DemoApp_ClusterList
};

uint8 DemoApp_TaskID;
endPointDesc_t DemoApp_epDesc;
devStates_t DemoApp_NwkState;
uint8 DemoApp_TransID;

afAddrType_t DemoApp_Broadcast_DstAddr;
afAddrType_t DemoApp_Group_DstAddr;

aps_Group_t DemoApp_Group;

uint8 DemoAppFlashCounter = 0;

void DemoApp_HandleKeys(uint8 shift, uint8 keys);
void DemoApp_MessageMSGCB(afIncomingMSGPacket_t *pckt);
void DemoApp_SendBroadcastMessage( void );
void DemoApp_SendMulticastMessage();

void DemoApp_UartRecv(void);
void DemoApp_UartCallBack(uint8 port, uint8 event);

/**
 * @brief		EP任务初始化
 * @param[in]	任务ID
 * @param[out] 	void
 * @return		void
 */
void DemoApp_Init(uint8 task_id)
{
	DemoApp_TaskID = task_id;
	DemoApp_NwkState = DEV_INIT;
	DemoApp_TransID = 0;

	/* 填充EP描述符 */
	DemoApp_epDesc.endPoint = DemoApp_ENDPOINT;
	DemoApp_epDesc.task_id = &DemoApp_TaskID;
	DemoApp_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&DemoApp_SimpleDesc;
	DemoApp_epDesc.latencyReq = noLatencyReqs;

	/* 在AF层登记EP任务 */
	afRegister(&DemoApp_epDesc);

	/* 注册按键消息 */
	RegisterForKeys(DemoApp_TaskID);

	/* 初始化串口 */
	uart_init(DemoApp_UartCallBack);
	uint8 *send_buf = "SerialApp_Init\n";  
	HalUARTWrite(SERIAL_APP_PORT, send_buf, osal_strlen((char *)send_buf));

	/* 初始化广播地址 */
	DemoApp_Broadcast_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
	DemoApp_Broadcast_DstAddr.endPoint = DemoApp_ENDPOINT;
	DemoApp_Broadcast_DstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
	/* 初始化组地址 */
	DemoApp_Group_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
	DemoApp_Group_DstAddr.endPoint = DemoApp_ENDPOINT;
	DemoApp_Group_DstAddr.addr.shortAddr = DemoApp_GROUP_ID;

	/* 设备默认加到Group 1 */
	DemoApp_Group.ID = DemoApp_GROUP_ID;
	osal_memcpy(DemoApp_Group.name, "Group 1", 7 );
	aps_AddGroup(DemoApp_ENDPOINT, &DemoApp_Group);
}

/**
 * @brief		EP任务事件处理函数
 * @param[in]	task_id: 	EP任务ID
 * @param[in]	events: 	EP任务事件
 * @param[out] 	void
 * @return		未处理的事件
 */
uint16 DemoApp_ProcessEvent(uint8 task_id, uint16 events)
{
	afIncomingMSGPacket_t *MSGpkt;

	afDataConfirm_t *afDataConfirm;
	ZStatus_t sentStatus;

	/* 系统事件 */
	if ( events & SYS_EVENT_MSG )
	{
		MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(DemoApp_TaskID);
		while (MSGpkt)
		{
			switch (MSGpkt->hdr.event)
			{
				/* 按键消息 */
				case KEY_CHANGE:
					DemoApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
					break;

				/* RF接收到数据 */
				case AF_INCOMING_MSG_CMD:
					DemoApp_MessageMSGCB(MSGpkt);
					break;

				/* 在网络中状态改变 */
				case ZDO_STATE_CHANGE:
					DemoApp_NwkState = (devStates_t)(MSGpkt->hdr.status);

					if (DEV_ZB_COORD == DemoApp_NwkState)
					{
						HalLedBlink(HAL_LED_1, 1, 90, 500);
						osal_start_timerEx(DemoApp_TaskID,
					          DemoApp_SEND_PERIODIC_MSG_EVT,
					          DemoApp_SEND_PERIODIC_MSG_TIMEOUT);
					}
					else if (DEV_ROUTER == DemoApp_NwkState)
					{
						HalLedBlink(HAL_LED_1, 2, 80, 500);
					}
					else if (DEV_END_DEVICE == DemoApp_NwkState)
					{
						HalLedBlink(HAL_LED_1, 3, 70, 500);
					}
					break;

				/* 匹配描述符请求响应,如自动匹配 */
				case ZDO_NEW_DSTADDR:
					break;

				/* 调用 AF_DataRequest()函数发送数据请求成功的指示 */
				case AF_DATA_CONFIRM_CMD:
					afDataConfirm = (afDataConfirm_t *)MSGpkt;
					sentStatus = afDataConfirm->hdr.status;
					if ( sentStatus != ZSuccess )
					{
						;		// 数据没有发送成功
					}
					break;
					
				default:
					break;
			}

			osal_msg_deallocate((uint8 *)MSGpkt);
			MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( DemoApp_TaskID );
		}

		return (events ^ SYS_EVENT_MSG);
	}

	/* 定时器中断事件 */
	if (events & DemoApp_SEND_PERIODIC_MSG_EVT)
	{
		DemoApp_SendBroadcastMessage();

		/* 重置定时器 */
		osal_start_timerEx(DemoApp_TaskID, DemoApp_SEND_PERIODIC_MSG_EVT,
			(DemoApp_SEND_PERIODIC_MSG_TIMEOUT + (osal_rand() & 0x00FF)));

		return (events ^ DemoApp_SEND_PERIODIC_MSG_EVT);
	}

	return 0;
}

/**
 * @brief		按键消息处理函数
 * @param[in]	shift: 	双击按键号
 * @param[in]	keys: 	单击按键号
 * @param[out] 	void
 * @return		void
 */
void DemoApp_HandleKeys(uint8 shift, uint8 keys)
{
	uint8 send_buf[5] = {0};  

	if (keys & HAL_KEY_SW_6)
	{
		HalLedBlink(HAL_LED_1, 1, 10, 500);
		osal_memcpy(send_buf, "SW_6", osal_strlen("SW_6"));
		HalUARTWrite(SERIAL_APP_PORT, send_buf, osal_strlen((char *)send_buf));
	}
}

/**
 * @brief		RF数据接收处理函数
 * @param[in]	pkt: 	RF数据包
 * @param[out] 	void
 * @return		void
 */
void DemoApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
{
	uint8 send_buf[5] = {0};

	switch (pkt->clusterId)
	{
		case DemoApp_1_CLUSTERID:
			/* 周期性收到广播数据 */
			HalLedBlink(HAL_LED_1, 1, 10, 500);
			
			osal_memcpy(send_buf, pkt->cmd.Data, pkt->cmd.DataLength);
			HalUARTWrite(SERIAL_APP_PORT, send_buf, osal_strlen((char *)send_buf));
			break;

		case DemoApp_2_CLUSTERID:
			/* 周期性收到多播数据 */
			HalLedBlink(HAL_LED_1, 1, 10, 500);
			
			osal_memcpy(send_buf, pkt->cmd.Data, pkt->cmd.DataLength);
			HalUARTWrite(SERIAL_APP_PORT, send_buf, osal_strlen((char *)send_buf));
			break;
	}
}

/**
 * @brief		周期性发送消息
 * @param[in]	void
 * @param[out] 	void
 * @return		void
 */
void DemoApp_SendPeriodicMessage(void)
{
	DemoApp_SendMulticastMessage();

	return;
}

/**
 * @brief		发送广播信息
 * @param[in]	void
 * @param[out] 	void
 * @return		void
 */
void DemoApp_SendBroadcastMessage()
{
	uint8 *sendBuf = "{1}";

	if (afStatus_SUCCESS == AF_DataRequest(&DemoApp_Broadcast_DstAddr, &DemoApp_epDesc,
							DemoApp_1_CLUSTERID,
							3,
							sendBuf,
							&DemoApp_TransID,
							AF_DISCV_ROUTE,
							AF_DEFAULT_RADIUS))
	{
		HalLedBlink(HAL_LED_1, 1, 5, 500);
	}
	else
	{
		;
	}

	return;
}

/**
 * @brief		发送组播信息
 * @param[in]	void
 * @param[out] 	void
 * @return		void
 */
void DemoApp_SendMulticastMessage()
{
	uint8 *sendBuf = "{1}";

	if (afStatus_SUCCESS == AF_DataRequest(&DemoApp_Group_DstAddr, &DemoApp_epDesc,
							DemoApp_2_CLUSTERID,
							3,
							sendBuf,
							&DemoApp_TransID,
							AF_DISCV_ROUTE,
							AF_DEFAULT_RADIUS))
	{
		HalLedBlink(HAL_LED_1, 1, 5, 500);
	}
	else
	{
		;
	}

	return;
}

/**
 * @brief		串口接收函数
 * @param[in]	void
 * @param[out] 	void
 * @return		void
 */
void DemoApp_UartRecv(void)
{
	uint8 rx_buffer[SERIAL_APP_TX_MAX+1] = {0};
	uint8 tx_buffer[SERIAL_APP_TX_MAX+1] = {0};
	uint16 rx_len = 0;

	if (rx_len = HalUARTRead(SERIAL_APP_PORT, rx_buffer, SERIAL_APP_TX_MAX))
	{
		sprintf((char *)tx_buffer, "Recv %d bytes: %s\n", rx_len, (char *)rx_buffer);
		HalUARTWrite(SERIAL_APP_PORT, tx_buffer, osal_strlen((char *)tx_buffer));
	}

	return;
}

/**
 * @brief		串口回调函数
 * @param[in]	port:	
 * @param[in]	event:	串口事件
 * @param[out] 	void
 * @return		void
 */
void DemoApp_UartCallBack(uint8 port, uint8 event)
{
	if (event & (HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT))
	{
		DemoApp_UartRecv();
	}

	return;
}

