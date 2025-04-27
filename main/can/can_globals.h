#ifndef CAN_GLOBALS_H
#define CAN_GLOBALS_H

#include "globals.h"

// Header we expect to send/receive on UDS packets
typedef struct {
	uint32_t	id;
	uint16_t	size;
	uint8_t*	buffer;
} UDSPacket_t;

//defines
typedef int16_t								bool16;
#define cMUTEX(x)							xSemaphoreTake(x, 0)
#define tMUTEX(x)							xSemaphoreTake(x, portMAX_DELAY)
#define rMUTEX(x)							xSemaphoreGive(x)
#define SafeFree(x, y)						if(x) { free(x); x = NULL; y = true; }
#define SafeDeleteSemaphore(x, y)			if(x) { vSemaphoreDelete(x); x = NULL; y = true; }
#define SafeDeleteQueue(x, y)				if(x) { vQueueDelete(x); x = NULL; y = true; }

//CAN
#define CAN_INTERNAL_RX_BUFFER_SIZE			1024
#define CAN_INTERNAL_TX_BUFFER_SIZE			1024
#define	CAN0_TX_PORT						GPIO_NUM_5
#define CAN0_RX_PORT						GPIO_NUM_4
#define	CAN1_TX_PORT						GPIO_NUM_6
#define CAN1_RX_PORT						GPIO_NUM_7
#define CAN_CLK_IO							TWAI_IO_UNUSED
#define CAN_BUS_IO							TWAI_IO_UNUSED
#define	CAN_MODE							TWAI_MODE_NORMAL
#define CAN_ALERTS							TWAI_ALERT_ABOVE_ERR_WARN | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED
#define CAN_CLK_DIVIDER						0
#define CAN_FLAGS							ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM
#define CAN_FILTER							TWAI_FILTER_CONFIG_ACCEPT_ALL()
#define CAN_RETRY_COUNT						1

#define CAN_TASK_STACK_SIZE					3072
#define CAN_TWAI_TASK_PRIO 			 		2


#endif