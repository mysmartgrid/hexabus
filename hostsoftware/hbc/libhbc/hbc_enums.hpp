#ifndef LIBHBC_HBC_ENUMS_HPP
#define LIBHBC_HBC_ENUMS_HPP

namespace hexabus {
  // define first operator as "1" because "0" means "not parsed (yet)".
  enum bool_operator { AND = 1, OR };
  enum comp_operator { STM_EQ = 1, STM_LEQ, STM_GEQ, STM_LT, STM_GT, STM_NEQ };
}

#endif // LIBHBC_HBC_ENUMS
