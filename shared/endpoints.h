#ifndef ENDPOINTS_H_

//Endpoint IDs
#define EP_DEVICE_DESCRIPTOR       0UL
#define EP_POWER_SWITCH            1UL
#define EP_POWER_METER             2UL
#define EP_TEMPERATURE             3UL
#define EP_BUTTON                  4UL
#define EP_HUMIDITY                5UL
#define EP_PRESSURE                6UL
#define EP_ENERGY_METER_TOTAL      7UL
#define EP_ENERGY_METER            8UL
#define EP_SM_CONTROL              9UL
#define EP_SM_UP_RECEIVER          10UL
#define EP_SM_UP_ACKNAK            11UL
#define EP_SM_RESET_ID             12UL
#define EP_POWER_DEFAULT_STATE	   13UL
#define EP_ANALOGREAD              22UL
#define EP_SHUTTER                 23UL
#define EP_HEXAPUSH_PRESSED        24UL
#define EP_HEXAPUSH_CLICKED        25UL
#define EP_PRESENCE_DETECTOR       26UL
#define EP_HEXONOFF_SET            27UL
#define EP_HEXONOFF_TOGGLE         28UL
#define EP_LIGHTSENSOR             29UL
#define EP_IR_RECEIVER             30UL
#define EP_LIVENESS                31UL
#define EP_EXT_DEV_DESC_1          32UL
#define EP_GENERIC_DIAL_0          33UL
#define EP_GENERIC_DIAL_1          34UL
#define EP_GENERIC_DIAL_2          35UL
#define EP_GENERIC_DIAL_3          36UL
#define EP_GENERIC_DIAL_4          37UL
#define EP_GENERIC_DIAL_5          38UL
#define EP_GENERIC_DIAL_6          39UL
#define EP_GENERIC_DIAL_7          40UL
#define EP_PV_PRODUCTION           41UL
#define EP_POWER_BALANCE           42UL
#define EP_BATTERY_BALANCE         43UL
#define EP_HEATER_HOT              44UL
#define EP_HEATER_COLD             45UL
#define EP_HEXASENSE_BUTTON_STATE  46UL
#define EP_FLUKSO_L1               47UL
#define EP_FLUKSO_L2               48UL
#define EP_FLUKSO_L3               49UL
#define EP_FLUKSO_S01              50UL
#define EP_FLUKSO_S02              51UL

// endpoints for Goerlitz SmartMeter
#define EP_GL_IMPORT_L1            52L
#define EP_GL_IMPORT_L2            53L
#define EP_GL_IMPORT_L3            54L
#define EP_GL_EXPORT_POWER         55L
#define EP_GL_EXPORT_L1            56L
#define EP_GL_EXPORT_L2            57L
#define EP_GL_EXPORT_L3            58L
#define EP_GL_IMPORT_ENERGY        59L
#define EP_GL_EXPORT_ENERGY        60L
#define EP_GL_FIRMWARE             61L
#define EP_GL_CURRENT_L1           62L
#define EP_GL_CURRENT_L2           63L
#define EP_GL_CURRENT_L3           65L
#define EP_GL_VOLTAGE_L1           66L
#define EP_GL_VOLTAGE_L2           67L
#define EP_GL_VOLTAGE_L3           68L
#define EP_GL_POWER_FACTOR_L1      69L
#define EP_GL_POWER_FACTOR_L2      70L
#define EP_GL_POWER_FACTOR_L3      71L

//Property IDs
#define EP_PROP_NAME               1UL
#define EP_PROP_MIN                2UL
#define EP_PROP_MAX                3UL

#endif // ENDPOINTS_H_
