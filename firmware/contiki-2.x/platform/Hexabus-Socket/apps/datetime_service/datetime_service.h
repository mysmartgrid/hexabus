#ifndef DATETIME_SERVICE_H_
#define DATETIME_SERVICE_H_

#include <stdbool.h>
#include "process.h"
#include "../../../../../../shared/hexabus_packet.h"

#define UPDATE_INTERVALL 1UL
#define VALID_TIME 300 //Valid for 5Minutes


void updateDatetime(struct hxb_envelope*envelope);
int getDatetime(struct datetime *dt);
uint32_t getTimestamp();

PROCESS_NAME(datetime_service_process);

#endif /* DATETIME_SERVICE_H_ */
