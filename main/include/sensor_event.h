#ifndef SENSOR_EVENT_h
#define SENSOR_EVENT_h

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SENSOR_EVENT);

typedef enum{
    SENSOR_EVENT_UNKNOWN = -1,
    SENSOR_EVENT_DRY,
    SENSOR_EVENT_WET,
}sensor_event_id_t;


#endif