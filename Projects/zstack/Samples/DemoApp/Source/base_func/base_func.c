/**@file base_func.c
 * @note 2012-2015 xxx Co., Ltd. All Right Reserved.
 * @brief 
 * 
 * @author 	 jiangpenghai
 * @date     2014/7/10
 * 
 * @note 
 * @note 历史记录: 
 * @note    2014/7/10 V1.0 jiangpenghai 创建
 * 
 * @warning  
 */

#include "base_func.h"


void uart_init(halUARTCBack_t SerialApp_CallBack)
{
	halUARTCfg_t uartConfig;
	
	uartConfig.configured           = TRUE;              // 2x30 don't care - see uart driver.
	uartConfig.baudRate             = SERIAL_APP_BAUD;
	uartConfig.flowControl          = FALSE;
	uartConfig.flowControlThreshold = SERIAL_APP_THRESH; // 2x30 don't care - see uart driver.
	uartConfig.rx.maxBufSize        = SERIAL_APP_RX_SZ;  // 2x30 don't care - see uart driver.
	uartConfig.tx.maxBufSize        = SERIAL_APP_TX_SZ;  // 2x30 don't care - see uart driver.
	uartConfig.idleTimeout          = SERIAL_APP_IDLE;   // 2x30 don't care - see uart driver.
	uartConfig.intEnable            = TRUE;              // 2x30 don't care - see uart driver.
	uartConfig.callBackFunc         = SerialApp_CallBack;
	HalUARTOpen(SERIAL_APP_PORT, &uartConfig);
}

