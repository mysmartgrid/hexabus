#include "endpoint_registry.h"

#include <stddef.h>
#include <avr/eeprom.h>

#include "eeprom_variables.h"
#include <stdio.h>

#if ENDPOINT_ACCESS_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

struct endpoint_registry_entry* _endpoint_chain = 0;

static void synthesize_read_m32(uint32_t eid, struct hxb_value* value)
{
	value->datatype = HXB_DTYPE_UINT32;
	value->v_u32 = 1;

	for (struct endpoint_registry_entry* ep = _endpoint_chain; ep; ep = ep->next) {
		uint32_t ep_eid = pgm_read_dword(((uint16_t) ep->descriptor) + offsetof(struct endpoint_descriptor, eid));
		if (ep_eid >= eid && ep_eid < eid + 32) {
			value->v_u32 |= 1UL << (ep_eid - eid);
		}
	}
}

static uint8_t find_descriptor(uint32_t eid, struct endpoint_descriptor* result)
{
	for (struct endpoint_registry_entry* ep = _endpoint_chain; ep; ep = ep->next) {
		if (pgm_read_dword(((uint16_t) ep->descriptor) + offsetof(struct endpoint_descriptor, eid)) == eid) {
			memcpy_P(result, ep->descriptor, sizeof(*result));
			return 1;
		}
	}

	return 0;
}

enum hxb_datatype _endpoint_get_datatype(uint32_t eid)
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

enum hxb_error_code _endpoint_write(uint32_t eid, const struct hxb_value* value)
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
				return ep.write(eid, value);
			}
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

enum hxb_error_code _endpoint_read(uint32_t eid, struct hxb_value* value)
{
	if (eid % 32 == 0) {
		synthesize_read_m32(eid, value);
		return HXB_ERR_SUCCESS;
	} else {
		struct endpoint_descriptor ep;
		if (find_descriptor(eid, &ep)) {
			value->datatype = ep.datatype;
			return ep.read(eid, value);
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

enum hxb_error_code _endpoint_get_name(uint32_t eid, char* buffer, size_t len)
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

