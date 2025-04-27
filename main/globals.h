#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// --------------------------------------------
#define FIRMWARE_VERSION				"v0.01"
// --------------------------------------------

//GPIO
#define GPIO_OUTPUT_PIN_SEL(X)  		((1ULL<<X))

//Timeouts
#define TIMEOUT_SHORT					50
#define TIMEOUT_NORMAL					100
#define TIMEOUT_LONG					1000

//WDT
#define WDT_TIMEOUT_S					5
#define US_TO_S							1000000

#endif