#include "globals.h"
#include "can_globals.h"
#include "twai.h"

#define TAG		"TWAI"

TwaiBusData_t		CTwai::canBus[TWAI_BUS_COUNT] = {
	{
		.thisBus			= TWAI_IN_BUS,
		.outBus				= TWAI_OUT_BUS,
		.twaiConfig			= {
			.controller_id	= TWAI_IN_BUS,
			.mode			= CAN_MODE,
			.tx_io			= CAN0_TX_PORT,
			.rx_io			= CAN0_RX_PORT,
			.clkout_io		= CAN_CLK_IO,
			.bus_off_io		= CAN_BUS_IO,
			.tx_queue_len	= CAN_INTERNAL_TX_BUFFER_SIZE,
			.rx_queue_len	= CAN_INTERNAL_RX_BUFFER_SIZE,
			.alerts_enabled = CAN_ALERTS,
			.clkout_divider = CAN_CLK_DIVIDER,
			.intr_flags		= CAN_FLAGS
		},
		.receiveTaskMutex	= NULL,
		.alertTaskMutex		= NULL,
		.busOffMutex		= NULL,
		.twaiHandle			= NULL,
		.onMessageReceived	= NULL,
	},
	{
		.thisBus			= TWAI_OUT_BUS,
		.outBus				= TWAI_IN_BUS,
		.twaiConfig = {
			.controller_id	= TWAI_OUT_BUS,
			.mode			= CAN_MODE,
			.tx_io			= CAN1_TX_PORT,
			.rx_io			= CAN1_RX_PORT,
			.clkout_io		= CAN_CLK_IO,
			.bus_off_io		= CAN_BUS_IO,
			.tx_queue_len	= CAN_INTERNAL_TX_BUFFER_SIZE,
			.rx_queue_len	= CAN_INTERNAL_RX_BUFFER_SIZE,
			.alerts_enabled = CAN_ALERTS,
			.clkout_divider = CAN_CLK_DIVIDER,
			.intr_flags		= CAN_FLAGS
		},
		.receiveTaskMutex	= NULL,
		.alertTaskMutex		= NULL,
		.busOffMutex		= NULL,
		.twaiHandle			= NULL,
		.onMessageReceived	= NULL,
	},
};
TwaiBaud_t			CTwai::baudRate				= TWAI_BAUD_500;
TwaiState_t			CTwai::currentState			= TWAI_DEINIT;

CTwai Twai;

//----------------TWAI Public Functions---------------------------//

TwaiReturn_t CTwai::init()
{
	if (currentState != TWAI_DEINIT) {
		ESP_LOGI(TAG, "init: invalid state");

		return TWAI_INVALID_STATE;
	}

	//init mutexes
	for (uint8_t i = 0; i < TWAI_BUS_COUNT; i++) {
		canBus[i].receiveTaskMutex	= xSemaphoreCreateMutex();
		canBus[i].alertTaskMutex	= xSemaphoreCreateMutex();
		canBus[i].busOffMutex		= xSemaphoreCreateMutex();
	}

	ESP_LOGI(TAG, "init: complete");

	currentState = TWAI_INIT;

	return TWAI_OK;
}

TwaiReturn_t CTwai::deinit()
{
	if (currentState != TWAI_INIT) {
		ESP_LOGI(TAG, "deinit: invalid state");

		return TWAI_INVALID_STATE;
	}

	bool16 didDeInit = false;
	for (uint8_t i = 0; i < TWAI_BUS_COUNT; i++) {
		SafeDeleteSemaphore(canBus[i].receiveTaskMutex, didDeInit)
		SafeDeleteSemaphore(canBus[i].alertTaskMutex, didDeInit)
		SafeDeleteSemaphore(canBus[i].busOffMutex, didDeInit)
	}

	currentState = TWAI_DEINIT;

	if (didDeInit) {
		ESP_LOGI(TAG, "deinit: complete");

		return TWAI_OK;
	}

	ESP_LOGD(TAG, "deinit: error");

	return TWAI_ERROR;
}

