/*
 * provisioning.h
 *
 *  Created on: 14.06.2011
 *      Author: guenter hildebrandt
 */

#ifndef PROVISIONING_H_
#define PROVISIONING_H_

/*indicates ongoing provisioning*/
void provisioning_leds(void);

#if RAVEN_REVISION == HEXABUS_USB
int provisioning_master(void);
#elif RAVEN_REVISION == HEXABUS_SOCKET
int provisioning_slave(void);
#endif

#endif /* PROVISIONING_H_ */
