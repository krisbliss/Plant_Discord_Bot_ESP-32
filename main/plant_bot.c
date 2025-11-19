#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" // --- ADDED for FreeRTOS Queue ---
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "sensor_event.h" // Assuming this defines:
                          // ESP_EVENT_DECLARE_BASE(SENSOR_EVENT);
                          // enum { SENSOR_EVENT_WET, SENSOR_EVENT_DRY, SENSOR_EVENT_UNKNOWN };
#include "random_response.h"

#include "discord.h"
#include "discord/session.h"
#include "discord/message.h"
#include "estr.h"

#include "esp_adc/adc_oneshot.h" 

// program tag
static const char *TAG = "discord_bot";

/*** Setup GPOI pin 36 for reading Analog to Digital  ***/
static const adc_unit_t adc_unit = ADC_UNIT_1; 
static const adc_channel_t adc_channel = ADC_CHANNEL_0;
static const adc_atten_t adc_atten = ADC_ATTEN_DB_12;
static const adc_bitwidth_t adc_bitwidth = ADC_BITWIDTH_12;
static adc_oneshot_unit_handle_t adc1_handle;

// NOTE if, value in menu config is not set, then the default sensors threshold value is 1900
#ifdef CONFIG_ADC_THRESHOLD_VAL
    static const int ADC_THRESHOLD = CONFIG_ADC_THRESHOLD_VAL;
#else
    static const int ADC_THRESHOLD = 1900;
#endif

// Discord driver handle and 
static discord_handle_t bot;
const char* discord_id = CONFIG_DISCORD_CHANNEL_ID;

// Queue handle for the sensor events
static QueueHandle_t sensor_event_queue = NULL;



/*** Discord Call back ***/
static void bot_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    discord_event_data_t *data = (discord_event_data_t *)event_data;

    switch (event_id) {

        // connection event
        case DISCORD_EVENT_CONNECTED: {
            discord_session_t *session = (discord_session_t *)data->ptr;
            ESP_LOGI(TAG, "Bot %s#%s connected", session->user->username, session->user->discriminator);
        } break;


        // disconnect event
        case DISCORD_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Bot logged out");
            break;
        
        default:
            break;
    }
}

/*** Define Sensor event and callback ***/
ESP_EVENT_DEFINE_BASE(SENSOR_EVENT);
static void sensor_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    if(base != SENSOR_EVENT) {
        return;
    }
    // Send the event to seperate task queue with a copy of event_id. (0-tick wait, shouldn't fail)
    xQueueSend(sensor_event_queue, &event_id, 0);
}


// Dedicated worker task so that we can avoid stackoverflows having this work done in the default task loop
static void discord_sender_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Discord sender task started.");
    int event_id;

    while (1) {
        // puts task in blocking (aka non-ready state) until queue receives an event.
        if (xQueueReceive(sensor_event_queue, &event_id, portMAX_DELAY)) {

            char* msg_content = NULL;

            switch (event_id)
            {
            case SENSOR_EVENT_WET:
                ESP_LOGI(TAG, "Worker Task: Processing WET");
                msg_content = "Thank you :) ";
                break;

            case SENSOR_EVENT_DRY:
                ESP_LOGI(TAG, "Worker Task: Processing DRY");
                
                // pull possible resposne from random_response.h
                msg_content = pick_rand_response(responses);
                break;
                
            default:
                ESP_LOGW(TAG, "Worker Task: Unknown event %d", event_id);
                continue; // Skip unknown events
            }

            discord_message_t msg = {
                .content = msg_content,
                .channel_id = CONFIG_DISCORD_CHANNEL_ID
            };

            ESP_LOGI(TAG, "Worker Task: Sending message '%s'", msg_content);

            //IMPORTANT NOTE: discord_message_send() is a blocking i/o operation as it waits for network communication.
            esp_err_t err = discord_message_send(bot, &msg, NULL);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send sensor status message to Discord.");
            } else {
                ESP_LOGI(TAG, "Worker Task: Message sent successfully.");
            }
        }
    }
}


/*** ADC Polling Task  ***/
static void adc_polling_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting ADC polling task...");
    int current_state = SENSOR_EVENT_UNKNOWN;

    while (1) {
        int raw_value = 0;
        esp_err_t ret = adc_oneshot_read(adc1_handle, adc_channel, &raw_value);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Sensor Raw Value: %d", raw_value);

            int new_state = (raw_value >= ADC_THRESHOLD) ? SENSOR_EVENT_DRY : SENSOR_EVENT_WET;

            if (new_state != current_state) {
                ESP_LOGI(TAG, "ADC state change. State: %s", 
                         (new_state == SENSOR_EVENT_DRY) ? "DRY" : "WET");
                
                current_state = new_state;

                esp_event_post(SENSOR_EVENT, current_state, NULL, 0, portMAX_DELAY);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    // default loop to post sensor event to
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // example program from ESP-IDF lib to connecte to wifi
    ESP_ERROR_CHECK(example_connect());

    // Initialize ADC
    ESP_LOGI(TAG, "Initializing ADC one-shot driver...");
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = adc_unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = adc_atten,
        .bitwidth = adc_bitwidth,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc_channel, &chan_config));
    ESP_LOGI(TAG, "ADC one-shot driver initialized.");

    // define sensor_event queue
    sensor_event_queue = xQueueCreate(
        5,                  // Queue depth
        sizeof(int)         // Item size (our event ID)
    );

    if (sensor_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor event queue!");
    }


    // Register sensor event handler to default loop
    ESP_ERROR_CHECK(esp_event_handler_register(
        SENSOR_EVENT,
        ESP_EVENT_ANY_ID,
        &sensor_event_handler,
        NULL
    ));
    ESP_LOGI(TAG, "Sensor event handler registered.");

    // Discord bot setup as event loop
    discord_config_t cfg = { .intents = DISCORD_INTENT_GUILD_MESSAGES | DISCORD_INTENT_MESSAGE_CONTENT };
    bot = discord_create(&cfg);
    ESP_ERROR_CHECK(discord_register_events(bot, DISCORD_EVENT_ANY, bot_event_handler, NULL));
    ESP_ERROR_CHECK(discord_login(bot));

    // Start the ADC polling task
    xTaskCreate(
        adc_polling_task,       // Function to run
        "adc_polling_task",     // Name
        4096,                   // 4kb
        NULL,                   // Params
        5,                      // Priority
        NULL                    // Handle
    );

    // Start discord worker task
    xTaskCreate(
        discord_sender_task,
        "discord_sender_task",
        8192,                   // 8KB (large stack)
        NULL,
        5,
        NULL
    );
}