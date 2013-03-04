#include "endpoint_registry.h"

#include <stddef.h>
#include <avr/eeprom.h>

#include "eeprom_variables.h"
#include <stdio.h>

#define ENDPOINT_REGISTRY_DEBUG 1
#if ENDPOINT_REGISTRY_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

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

	if (ep_copy.eid % 32 == 0) {
		printf_P(PSTR("Endpoint descriptor %p claims to be synthethic (EID = 0 (mod 32)), ignoring\n"), ep);
		return;
	}

	for (struct endpoint_registry_entry* head = _endpoint_chain; head; head = head->next) {
		if (descriptor_eid(head) == ep_copy.eid) {
			printf_P(PSTR("Endpoint descriptor %p re-registers EID %lu, ignoring\n"), ep, ep_copy.eid);
			return;
		}
	}

	if (!ep_copy.name) {
		printf_P(PSTR("Endpoint %lu has no name, ignoring\n"), ep_copy.eid);
		return;
	}

	if (!ep_copy.read) {
		printf_P(PSTR("Endpoint %lu is not readable, ignoring\n"), ep_copy.eid);
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
		case HXB_DTYPE_66BYTES:
		case HXB_DTYPE_16BYTES:
			break;

		default:
			printf_P(PSTR("Endpoint %lu has no correct datatype, ignoring\n"), ep_copy.eid);
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

static uint8_t find_descriptor(uint32_t eid, struct endpoint_descriptor* result)
{
	for (struct endpoint_registry_entry* ep = _endpoint_chain; ep; ep = ep->next) {
		if (descriptor_eid(ep) == eid) {
			memcpy_P(result, ep->descriptor, sizeof(*result));
			return 1;
		}
	}

	return 0;
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

enum hxb_error_code endpoint_write(uint32_t eid, const struct hxb_value* value)
{
	if (eid % 32 == 0) {
		return HXB_ERR_WRITEREADONLY;
	} else {
		struct endpoint_descriptor ep;
		if (find_descriptor(eid, &ep)) {
			if (ep.write == 0) {
				return HXB_ERR_WRITEREADONLY;
			} else if (ep.datatype != value->datatype) {
				return HXB_ERR_DATATYPE;
			} else {
				return ep.write(value);
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

