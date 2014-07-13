// 节点类型
zgDeviceLogicalType
	ZG_DEVICETYPE_COORDINATOR
	ZG_DEVICETYPE_ROUTER
	ZG_DEVICETYPE_ENDDEVICE


// LED灯
/* LED1~LED4 -> P1_0~P1-3 */
HalLedBlink(HAL_LED_1, 0, 90, 500);			/* 周期为1s, 90%的时间亮 */
HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);		/* 开灯 */
HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);		/* 关灯 */
HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);	/* 反转 */				// HAL_TOGGLE_LED1();


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
aps_Group_t *grp = aps_FindGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
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


// Zigbee EP描述符
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


// 任务事件处理
XxxApp_ProcessEvent()
1. 系统事件 SYS_EVENT_MSG
消息
AF_DATA_CONFIRM_CMD			///< 调用 AF_DataRequest()函数数据请求成功的指示
AF_INCOMING_MSG_CMD			///< AF 信息输入指示
KEY_CHANGE					///< 键盘动作指示
ZDO_NEW_DSTADDR				///< 匹配描述符请求响应指示（例如：自动匹配）
ZDO_STATE_CHANGE			///< 网络状态改变指示(设备形成或加入网络)


// 字符串操作函数
sprintf(char *dst, char *format, ...)
int osal_strlen( char *pString )
void *osal_memcpy( void *dst, const void GENERIC *src, unsigned int len )
void *osal_memset( void *dest, uint8 value, int len )
uint8 osal_memcmp( const void GENERIC *src1, const void GENERIC *src2, unsigned int len )


// 串口支持
Alt+F7	C/C++Compiler/Preprocess
HAL_UART=TRUE		// 祛除其它宏


// 其它
LO_UINT16()
HI_UINT16()
BUILD_UINT16(低8位, 高8位)

LO_UINT8()
HI_UINT8()
BUILD_UINT8(高4位, 低4位)


/// 绑定[http://blog.csdn.net/tanqiuwei/article/details/7642716]
// 		自动绑定/服务发现/自动匹配
// 场景: 网络中有协调器存在，另外有两个节点A和B，这两个节点具有互补特性(即A节点的incluster是B节点的outcluster)
// 负责发消息的设备调用ZDP_MatchDescReq()广播个人公告
// 匹配的设备会自动作出响应
// 由ZDO处理和验证响应
// 负责发送消息的设备建立绑定表并保存绑定记录

//		按键绑定
// 场景: 网络中不一定有协调器存在，但是有A、B、C、D等多个节点，A性质是Outcluster，B、C、D的性质是Incluster，你可以通过按键策略来在一定时间内允许B、C、D中的任何一个开启被Match的功能
// 两个节点一定时间(16s)内都按键调用ZDP_EndDeviceBindReq()函数
