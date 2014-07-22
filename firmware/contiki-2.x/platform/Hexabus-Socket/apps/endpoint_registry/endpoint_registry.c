#include "endpoint_registry.h"

#include <stddef.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include "eeprom_variables.h"
#include <stdio.h>
#include "hexabus_config.h"
#include "eeprom_variables.h"

//#define LOG_LEVEL ENDPOINT_REGISTRY_DEBUG
#define LOG_LEVEL LOG_DEBUG
#include "syslog.h"

struct endpoint_registry_entry* _endpoint_chain = 0;
struct endpoint_property_registry_entry* _endpoint_property_chain = 0;

static void* next_eeprom_addr = eep_addr(endpoint_properties);

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
		case HXB_DTYPE_66BYTES:
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
		if (len >= eep_size(domain_name)) {
			len = eep_size(domain_name) - 1;
		}
		eeprom_read_block(buffer, eep_addr(domain_name), len);
		buffer[eep_size(domain_name)] = '\0';
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

static uint32_t property_eid(struct endpoint_property_registry_entry* entry)
{
	return pgm_read_dword(((uint16_t) entry->descriptor) + offsetof(struct endpoint_property_descriptor, eid));
}

static uint32_t property_id(struct endpoint_property_registry_entry* entry)
{
	return pgm_read_dword(((uint16_t) entry->descriptor) + offsetof(struct endpoint_property_descriptor, propid));
}

