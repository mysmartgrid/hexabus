#ifndef SYSLOG_H_
#define SYSLOG_H_ 1

#include "hexabus_config.h"

#define LOG_NONE   0
#define LOG_EMERG  1
#define LOG_ALERT  2
#define LOG_CRIT   3
#define LOG_ERR    4
#define LOG_WARN   5
#define LOG_NOTICE 6
#define LOG_INFO   7
#define LOG_DEBUG  8

#define LOG_6ADDR_FMT "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x"
#define LOG_6ADDR_VAL(addr) (addr).u8[0], (addr).u8[1], (addr).u8[2], (addr).u8[3], (addr).u8[4], (addr).u8[5], (addr).u8[6], (addr).u8[7], \
                            (addr).u8[8], (addr).u8[9], (addr).u8[10], (addr).u8[11], (addr).u8[12], (addr).u8[13], (addr).u8[14], (addr).u8[15]

#define LOG_LLADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define LOG_LLADDR_VAL(lladdr) (lladdr).addr[0], (lladdr).addr[1], (lladdr).addr[2], (lladdr).addr[3],(lladdr).addr[4], (lladdr).addr[5]

#if defined(LOG_LEVEL) && LOG_LEVEL != LOG_NONE

#include <stdio.h>

#ifndef __SHORT_FILE__
#define __SHORT_FILE__ __FILE__
#endif

#define SYSLOG_PRINT2(line, fmt, ...) \
	printf_rofmt(ROSTR("%-30" ROSTR_FMT fmt "\n"), ROSTR("(" __SHORT_FILE__ ":" #line ")"), ##__VA_ARGS__)
#define SYSLOG_PRINT(line, fmt, ...) SYSLOG_PRINT2(line, fmt, ##__VA_ARGS__)

#define syslog(level, fmt, ...) \
	do { \
		if (level <= LOG_LEVEL) { \
			SYSLOG_PRINT(__LINE__, fmt, ##__VA_ARGS__); \
		} \
	} while (0)

#else // defined(LOG_LEVEL) && LOG_LEVEL > LOG_NONE

#define syslog(...)

#endif

#endif
