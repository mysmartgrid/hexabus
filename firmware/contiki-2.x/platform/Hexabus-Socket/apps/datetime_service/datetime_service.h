#ifndef DATETIME_SERVICE_H_
#define DATETIME_SERVICE_H_

#include <stdbool.h>
#include "process.h"
#include "../../../../../../shared/hexabus_packet.h"

#define UPDATE_INTERVALL 1

void updateDatetime(struct datetime *dt);
int getDatetime(struct datetime *dt);

PROCESS_NAME(datetime_service_process);

#endif /* DATETIME_SERVICE_H_ */
