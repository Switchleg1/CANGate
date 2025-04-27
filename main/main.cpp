#include "globals.h"
#include "twai.h"

// ---------------- DEFINES ------------------------- //

#define TAG		"Main"


void mainInit()
{
	Twai.init();
	Twai.start(TWAI_BAUD_500);
}

void mainLoop()
{

}

extern "C" void app_main(void)
{
	ESP_LOGI(TAG, "app_main: application starting");

#if !CONFIG_ESP_TASK_WDT
	esp_task_wdt_config_t wdt_config = {
		.timeout_ms = WDT_TIMEOUT_S * 1000,
		.idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
		.trigger_panic = true
	};
    // If the WDT was not initialized automatically on startup, manually intialize it now
    ESP_ERROR_CHECK(esp_task_wdt_init(&wdt_config));
    ESP_LOGI(TAG, "app_main: WDT initialized");
#endif

	//subscribe to WDT
	ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
	ESP_ERROR_CHECK(esp_task_wdt_status(NULL));

	mainInit();

	ESP_LOGI(TAG, "app_main: starting main loop");

	//main loop
	while (1) {
		mainLoop();

		//reset watchdog and wait for next tick
		esp_task_wdt_reset();
		vTaskDelay(1);
	}
}