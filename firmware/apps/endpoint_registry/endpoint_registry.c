#include "endpoint_registry.h"

#include <stddef.h>

#include "hexabus_config.h"
#include "nvm.h"
#include "endpoints.h"

#include "value_broadcast.h"

#define LOG_LEVEL ENDPOINT_REGISTRY_DEBUG
#include "syslog.h"

static const char str_dev_info[] RODATA = "Device Information";

struct endpoint_registry_entry* _endpoint_chain = 0;
struct endpoint_property_registry_entry* _endpoint_property_chain = 0;

static uintptr_t next_nvm_addr = nvm_addr(endpoint_properties);

static uint32_t descriptor_eid(struct endpoint_registry_entry* entry)
{
	struct endpoint_descriptor ep = {};
	memcpy_from_rodata(&ep, entry->descriptor, sizeof(ep));
	return ep.eid;
}

void _endpoint_register(const struct endpoint_descriptor* ep, struct endpoint_registry_entry* chain_link)
{
	struct endpoint_descriptor ep_copy;
	memcpy_from_rodata(&ep_copy, ep, sizeof(ep_copy));

#if ENDPOINT_REGISTRY_DEBUG

	syslog(LOG_DEBUG, "Register endpoint %lu", ep_copy.eid);

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
	case HXB_DTYPE_UINT16:
	case HXB_DTYPE_UINT32:
	case HXB_DTYPE_UINT64:
	case HXB_DTYPE_SINT8:
	case HXB_DTYPE_SINT16:
	case HXB_DTYPE_SINT32:
	case HXB_DTYPE_SINT64:
	case HXB_DTYPE_FLOAT:
	case HXB_DTYPE_128STRING:
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

static void synthesize_read_zero(uint32_t eid, struct hxb_value* value)
{
	value->datatype = HXB_DTYPE_128STRING;

	size_t len = HXB_STRING_PACKET_BUFFER_LENGTH;
	if (len >= nvm_size(domain_name)) {
			len = nvm_size(domain_name) - 1;
		}

	nvm_read_block(domain_name, value->v_string, len);
	value->v_string[nvm_size(domain_name)] = '\0';
}

static bool find_descriptor(uint32_t eid, struct endpoint_descriptor* result)
{
	for (struct endpoint_registry_entry* ep = _endpoint_chain; ep; ep = ep->next) {
		if (descriptor_eid(ep) == eid) {
			memcpy_from_rodata(result, ep->descriptor, sizeof(*result));
			return true;
		}
	}

	return false;
}

enum hxb_datatype endpoint_get_datatype(uint32_t eid)
{
	if (eid == 0) {
		return HXB_DTYPE_128STRING;
	} else {
		struct endpoint_descriptor ep = {};
		if (find_descriptor(eid, &ep)) {
			return ep.datatype;
		} else {
			return HXB_DTYPE_UNDEFINED;
		}
	}
}

enum hxb_error_code endpoint_write(uint32_t eid, const struct hxb_envelope* env)
{
	if (eid == 0) {
		return HXB_ERR_WRITEREADONLY;
	} else {
		struct endpoint_descriptor ep = {};
		if (find_descriptor(eid, &ep)) {
			if (ep.write == 0) {
				return HXB_ERR_WRITEREADONLY;
			} else if (ep.datatype != env->value.datatype) {
				return HXB_ERR_DATATYPE;
			} else {
				enum hxb_error_code error = ep.write(env);
				if(error == 0) {
					broadcast_value(eid);
				}
				return error;
			}
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

enum hxb_error_code endpoint_read(uint32_t eid, struct hxb_value* value)
{
	if (eid == 0) {
		synthesize_read_zero(eid, value);
		return HXB_ERR_SUCCESS;
	} else {
		struct endpoint_descriptor ep = {};
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
	if (eid == 0) {
		strncpy_from_rodata(buffer, str_dev_info, len);
		return HXB_ERR_SUCCESS;
	} else {
		struct endpoint_descriptor ep;
		if (find_descriptor(eid, &ep)) {
			strncpy_from_rodata(buffer, ep.name, len);
			return HXB_ERR_SUCCESS;
		} else {
			return HXB_ERR_UNKNOWNEID;
		}
	}
}

static uint32_t property_eid(struct endpoint_property_registry_entry* entry)
{
	struct endpoint_property_descriptor pd = {};
	memcpy_from_rodata(&pd, entry->descriptor, sizeof(pd));
	return pd.eid;
}

static uint32_t property_id(struct endpoint_property_registry_entry* entry)
{
	struct endpoint_property_descriptor pd = {};
	memcpy_from_rodata(&pd, entry->descriptor, sizeof(pd));
	return pd.propid;
}

void _property_register(const struct endpoint_property_descriptor* epp, struct endpoint_property_registry_entry* chain_link)
{
	struct endpoint_property_descriptor epp_copy;
	memcpy_from_rodata(&epp_copy, epp, sizeof(epp_copy));

#if ENDPOINT_REGISTRY_DEBUG
	syslog(LOG_DEBUG, "Register property %lu on endpoint %lu", epp_copy.propid, epp_copy.eid);

	for (struct endpoint_property_registry_entry* head = _endpoint_property_chain; head; head = head->next) {
		if (property_id(head) == epp_copy.propid && property_eid(head) == epp_copy.eid) {
			syslog(LOG_DEBUG, "Property descriptor %p re-registers PropID %lu on EID %lu, ignoring", epp, epp_copy.propid, epp_copy.eid);
			return;
		}
	}

	switch (epp_copy.datatype) {
	case HXB_DTYPE_BOOL:
	case HXB_DTYPE_UINT8:
	case HXB_DTYPE_UINT16:
	case HXB_DTYPE_UINT32:
	case HXB_DTYPE_UINT64:
	case HXB_DTYPE_SINT8:
	case HXB_DTYPE_SINT16:
	case HXB_DTYPE_SINT32:
	case HXB_DTYPE_SINT64:
	case HXB_DTYPE_FLOAT:
	case HXB_DTYPE_128STRING:
	case HXB_DTYPE_65BYTES:
	case HXB_DTYPE_16BYTES:
		break;

	default:
		syslog(LOG_DEBUG, "Property %lu on endpoint %lu has no correct datatype, ignoring", epp_copy.propid, epp_copy.eid);
		return;
	}
#endif

	chain_link->value = next_nvm_addr;

	switch(epp_copy.datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			next_nvm_addr+=sizeof(uint8_t);
			break;
		case HXB_DTYPE_UINT16:
			next_nvm_addr+=sizeof(uint16_t);
			break;
		case HXB_DTYPE_UINT32:
			next_nvm_addr+=sizeof(uint32_t);
			break;
		case HXB_DTYPE_UINT64:
			next_nvm_addr+=sizeof(uint64_t);
			break;
		case HXB_DTYPE_SINT8:
			next_nvm_addr+=sizeof(int8_t);
			break;
		case HXB_DTYPE_SINT16:
			next_nvm_addr+=sizeof(int16_t);
			break;
		case HXB_DTYPE_SINT32:
			next_nvm_addr+=sizeof(int32_t);
			break;
		case HXB_DTYPE_SINT64:
			next_nvm_addr+=sizeof(int64_t);
			break;
		case HXB_DTYPE_FLOAT:
			next_nvm_addr+=sizeof(float);
			break;
		case HXB_DTYPE_128STRING:
			next_nvm_addr+=HXB_PROPERTY_STRING_LENGTH+1;
			break;
		case HXB_DTYPE_65BYTES:
			next_nvm_addr+=65;
			break;
		case HXB_DTYPE_16BYTES:
			next_nvm_addr+=16;
			break;
		default:
			return;
	}

	if((next_nvm_addr-nvm_addr(endpoint_properties)) > nvm_size(endpoint_properties)) {
		syslog(LOG_ERR, "Could not register property: Not enough memory.");
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
			memcpy_from_rodata(&(result->desc), epp->descriptor, sizeof(result->desc));
			result->value = epp->value;
			return true;
		}
	}

	return false;
}

enum hxb_error_code endpoint_property_write(uint32_t eid, uint32_t propid, const struct hxb_value* value)
{
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
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_bool), sizeof(uint8_t));
						break;
					case HXB_DTYPE_UINT8:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_u8), sizeof(uint8_t));
						break;
					case HXB_DTYPE_UINT16:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_u16), sizeof(uint16_t));
						break;
					case HXB_DTYPE_UINT32:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_u32), sizeof(uint32_t));
						break;
					case HXB_DTYPE_UINT64:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_u64), sizeof(uint64_t));
						break;
					case HXB_DTYPE_SINT8:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_s8), sizeof(int8_t));
						break;
					case HXB_DTYPE_SINT16:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_s16), sizeof(int16_t));
						break;
					case HXB_DTYPE_SINT32:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_s32), sizeof(int32_t));
						break;
					case HXB_DTYPE_SINT64:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_s64), sizeof(int64_t));
						break;
					case HXB_DTYPE_FLOAT:
						nvm_write_block_at(epp.value, (unsigned char*) &(value->v_float), sizeof(float));
						break;
					case HXB_DTYPE_128STRING:
						value->v_string[HXB_PROPERTY_STRING_LENGTH] =  '\0';
						nvm_write_block_at(epp.value, (unsigned char*) value->v_string, HXB_PROPERTY_STRING_LENGTH);
						break;
					case HXB_DTYPE_65BYTES:
						nvm_write_block_at(epp.value, (unsigned char*) value->v_binary, 65);
						break;
					case HXB_DTYPE_16BYTES:
						nvm_write_block_at(epp.value, (unsigned char*) value->v_binary, 16);
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
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_bool), sizeof(uint8_t));
					break;
				case HXB_DTYPE_UINT8:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_u8), sizeof(uint8_t));
					break;
				case HXB_DTYPE_UINT16:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_u16), sizeof(uint16_t));
					break;
				case HXB_DTYPE_UINT32:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_u32), sizeof(uint32_t));
					break;
				case HXB_DTYPE_UINT64:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_u64), sizeof(uint64_t));
					break;
				case HXB_DTYPE_SINT8:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_s8), sizeof(int8_t));
					break;
				case HXB_DTYPE_SINT16:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_s16), sizeof(int16_t));
					break;
				case HXB_DTYPE_SINT32:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_s32), sizeof(int32_t));
					break;
				case HXB_DTYPE_SINT64:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_s64), sizeof(int64_t));
					break;
				case HXB_DTYPE_FLOAT:
					nvm_read_block_at(epp.value, (unsigned char*) &(value->v_float), sizeof(float));
					break;
				case HXB_DTYPE_128STRING:
					nvm_read_block_at(epp.value, (unsigned char*) value->v_string, HXB_PROPERTY_STRING_LENGTH);
					break;
				case HXB_DTYPE_65BYTES:
					nvm_read_block_at(epp.value, (unsigned char*) value->v_binary, 65);
					break;
				case HXB_DTYPE_16BYTES:
					nvm_read_block_at(epp.value, (unsigned char*) value->v_binary, 16);
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
