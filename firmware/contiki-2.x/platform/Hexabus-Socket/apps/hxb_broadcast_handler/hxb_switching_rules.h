#ifndef HXB_SWITCHING_RULES_H_
#define HXB_SWITCHING_RULES_H_

// operator definitions for switching rules
#define HXB_OP_NULL         0x00 // do nothing, rule is not used
#define HXB_OP_EQUALS       0x01
#define HXB_OP_LESSTHAN     0x02
#define HXB_OP_GREATERTHAN  0x03

// representation of a Hexabus switching rule
struct hxb_switching_rule {
  char      device[16]; // IP address of device that sends the broadcast
  uint8_t   vid;
  uint8_t   op;
  uint8_t   constant;
  uint8_t   target_vid;   // TODO this has to be able to handle VIDs with different datatypes!
  uint8_t   target_value;
};

#endif
