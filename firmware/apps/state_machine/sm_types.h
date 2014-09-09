#ifndef APPS_STATE_MACHINE_SM_TYPES_H
#define APPS_STATE_MACHINE_SM_TYPES_H

#include "../../../../../shared/hexabus_types.h"

typedef struct hxb_datetime hxb_datetime_t;

inline uint8_t hxbdt_hour(const hxb_datetime_t* dt)
{ return dt->hour; }

inline uint8_t hxbdt_minute(const hxb_datetime_t* dt)
{ return dt->minute; }

inline uint8_t hxbdt_second(const hxb_datetime_t* dt)
{ return dt->second; }

inline uint8_t hxbdt_day(const hxb_datetime_t* dt)
{ return dt->day; }

inline uint8_t hxbdt_month(const hxb_datetime_t* dt)
{ return dt->month; }

inline uint16_t hxbdt_year(const hxb_datetime_t* dt)
{ return dt->year; }

inline uint8_t hxbdt_weekday(const hxb_datetime_t* dt)
{ return dt->weekday; }

inline void hxbdt_set(hxb_datetime_t* dt,
		uint8_t second, uint8_t minute, uint8_t hour,
		uint8_t day, uint8_t month, uint16_t year,
		uint8_t weekday)
{
	dt->second = second;
	dt->minute = minute;
	dt->hour = hour;
	dt->day = day;
	dt->month = month;
	dt->year = year;
	dt->weekday = weekday;
}



typedef struct {
	uint8_t type;

	union {
		bool v_bool;
		uint32_t v_uint;
		float v_float;
		hxb_datetime_t v_datetime;
		const char* v_binary;
	};
} hxb_sm_value_t;

#include "../../../../../shared/sm_types.h"

#endif
