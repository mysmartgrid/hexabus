#ifndef APPS_PT100_H
#define APPS_PT100_H

#define PT100_CHANNEL_HOT       0
#define PT100_CHANNEL_COLD      4
#define PT100_TEMP_MIN      -10.f
#define PT100_TEMP_MAX      100.f
#define PT100_ADC_MIN         0.f
#define PT100_ADC_MAX       960.f

void pt100_init(void);

#endif
