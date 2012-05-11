#ifndef HEXABUS_STATEMACHINE_STRUCTS_H_
#define HEXABUS_STATEMACHINE_STRUCTS_H_

// TODO AAAAH remove this from here because it is already in hexabus_packet.h (or at least #ifndef HXB_PACKET_H_ it!)
struct hxb_value {
  uint8_t   datatype;   // Datatype that is used, or HXB_DTYPE_UNDEFINED
  char      data[8];    // leave enough room for the largest datatype (datetime in this case)
};


// these structs implement a simple table layout

// op for datetime: bits 0..6 denote dependency on hour, minute, second, ...; bit 7 sets whether to check for >= or <.
// date/time transitions need to be stored separately. They are also executed seperately, each time before the "normal" transitions are executed

struct condition {
  uint8_t sourceIP[16];      // IP
  uint8_t sourceEID;      // EID we expect data from
  uint8_t op;           // predicate function						|
  uint8_t datatype;			// The constant to compare with v
	char 		data[4];			// Leave enough room for largest chunk of data, uint32_t/float in this case
} __attribute__ ((packed));

struct transition {
  uint8_t fromState;      // current state
  uint8_t cond;           // index of condition that must be matched
  uint8_t eid;            // id of endpoint which should do something
  uint8_t goodState;      // new state if everything went fine
  uint8_t badState;       // new state if some went wrong
  struct hxb_value value;  // Data for the endpoint
} __attribute__ ((packed));

#endif

