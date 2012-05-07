/*
 * Copyright (c) 2011, Fraunhofer ITWM
 * All rights reserved.
 * Author: 	Mathias Dalheimer <dalheimer@itwm.fhg.de>
 *
 */

#ifndef APPS_MEMDEBUG_MEMORY_DEBUGGER_H
#define APPS_MEMDEBUG_MEMORY_DEBUGGER_H 1

#include "mem-debug.h"
#include "hexabus_config.h"
#include "process.h"

PROCESS_NAME(memory_debugger_process);

void memory_debugger_init(void);

#endif /* APPS_MEMDEBUG_MEMORY_DEBUGGER_H */
