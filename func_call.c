// LED灯
/* LED1~LED4 -> P1_0~P1-3 */
HalLedBlink(HAL_LED_1, 0, 90, 500);			/* 周期为1s, 90%的时间亮 */
HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);		/* 开灯 */
HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);		/* 关灯 */


// 发送广播信息
/* 广播地址 */
afAddrType_t SampleApp_Broadcast_DstAddr;
SampleApp_Broadcast_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
SampleApp_Broadcast_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
SampleApp_Broadcast_DstAddr.addr.shortAddr = 0xFFFF;
/* 应用对象(终端描述符) */
endPointDesc_t SampleApp_epDesc;
SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
SampleApp_epDesc.task_id = &SampleApp_TaskID;
SampleApp_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
SampleApp_epDesc.latencyReq = noLatencyReqs;		/* 无延时 */
uint8 buffer[6] = "hello";
if (afStatus_SUCCESS == AF_DataRequest( &SampleApp_Broadcast_DstAddr, &SampleApp_epDesc,
					SAMPLEAPP_PERIODIC_CLUSTERID,
					1,
					buffer,
					&SampleApp_TransID,
					AF_DISCV_ROUTE,
					AF_DEFAULT_RADIUS ))


// 加入/退出组
aps_Group_t SampleApp_Group;
SampleApp_Group.ID = 0x0001;
osal_memcpy( SampleApp_Group.name, "Group 1", 7  );
aps_Group_t *grp;
grp = aps_FindGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );		/* 查看加入了哪个组 */
if ( grp )
{
	/* 退出组 */
	aps_RemoveGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
}
else
{
	/* 加入组 */
	aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
}


// 发送消息到组
afAddrType_t SampleApp_Group_DstAddr;
SampleApp_Group_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
SampleApp_Group_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
SampleApp_Group_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;
uint8 buffer[6] = "hello";
if (afStatus_SUCCESS = AF_DataRequest( &SampleApp_Group_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_FLASH_CLUSTERID,
                       6,
                       buffer,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ))


// 开启定时器中断(中断发生后需要刷新)
#define SAMPLEAPP_SEND_PERIODIC_MSG_EVT       0x0001		///< 定时器中断events
#define SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT   5000			///< 定时器超时时间
osal_start_timerEx( SampleApp_TaskID,
						SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
						SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );


// Zigbee终端简单描述
const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
	SAMPLEAPP_ENDPOINT,              //  终端号
	SAMPLEAPP_PROFID,                //  定义了这个终端上支持的Profile ID（剖面ID）， ID最好遵循由ZigBee联盟的分配
	SAMPLEAPP_DEVICEID,              //  终端支持的设备ID，ID最好遵循ZigBee联盟的分配
	SAMPLEAPP_DEVICE_VERSION,        //  此终端上设备执行的设备描述的版本：0x00为Version 1.0
	SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
	SAMPLEAPP_MAX_CLUSTERS,          //  终端支持的输入簇数目
	(cId_t *)SampleApp_ClusterList,  //  指向输入Cluster ID列表的指针
	SAMPLEAPP_MAX_CLUSTERS,          //  终端支持的输出簇数目
	(cId_t *)SampleApp_ClusterList   //  指向输出Cluster ID列表的指针
};
// 根据终端找终端描述符
endPointDesc_t *afFindEndPointDesc( byte endPoint );


// 串口操作
HAL_UART = TRUE