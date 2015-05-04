#ifndef ENDPOINT_REGISTRY_H_
#define ENDPOINT_REGISTRY_H_

#include "hexabus_config.h"

#include "hexabus_packet.h"

typedef enum hxb_error_code (*endpoint_read_fn)(struct hxb_value* value);
typedef enum hxb_error_code (*endpoint_write_fn)(const struct hxb_envelope* value);

struct endpoint_descriptor {
	uint8_t datatype;
	uint32_t eid;

	const char* name;

	// if set to 0, the endpoint will not be readable
	endpoint_read_fn read;
	// if set to 0, the endpoint will not be writable
	endpoint_write_fn write;
};

struct endpoint_property_descriptor {
	uint8_t datatype;
	uint32_t eid;
	uint32_t propid;
};

struct endpoint_property_value_descriptor {
	struct endpoint_property_descriptor desc;
	uintptr_t value;
};

struct endpoint_registry_entry {
	const struct endpoint_descriptor* descriptor;
	struct endpoint_registry_entry* next;
};

struct endpoint_property_registry_entry {
	const struct endpoint_property_descriptor* descriptor;
	uintptr_t value;
	struct endpoint_property_registry_entry* next;
};

extern struct endpoint_registry_entry* _endpoint_chain;
extern struct endpoint_property_registry_entry* _endpoint_property_chain;

void _endpoint_register(const struct endpoint_descriptor* ep, struct endpoint_registry_entry* chain_link);
void _property_register(const struct endpoint_property_descriptor* epp, struct endpoint_property_registry_entry* chain_link);

#define ENDPOINT_REGISTER(DATATYPE, EID, NAME, READ_FN, WRITE_FN) \
	do { \
		static struct endpoint_registry_entry _endpoint_entry = { 0 }; \
		\
		static const char name[] RODATA = NAME; \
		\
		static const struct endpoint_descriptor ep_descriptor RODATA = { \
			.datatype = DATATYPE, \
			.eid = EID, \
			.name = name, \
			.read = READ_FN, \
			.write = WRITE_FN, \
		}; \
		\
		_endpoint_register(&ep_descriptor, &_endpoint_entry); \
		\
		ENDPOINT_PROPERTY_REGISTER(HXB_DTYPE_128STRING, EID, EP_PROP_NAME); \
	} while (0)

#define ENDPOINT_PROPERTY_REGISTER(DATATYPE, EID, PROPID) \
	do { \
		static struct endpoint_property_registry_entry _property_entry = { 0 }; \
		\
		static const struct endpoint_property_descriptor prop_descriptor RODATA = { \
			.datatype = DATATYPE, \
			.eid = EID, \
			.propid = PROPID, \
		}; \
		_property_register(&prop_descriptor, &_property_entry); \
	} while (0)

enum hxb_datatype endpoint_get_datatype(uint32_t eid);
enum hxb_error_code endpoint_write(uint32_t eid, const struct hxb_envelope* env);
enum hxb_error_code endpoint_read(uint32_t eid, struct hxb_value* value);
enum hxb_error_code endpoint_get_name(uint32_t eid, char* buffer, size_t len);

enum {
	HXB_PROPERTY_STRING_LENGTH = 30
};

enum hxb_error_code endpoint_property_write(uint32_t eid, uint32_t propid, const struct hxb_value* value);
enum hxb_error_code endpoint_property_read(uint32_t eid, uint32_t propid, struct hxb_value* value);
uint32_t get_next_property(uint32_t eid, uint32_t propid);
#endif
