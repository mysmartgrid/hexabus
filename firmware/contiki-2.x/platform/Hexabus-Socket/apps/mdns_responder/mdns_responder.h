/*
 * mdns_responder.h
 *
 *  Created on: 26.05.2011
 *      Author: admin_lokal
 */

#ifndef MDNS_RESPONDER_H_
#define MDNS_RESPONDER_H_
#include "process.h"

void mdns_responder_init(void);
void mdns_responder_set_domain_name(char*, uint8_t);
#endif /* MDNS_RESPONDER_H_ */
