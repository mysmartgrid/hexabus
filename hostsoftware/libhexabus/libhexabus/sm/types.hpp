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





enum hxb_sm_opcode {
	HSO_LD_SOURCE_IP,
	HSO_LD_SOURCE_EID,
	HSO_LD_SOURCE_VAL,
	HSO_LD_CURSTATE,
	HSO_LD_CURSTATETIME,
	HSO_LD_FALSE,
	HSO_LD_TRUE,
	HSO_LD_U8,
	HSO_LD_U16,
	HSO_LD_U32,
	HSO_LD_FLOAT,
	HSO_LD_DT,
	HSO_LD_SYSTIME,

	HSO_LD_REG,
	HSO_ST_REG,

	HSO_OP_MUL,
	HSO_OP_DIV,
	HSO_OP_MOD,
	HSO_OP_ADD,
	HSO_OP_SUB,
	HSO_OP_DT_DIFF,
	HSO_OP_AND,
	HSO_OP_OR,
	HSO_OP_XOR,
	HSO_OP_NOT,
	HSO_OP_SHL,
	HSO_OP_SHR,
	HSO_OP_DUP,
	HSO_OP_DUP_I,
	HSO_OP_ROT,
	HSO_OP_ROT_I,
	HSO_OP_DT_DECOMPOSE,
	HSO_OP_GETTYPE,
	HSO_OP_SWITCH_8,
	HSO_OP_SWITCH_16,
	HSO_OP_SWITCH_32,

	HSO_CMP_BLOCK,
	HSO_CMP_IP_UNDEF,
	HSO_CMP_IP_LO,
	HSO_CMP_LT,
	HSO_CMP_LE,
	HSO_CMP_GT,
	HSO_CMP_GE,
	HSO_CMP_EQ,
	HSO_CMP_NEQ,
	HSO_CMP_DT_LT,
	HSO_CMP_DT_GE,

	HSO_CONV_B,
	HSO_CONV_U8,
	HSO_CONV_U32,
	HSO_CONV_F,

	HSO_JNZ,
	HSO_JZ,
	HSO_JUMP,

	HSO_JNZ_S,
	HSO_JZ_S,
	HSO_JUMP_S,

	HSO_WRITE,

	HSO_POP,

	HSO_RET_CHANGE,
	HSO_RET_STAY,
};

enum hxb_sm_dtmask {
	HSDM_SECOND  = 1,
	HSDM_MINUTE  = 2,
	HSDM_HOUR    = 4,
	HSDM_DAY     = 8,
	HSDM_MONTH   = 16,
	HSDM_YEAR    = 32,
	HSDM_WEEKDAY = 64,
};

struct hxb_sm_instruction {
	enum hxb_sm_opcode opcode;

	hxb_sm_value_t immed;
	uint8_t dt_mask;
	uint16_t jump_skip;
	struct {
		uint8_t first;
		uint8_t last;
		char data[16];
	} block;
};

}
}

#endif
