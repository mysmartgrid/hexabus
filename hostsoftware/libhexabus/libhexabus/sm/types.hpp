#ifndef LIBHEXABUS_SM_TYPES_HPP_
#define LIBHEXABUS_SM_TYPES_HPP_

#include <stdint.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/variant.hpp>

namespace hexabus {
namespace sm {

typedef boost::posix_time::ptime hxb_datetime_t;

inline uint8_t hxbdt_hour(const hxb_datetime_t* dt)
{ return dt->time_of_day().hours(); }

inline uint8_t hxbdt_minute(const hxb_datetime_t* dt)
{ return dt->time_of_day().minutes(); }

inline uint8_t hxbdt_second(const hxb_datetime_t* dt)
{ return dt->time_of_day().seconds(); }

inline uint8_t hxbdt_day(const hxb_datetime_t* dt)
{ return dt->date().day(); }

inline uint8_t hxbdt_month(const hxb_datetime_t* dt)
{ return dt->date().month(); }

inline uint16_t hxbdt_year(const hxb_datetime_t* dt)
{ return dt->date().year(); }

inline uint8_t hxbdt_weekday(const hxb_datetime_t* dt)
{ return dt->date().day_of_week(); }

inline void hxbdt_set(hxb_datetime_t* dt,
		uint8_t second, uint8_t minute, uint8_t hour,
		uint8_t day, uint8_t month, uint16_t year,
		uint8_t weekday)
{
	*dt = boost::posix_time::ptime(
		boost::gregorian::date(year, month, day),
		boost::posix_time::time_duration(hour, minute, second));
}



typedef struct {
	uint8_t type;

	bool v_bool;
	uint32_t v_uint;
	float v_float;
	hxb_datetime_t v_datetime;
	const char* v_binary;
} hxb_sm_value_t;



#include "sm/sm_types.h"

}
}

#endif
