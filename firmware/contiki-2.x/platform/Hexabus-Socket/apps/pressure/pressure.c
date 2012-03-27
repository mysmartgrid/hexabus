#include "pressure.h"
#include "i2c.h"
#include "hexabus_config.h"
#include "contiki.h"

#include <util/delay.h>

#if PRESSURE_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int16_t ac1;
static int16_t ac2;
static int16_t ac3;
static uint16_t ac4;
static uint16_t ac5;
static uint16_t ac6;
static int16_t b1;
static int16_t b2;
static int16_t mb;
static int16_t mc;
static int16_t md;

static int32_t pressure = 0;
static float pressure_temp = 0;

uint16_t pressure_read16(uint8_t addr) {
    if(i2c_write_bytes(BMP085_ADDR, &addr, 1)) {
        return 0;
    }

    uint8_t val[2];

    if(i2c_read_bytes(BMP085_ADDR, val, 2)) {
        return 0;
    }

    return ((val[0] << 8) | val[1]);

}

uint8_t pressure_read8(uint8_t addr) {
    if(i2c_write_bytes(BMP085_ADDR, &addr, 1)) {
        return 0;
    }

    uint8_t val[1];

    if(i2c_read_bytes(BMP085_ADDR, val, 1)) {
        return 0;
    }

    return val[0];

}

void pressure_init() {
  
    ac1 = pressure_read16(0xAA);
    ac2 = pressure_read16(0xAC);
    ac3 = pressure_read16(0xAE);
    ac4 = pressure_read16(0xB0);
    ac5 = pressure_read16(0xB2);
    ac6 = pressure_read16(0xB4);
    b1 = pressure_read16(0xB6);
    b2 = pressure_read16(0xB8);
    mb = pressure_read16(0xBA);
    mc = pressure_read16(0xBC);
    md = pressure_read16(0xBE);

    process_start(&pressure_process,NULL);

    PRINTF("Pressure init complete");

}

float read_pressure() {
    return pressure/100.0;
}

float read_pressure_temp() {
    return pressure_temp;
}

PROCESS(pressure_process, "Priodically reads the pressure sensor");

PROCESS_THREAD(pressure_process, ev, data) {

static struct etimer pressure_read_delay;
static uint16_t ut;
static uint32_t up;
static uint8_t pressure_write[2];

PROCESS_BEGIN();

while(1) {

    //Read Temperature
    pressure_write[0] = 0xF4;
    pressure_write[1] = 0x2E;
    i2c_write_bytes(BMP085_ADDR, pressure_write, 2);

    etimer_set(&pressure_read_delay, CLOCK_SECOND*10UL/1000);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&pressure_read_delay));
    
    ut = pressure_read16(0xF6);

    //Read Pressure
    pressure_write[0] = 0xF4;
    pressure_write[1] = 0x34 + (PRESSURE_OVERSAMPLING<<6);
    i2c_write_bytes(BMP085_ADDR, pressure_write, 2);

    etimer_set(&pressure_read_delay, CLOCK_SECOND*30UL/1000);  //TODO: oversampling related dalay
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&pressure_read_delay));
    
    up = pressure_read16(0xF6);
    up <<= 8;
    up |= pressure_read8(0xF8);
    up >>= (8-PRESSURE_OVERSAMPLING);

    PRINTF("Raw pressure: %lu ,%u\n", up, ut);

    //Calculate real temperature
    int32_t x1 = (((int32_t)ut-(int32_t)ac6)*(int32_t)ac5)/32768;
    int32_t x2 = (int32_t)mc*2048/(x1+(int32_t)md);

    pressure_temp = ((x1+x2+8.0)/16.0)/10.0;

    //Calculate real pressure
    int32_t b6 = (x1+x2) - 4000;
    x1 = ((int32_t)b2*((b6*b6)/4096))/2048;
    x2 = ((int32_t)ac2*b6)/2048;
    int32_t x3 = x1+x2;
    int32_t b3 = ((((int32_t)ac1*4+x3)<<PRESSURE_OVERSAMPLING)+2)/4;
    x1 = ((int32_t)ac3*b6)/8192;
    x2 = ((int32_t)b1*((b6*b6)/4096))/65536;
    x3 = ((x1+x2)+2)/4;
    uint32_t b4 = ((uint32_t)ac4*(uint32_t)(x3+32768))/32768;
    uint32_t b7 = ((uint32_t)up-b3)*(uint32_t)(50000UL>>PRESSURE_OVERSAMPLING);

    int32_t p;

    if (b7<0x80000000) {
        p = (b7*2)/b4;
    } else {
        p = (b7/b4)*2;
    }

    x1 = (p/256)*(p/256);
    x1 = (x1*3038)/65536;
    x2 = (-7357*p)/65536;

    pressure = p+((x1+x2+(int32_t)3791)/16);

    PRINTF("Real pressure: %ld \n", pressure);
    
    etimer_set(&pressure_read_delay, CLOCK_SECOND*PRESSURE_READ_DELAY);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&pressure_read_delay));
}

PROCESS_END();
}