void _property_register(const struct endpoint_property_descriptor* epp, struct endpoint_property_registry_entry* chain_link)
{
#if ENDPOINT_REGISTRY_DEBUG
	//TODO
#endif
	struct endpoint_property_descriptor epp_copy;
	memcpy_P(&epp_copy, epp, sizeof(epp_copy));
	syslog(LOG_DEBUG, "Registering %u %lu %lu\n", epp_copy.datatype, epp_copy.eid, epp_copy.propid);

	chain_link->value = next_eeprom_addr;

	switch(epp_copy.datatype){
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			next_eeprom_addr+=sizeof(uint8_t);
			break;
		case HXB_DTYPE_UINT32:
			next_eeprom_addr+=sizeof(uint32_t);
			break;
		case HXB_DTYPE_DATETIME:
			next_eeprom_addr+=sizeof(struct hxb_datetime);
			break;
		case HXB_DTYPE_FLOAT:
			next_eeprom_addr+=sizeof(float);
			break;
		case HXB_DTYPE_128STRING:
			next_eeprom_addr+=HXB_PROPERTY_STRING_LENGTH+1;
			break;
		case HXB_DTYPE_TIMESTAMP:
			next_eeprom_addr+=sizeof(uint32_t);
			break;
		case HXB_DTYPE_66BYTES:
			next_eeprom_addr+=sizeof(HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH);
			break;
		case HXB_DTYPE_16BYTES:
			next_eeprom_addr+=sizeof(HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
			break;
		default:
			return;
	}

	chain_link->descriptor = epp;
	chain_link->next = _endpoint_property_chain;
	_endpoint_property_chain = chain_link;
}

static bool find_property(uint32_t eid, uint32_t propid, struct endpoint_property_value_descriptor* result)
{
	for (struct endpoint_property_registry_entry* epp = _endpoint_property_chain; epp; epp = epp->next) {
		if (property_eid(epp) == eid && property_id(epp) == propid) {
			memcpy_P(&(result->desc), epp->descriptor, sizeof(result->desc));
			result->value = epp->value;
			return true;
		}
	}

	return false;
}

enum hxb_error_code endpoint_property_write(uint32_t eid, uint32_t propid, const struct hxb_value* value)
{
	if (eid % 32 == 0)
		eid = 0;

	if(propid == 0) {
		return HXB_ERR_WRITEREADONLY;
	} else {
		struct endpoint_property_value_descriptor epp;
		if (find_property(eid, propid, &epp)) {
			if (epp.desc.datatype != value->datatype) {
				return HXB_ERR_DATATYPE;
			} else {
				switch(value->datatype) {
					case HXB_DTYPE_BOOL:
						eeprom_write_block((unsigned char*) &(value->v_bool), epp.value, sizeof(uint8_t));
						break;
					case HXB_DTYPE_UINT8:
						eeprom_write_block((unsigned char*) &(value->v_u8), epp.value, sizeof(uint8_t));
						break;
					case HXB_DTYPE_UINT32:
						eeprom_write_block((unsigned char*) &(value->v_u32), epp.value, sizeof(uint32_t));
						break;
					case HXB_DTYPE_DATETIME:
						eeprom_write_block((unsigned char*) &(value->v_datetime), epp.value, sizeof(struct hxb_datetime));
						break;
					case HXB_DTYPE_FLOAT:
						eeprom_write_block((unsigned char*) &(value->v_float), epp.value, sizeof(float));
						break;
					case HXB_DTYPE_128STRING:
						value->v_string[HXB_PROPERTY_STRING_LENGTH] =  '\0';
						eeprom_write_block((unsigned char*) value->v_string, epp.value, HXB_PROPERTY_STRING_LENGTH);
						break;
					case HXB_DTYPE_TIMESTAMP:
						eeprom_write_block((unsigned char*) &(value->v_timestamp), epp.value, sizeof(uint32_t));
						break;
					case HXB_DTYPE_66BYTES:
						eeprom_write_block((unsigned char*) value->v_binary, epp.value, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH);
						break;
					case HXB_DTYPE_16BYTES:
						eeprom_write_block((unsigned char*) value->v_binary, epp.value, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
						break;
					default:
						return HXB_ERR_DATATYPE;
				}
				return HXB_ERR_SUCCESS;
			}
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

uint32_t get_next_endpoint(uint32_t eid)
{
	uint32_t next_eid = 0;

	for (struct endpoint_registry_entry* ep = _endpoint_chain; ep; ep = ep->next) {
		if (descriptor_eid(ep) > eid && (descriptor_eid(ep) < next_eid || next_eid == 0)) {
			next_eid = descriptor_eid(ep);
		}
	}

	return next_eid;
}

uint32_t get_next_property(uint32_t eid, uint32_t propid)
{
	uint32_t next_id = 0;

	for (struct endpoint_property_registry_entry* epp = _endpoint_property_chain; epp; epp = epp->next) {
		if (property_eid(epp) == eid && property_id(epp) > propid && (property_id(epp) < next_id || next_id == 0)) {
			next_id = property_id(epp);
		}
	}

	return next_id;
}

enum hxb_error_code endpoint_property_read(uint32_t eid, uint32_t propid, struct hxb_value* value)
{
	if (eid % 32 == 0)
		eid = 0;

	if(propid == 0) {
		value->datatype = HXB_DTYPE_UINT32;
		value->v_u32 = get_next_endpoint(eid);
		return HXB_ERR_SUCCESS;
	} else {
		struct endpoint_property_value_descriptor epp;
		if (find_property(eid, propid, &epp)) {
			value->datatype = epp.desc.datatype;
			switch(value->datatype) {
				case HXB_DTYPE_BOOL:
					eeprom_read_block((unsigned char*) &(value->v_bool), epp.value, sizeof(uint8_t));
					break;
				case HXB_DTYPE_UINT8:
					eeprom_read_block((unsigned char*) &(value->v_u8), epp.value, sizeof(uint8_t));
					break;
				case HXB_DTYPE_UINT32:
					eeprom_read_block((unsigned char*) &(value->v_u32), epp.value, sizeof(uint32_t));
					break;
				case HXB_DTYPE_DATETIME:
					eeprom_read_block((unsigned char*) &(value->v_datetime), epp.value, sizeof(struct hxb_datetime));
					break;
				case HXB_DTYPE_FLOAT:
					eeprom_read_block((unsigned char*) &(value->v_float), epp.value, sizeof(float));
					break;
				case HXB_DTYPE_128STRING:
					eeprom_read_block((unsigned char*) value->v_string, epp.value, HXB_PROPERTY_STRING_LENGTH);
					break;
				case HXB_DTYPE_TIMESTAMP:
					eeprom_read_block((unsigned char*) &(value->v_timestamp), epp.value, sizeof(uint32_t));
					break;
				case HXB_DTYPE_66BYTES:
					eeprom_read_block((unsigned char*) value->v_binary, epp.value, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH);
					break;
				case HXB_DTYPE_16BYTES:
					eeprom_read_block((unsigned char*) value->v_binary, epp.value, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
					break;
				default:
					return HXB_ERR_DATATYPE;
			}
			return HXB_ERR_SUCCESS;
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

