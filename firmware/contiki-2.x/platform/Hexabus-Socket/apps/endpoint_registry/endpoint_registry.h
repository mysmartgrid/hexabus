#ifndef ENDPOINT_REGISTRY_H_
#define ENDPOINT_REGISTRY_H_

#include <stdint.h>
#include <avr/pgmspace.h>
#include "dev/eeprom.h"
#include "hexabus_packet.h"

typedef enum hxb_error_code (*endpoint_read_fn)(struct hxb_value* value);
typedef enum hxb_error_code (*endpoint_write_fn)(const struct hxb_envelope* value);

struct endpoint_descriptor {
	uint8_t datatype;
	uint32_t eid;

	PGM_P name;

	// if set to 0, the endpoint will not be readable
	endpoint_read_fn read;
	// if set to 0, the endpoint will not be writable
	endpoint_write_fn write;
};

#define ENDPOINT_DESCRIPTOR const struct endpoint_descriptor PROGMEM

struct endpoint_property_descriptor {
	uint32_t eid;
	uint32_t propid;
	uint8_t datatype;
};

struct endpoint_property_value_descriptor {
	struct endpoint_property_descriptor desc;
	void* value;
};

#define ENDPOINT_PROPERTY_DESCRIPTOR const struct endpoint_property_descriptor PROGMEM

struct endpoint_registry_entry {
	const struct endpoint_descriptor* descriptor;
	struct endpoint_registry_entry* next;
};

struct endpoint_property_registry_entry {
	const struct endpoint_property_descriptor* descriptor;
	void* value;
	struct endpoint_property_registry_entry* next;
};

extern struct endpoint_registry_entry* _endpoint_chain;
extern struct endpoint_property_registry_entry* _endpoint_property_chain;

void _endpoint_register(const struct endpoint_descriptor* ep, struct endpoint_registry_entry* chain_link);
void _property_register(const struct endpoint_property_descriptor* epp, struct endpoint_property_registry_entry* chain_link);

#define ENDPOINT_REGISTER(DESC) \
	do { \
		static struct endpoint_registry_entry _endpoint_entry = { 0 }; \
		_endpoint_register(&DESC, &_endpoint_entry); \
	} while (0)

#define ENDPOINT_PROPERTY_REGISTER(DESC) \
	do { \
		static struct endpoint_property_registry_entry _property_entry = { 0 }; \
		_property_register(&DESC, &_property_entry); \
	} while (0)

enum hxb_datatype endpoint_get_datatype(uint32_t eid);
enum hxb_error_code endpoint_write(uint32_t eid, const struct hxb_envelope* env);
enum hxb_error_code endpoint_read(uint32_t eid, struct hxb_value* value);
enum hxb_error_code endpoint_get_name(uint32_t eid, char* buffer, size_t len);

enum hxb_error_code endpoint_property_write(uint32_t eid, uint32_t propid, const struct hxb_value* value);
enum hxb_error_code endpoint_property_read(uint32_t eid, uint32_t propid, struct hxb_value* value);
uint32_t get_next_property(uint32_t eid, uint32_t propid);
#endif
