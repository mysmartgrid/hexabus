#ifndef LIBHBC_HBC_ENUMS_HPP
#define LIBHBC_HBC_ENUMS_HPP

namespace hexabus {
  // define first operator as "1" because "0" means "not parsed (yet)".
  enum bool_operator { AND = 1, OR };
  enum comp_operator { STM_EQ = 1, STM_LEQ, STM_GEQ, STM_LT, STM_GT, STM_NEQ };
  enum datatype { DT_UNDEFINED = 0, DT_BOOL = 1, DT_UINT8, DT_UINT32, DT_FLOAT }; // TODO needs more...
  enum access_level { AC_READ = 1, AC_WRITE, AC_BROADCAST };
}

#endif // LIBHBC_HBC_ENUMS
