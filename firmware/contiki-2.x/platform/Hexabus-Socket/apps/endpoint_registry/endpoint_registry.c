#include "endpoint_registry.h"

#include <stddef.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include "eeprom_variables.h"
#include <stdio.h>
#include "hexabus_config.h"

#define LOG_LEVEL ENDPOINT_REGISTRY_DEBUG
#include "syslog.h"

struct endpoint_registry_entry* _endpoint_chain = 0;

static uint32_t descriptor_eid(struct endpoint_registry_entry* entry)
{
	return pgm_read_dword(((uint16_t) entry->descriptor) + offsetof(struct endpoint_descriptor, eid));
}

void _endpoint_register(const struct endpoint_descriptor* ep, struct endpoint_registry_entry* chain_link)
{
#if ENDPOINT_REGISTRY_DEBUG
	struct endpoint_descriptor ep_copy;
	memcpy_P(&ep_copy, ep, sizeof(ep_copy));

	syslog(LOG_DEBUG, "Register endpoint %lu", ep_copy.eid);

	if (ep_copy.eid % 32 == 0) {
		syslog(LOG_DEBUG, "Endpoint descriptor %p claims to be synthethic (EID = 0 (mod 32)), ignoring", ep);
		return;
	}

	for (struct endpoint_registry_entry* head = _endpoint_chain; head; head = head->next) {
		if (descriptor_eid(head) == ep_copy.eid) {
			syslog(LOG_DEBUG, "Endpoint descriptor %p re-registers EID %lu, ignoring", ep, ep_copy.eid);
			return;
		}
	}

	if (!ep_copy.name) {
		syslog(LOG_DEBUG, "Endpoint %lu has no name, ignoring", ep_copy.eid);
		return;
	}

	if (!ep_copy.read) {
		syslog(LOG_DEBUG, "Endpoint %lu is not readable, ignoring", ep_copy.eid);
		return;
	}

	switch (ep_copy.datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
		case HXB_DTYPE_UINT32:
		case HXB_DTYPE_DATETIME:
		case HXB_DTYPE_FLOAT:
		case HXB_DTYPE_128STRING:
		case HXB_DTYPE_TIMESTAMP:
		case HXB_DTYPE_65BYTES:
		case HXB_DTYPE_16BYTES:
			break;

		default:
			syslog(LOG_DEBUG, "Endpoint %lu has no correct datatype, ignoring", ep_copy.eid);
			return;
	}
#endif

	chain_link->descriptor = ep;
	chain_link->next = _endpoint_chain;
	_endpoint_chain = chain_link;
}

static void synthesize_read_m32(uint32_t eid, struct hxb_value* value)
{
	value->datatype = HXB_DTYPE_UINT32;
	value->v_u32 = 1;

	for (struct endpoint_registry_entry* ep = _endpoint_chain; ep; ep = ep->next) {
		uint32_t ep_eid = descriptor_eid(ep);
		if (ep_eid >= eid && ep_eid < eid + 32) {
			value->v_u32 |= 1UL << (ep_eid - eid);
		}
	}
}

static bool find_descriptor(uint32_t eid, struct endpoint_descriptor* result)
{
	for (struct endpoint_registry_entry* ep = _endpoint_chain; ep; ep = ep->next) {
		if (descriptor_eid(ep) == eid) {
			memcpy_P(result, ep->descriptor, sizeof(*result));
			return true;
		}
	}

	return false;
}

enum hxb_datatype endpoint_get_datatype(uint32_t eid)
{
	if (eid % 32 == 0) {
		return HXB_DTYPE_UINT32;
	} else {
		struct endpoint_descriptor ep;
		if (find_descriptor(eid, &ep)) {
			return ep.datatype;
		} else {
			return HXB_DTYPE_UNDEFINED;
		}
	}
}

enum hxb_error_code endpoint_write(uint32_t eid, const struct hxb_envelope* env)
{
	if (eid % 32 == 0) {
		return HXB_ERR_WRITEREADONLY;
	} else {
		struct endpoint_descriptor ep;
		if (find_descriptor(eid, &ep)) {
			if (ep.write == 0) {
				return HXB_ERR_WRITEREADONLY;
			} else if (ep.datatype != env->value.datatype) {
				return HXB_ERR_DATATYPE;
			} else {
				return ep.write(env);
			}
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

enum hxb_error_code endpoint_read(uint32_t eid, struct hxb_value* value)
{
	if (eid % 32 == 0) {
		synthesize_read_m32(eid, value);
		return HXB_ERR_SUCCESS;
	} else {
		struct endpoint_descriptor ep;
		if (find_descriptor(eid, &ep)) {
			value->datatype = ep.datatype;
			return ep.read(value);
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

enum hxb_error_code endpoint_get_name(uint32_t eid, char* buffer, size_t len)
{
	if (eid % 32 == 0) {
		if (len >= EE_DOMAIN_NAME_SIZE) {
			len = EE_DOMAIN_NAME_SIZE - 1;
		}
		eeprom_read_block(buffer, (void*)(EE_DOMAIN_NAME), len);
		buffer[EE_DOMAIN_NAME_SIZE] = '\0';
		return HXB_ERR_SUCCESS;
	} else {
		struct endpoint_descriptor ep;
		if (find_descriptor(eid, &ep)) {
			strncpy_P(buffer, ep.name, len);
			return HXB_ERR_SUCCESS;
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