TwaiReturn_t CTwai::start(TwaiBaud_t baud)
{
	if (currentState != TWAI_INIT) {
		ESP_LOGI(TAG, "start: invalid state");

		return TWAI_INVALID_STATE;
	}

	// "TWAI" is knockoff CAN. Install TWAI driver.
	if (baud > TWAI_BAUD_1MB) {
		baud = TWAI_BAUD_500;
	}
	baudRate = baud;

	for (uint8_t i = 0; i < TWAI_BUS_COUNT; i++) {
		const twai_filter_config_t	can_filter_config = CAN_FILTER;
		twai_timing_config_t can_timing_config = getBaudConfig(baud);
		ESP_ERROR_CHECK(twai_driver_install_v2(&canBus[i].twaiConfig, &can_timing_config, &can_filter_config, &canBus[i].twaiHandle));
		ESP_ERROR_CHECK(twai_start_v2(canBus[i].twaiHandle));
	}

	currentState = TWAI_RUNNING;

	for (uint8_t i = 0; i < TWAI_BUS_COUNT; i++) {
		ESP_ERROR_CHECK(xTaskCreate(alertTask,   "twai_alert_task",   CAN_TASK_STACK_SIZE, (void*)&canBus[i], CAN_TWAI_TASK_PRIO, NULL));
		ESP_ERROR_CHECK(xTaskCreate(receiveTask, "twai_receive_task", CAN_TASK_STACK_SIZE, (void*)&canBus[i], CAN_TWAI_TASK_PRIO, NULL));
	}

	ESP_LOGI(TAG, "start: complete");

	return TWAI_OK;
}

TwaiReturn_t CTwai::stop()
{
	if (currentState != TWAI_RUNNING) {
		ESP_LOGI(TAG, "stop: invalid state");

		return TWAI_INVALID_STATE;
	}

	currentState = TWAI_STOPPING;

	for (uint8_t i = 0; i < TWAI_BUS_COUNT; i++) {
		tMUTEX(canBus[i].receiveTaskMutex);
		rMUTEX(canBus[i].receiveTaskMutex);

		tMUTEX(canBus[i].alertTaskMutex);
		rMUTEX(canBus[i].alertTaskMutex);

		ESP_LOGI(TAG, "stop: bus[%u] tasks stopped", i);

		twai_stop_v2(canBus[i].twaiHandle);
		twai_driver_uninstall_v2(canBus[i].twaiHandle);

		ESP_LOGI(TAG, "stop: bus[%u] driver uninstalled", i);
	}

	currentState = TWAI_INIT;

	return TWAI_OK;
}

TwaiReturn_t CTwai::send(TwaiBus_t bus, twai_message_t * msg)
{
	tMUTEX(canBus[bus].busOffMutex);
	rMUTEX(canBus[bus].busOffMutex);

	int16_t retry = -1;
	while (twai_transmit_v2(canBus[bus].twaiHandle, msg, pdMS_TO_TICKS(TIMEOUT_NORMAL)) != ESP_OK && ++retry < CAN_RETRY_COUNT) {
		vTaskDelay(1);
	}

	return retry < CAN_RETRY_COUNT ? TWAI_OK : TWAI_ERROR;
}

const char* CTwai::baudString(TwaiBaud_t baud)
{
	switch (baud) {
	case TWAI_BAUD_25:
		return "25KB";

	case TWAI_BAUD_50:
		return "50KB";

	case TWAI_BAUD_100:
		return "100KB";

	case TWAI_BAUD_125:
		return "125KB";

	case TWAI_BAUD_250:
		return "250KB";

	case TWAI_BAUD_500:
		return "500KB";

	case TWAI_BAUD_800:
		return "800KB";

	case TWAI_BAUD_1MB:
		return "1MB";
	}

	return "500KB";
}


//---------- Private Functions --------------//

