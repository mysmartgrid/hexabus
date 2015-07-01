#include "net/netstack.h"

static void empty(void) {};

const struct network_driver nullnetwork_driver = {
  "null",
  empty,
  empty
};