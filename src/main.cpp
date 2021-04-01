// Copyright (c) 2020 Guy Turcotte
//
// MIT License. Look at file licenses.txt for details.

#define __GLOBAL__ 1
#include "global.hpp"
  
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logging.hpp"

#include "controllers/books_dir_controller.hpp"
#include "controllers/app_controller.hpp"
#include "models/fonts.hpp"
#include "models/epub.hpp"
#include "models/config.hpp"
//#include "screen.hpp" // Will be replaced by EPDiy
//#include "inkplate_platform.hpp"
#include "helpers/unzip.hpp"
#include "viewers/msg_viewer.hpp"
#include "pugixml.hpp"
#include "nvs_flash.h"
#include "alloc.hpp"
#include "esp.hpp"
// SPIFFS
#include "esp_spiffs.h"
// EPDiy
#include "epd_driver.h"
uint8_t *framebuffer;

#include <stdio.h>

  static constexpr char const * TAG = "main";

  void 
  mainTask(void * params) 
  {
    LOG_I("EPub-Inkplate Startup.");
    
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err != ESP_OK) {
      if ((nvs_err == ESP_ERR_NVS_NO_FREE_PAGES) || (nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        LOG_D("Erasing NVS Partition... (Because of %s)", esp_err_to_name(nvs_err));
        if ((nvs_err = nvs_flash_erase()) == ESP_OK) {
          nvs_err = nvs_flash_init();
        }
      }
    } 
    if (nvs_err != ESP_OK) LOG_E("NVS Error: %s", esp_err_to_name(nvs_err));

    #if DEBUGGING
      for (int i = 10; i > 0; i--) {
        printf("\r%02d ...", i);
        fflush(stdout);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
      printf("\n"); fflush(stdout);
    #endif 

  // TEST clear epaper
  epd_poweron();
  epd_clear();

  bool config_err = !config.read();
  if (config_err) LOG_E("Config Error.");

    #if DEBUGGING
      config.show();
    #endif

    pugi::set_memory_management_functions(allocate, free);

    page_locs.setup();

    /**
     * SPIFFS
     */
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/",
      .partition_label = NULL,
      .max_files = 15
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }


    if (fonts.setup()) {
      
      /* Screen::Orientation    orientation;
      Screen::PixelResolution resolution;
      config.get(Config::Ident::ORIENTATION,      (int8_t *) &orientation);
      config.get(Config::Ident::PIXEL_RESOLUTION, (int8_t *) &resolution);
      screen.setup(resolution, orientation); */

      event_mgr.setup();
      event_mgr.set_orientation(Screen::Orientation::LEFT);

      if (nvs_err != ESP_OK) {
        msg_viewer.show(MsgViewer::ALERT, false, true, "Hardware Problem!",
          "Failed to initialise NVS Flash (%s). Entering Deep Sleep. Press a key to restart.",
           esp_err_to_name(nvs_err)
        );

        ESP::delay(500);
        inkplate_platform.deep_sleep();
      }
  
      /* if (inkplate_err) {
        msg_viewer.show(MsgViewer::ALERT, false, true, "Hardware Problem!",
          "Unable to initialize the InkPlate drivers. Entering Deep Sleep. Press a key to restart."
        );
        ESP::delay(500);
        inkplate_platform.deep_sleep();
      } */

      if (config_err) {
        msg_viewer.show(MsgViewer::ALERT, false, true, "Configuration Problem!",
          "Unable to read/save configuration file. Entering Deep Sleep. Press a key to restart."
        );
        ESP::delay(500);
        epd_poweroff();
      }

      books_dir_controller.setup();
      LOG_D("Initialization completed");
      app_controller.start();
    }
    /* else {
      LOG_E("Font loading error.");
      msg_viewer.show(MsgViewer::ALERT, false, true, "Font Loading Problem!",
        "Unable to read required fonts. Entering Deep Sleep. Press a key to restart."
      );
      ESP::delay(500);
      inkplate_platform.deep_sleep();
    }  */

    #if DEBUGGING
      while (1) {
        printf(". ");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
      }
    #endif
  }

  #define STACK_SIZE 40000

  extern "C" {

    void 
    app_main(void)
    {
      epd_init(EPD_OPTIONS_DEFAULT);
      epd_set_rotation(1);
      framebuffer = epd_init_hl(EPD_BUILTIN_WAVEFORM);     

      TaskHandle_t xHandle = NULL;
      xTaskCreate(mainTask, "mainTask", STACK_SIZE, (void *) 1, configMAX_PRIORITIES - 1, &xHandle);
      configASSERT(xHandle);
    }

  } // extern "C"