void IRAM_ATTR CTwai::alertTask(void* arg)
{
	TwaiBusData_t* busData = (TwaiBusData_t*)arg;

	//subscribe to WDT
	ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
	ESP_ERROR_CHECK(esp_task_wdt_status(NULL));

	tMUTEX(busData->alertTaskMutex);
	ESP_LOGI(TAG, "alertTask: started");
	uint32_t alerts;
	while (currentState == TWAI_RUNNING) {
		ESP_LOGD(TAG, "alertTask: memory free [%" PRIu16 "]", uxTaskGetStackHighWaterMark(NULL));
		if (twai_read_alerts_v2(busData->twaiHandle, &alerts, pdMS_TO_TICKS(TIMEOUT_LONG)) == ESP_OK) {
			if (alerts & TWAI_ALERT_ABOVE_ERR_WARN) {
				ESP_LOGI(TAG, "alertTask: surpassed error warning limit");
			}

			if (alerts & TWAI_ALERT_ERR_PASS) {
				ESP_LOGI(TAG, "alertTask: entered error passive state");
			}

			if (alerts & TWAI_ALERT_BUS_OFF) {
				ESP_LOGI(TAG, "alertTask: bus off state");
				if (xSemaphoreTake(busData->busOffMutex, pdMS_TO_TICKS(TIMEOUT_NORMAL)) == pdTRUE) {
					ESP_LOGI(TAG, "alertTask: initiate bus recovery");
					ESP_ERROR_CHECK(twai_initiate_recovery_v2(busData->twaiHandle));    //Needs 128 occurrences of bus free signal
				}
			}

			if (alerts & TWAI_ALERT_BUS_RECOVERED) {
				ESP_ERROR_CHECK(twai_start_v2(busData->twaiHandle));
				xSemaphoreGive(busData->busOffMutex);
				ESP_LOGI(TAG, "alertTask: bus recovered");
			}
		}

		//reset the WDT and yield to tasks
		esp_task_wdt_reset();
		taskYIELD();
	}
	ESP_LOGI(TAG, "alertTask: stopped");
	rMUTEX(busData->alertTaskMutex);

	//unsubscribe to WDT and delete task
	ESP_ERROR_CHECK(esp_task_wdt_delete(NULL));
	vTaskDelete(NULL);
}

void IRAM_ATTR CTwai::receiveTask(void* arg)
{
	TwaiBusData_t* busData = (TwaiBusData_t*)arg;

	//subscribe to WDT
	ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
	ESP_ERROR_CHECK(esp_task_wdt_status(NULL));

	tMUTEX(busData->receiveTaskMutex);
	ESP_LOGI(TAG, "receiveTask: started");
	twai_message_t twaiRXmsg;
	while (currentState == TWAI_RUNNING) {
		if (twai_receive_v2(busData->twaiHandle, &twaiRXmsg, pdMS_TO_TICKS(TIMEOUT_LONG)) == ESP_OK) {
			ESP_LOGD(TAG, "receiveTask: received [%" PRIX32 "] and length [%" PRIu16 "]", twaiRXmsg.identifier, twaiRXmsg.data_length_code);

			if (busData->onMessageReceived) busData->onMessageReceived(busData->thisBus, busData->outBus, &twaiRXmsg);
			else send(busData->outBus, &twaiRXmsg);
		}

		//reset the WDT and yield to tasks
		esp_task_wdt_reset();
		taskYIELD();
	}
	ESP_LOGI(TAG, "receiveTask: stopped");
	rMUTEX(busData->receiveTaskMutex);

	//unsubscribe to WDT and delete task
	ESP_ERROR_CHECK(esp_task_wdt_delete(NULL));
	vTaskDelete(NULL);
}

twai_timing_config_t CTwai::getBaudConfig(TwaiBaud_t baud)
{
	switch (baud) {
	case TWAI_BAUD_25:
		return TWAI_TIMING_CONFIG_25KBITS();

	case TWAI_BAUD_50:
		return TWAI_TIMING_CONFIG_50KBITS();

	case TWAI_BAUD_100:
		return TWAI_TIMING_CONFIG_100KBITS();

	case TWAI_BAUD_125:
		return TWAI_TIMING_CONFIG_125KBITS();

	case TWAI_BAUD_250:
		return TWAI_TIMING_CONFIG_250KBITS();

	case TWAI_BAUD_500:
		return TWAI_TIMING_CONFIG_500KBITS();

	case TWAI_BAUD_800:
		return TWAI_TIMING_CONFIG_800KBITS();

	case TWAI_BAUD_1MB:
		return TWAI_TIMING_CONFIG_1MBITS();
	}

	return TWAI_TIMING_CONFIG_500KBITS();
}