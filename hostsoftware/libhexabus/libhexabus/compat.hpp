#ifndef LIBHEXABUS_COMPAT_HPP
#define LIBHEXABUS_COMPAT_HPP 1

#include "libhexabus/config.h"

#if HAS_MACOS

/*** 
 * MD / SB: Mac OS 10.8.3 does not define bitwise operators - we inject
 * them here. This is supposed to be removed for newer versions of Mac
 * OS if they implement these operators.
 */

namespace std {
   template <class _Tp>
       struct bit_and : public binary_function<_Tp, _Tp, _Tp>
       {
         _Tp
         operator()(const _Tp& __x, const _Tp& __y) const
         { return __x & __y; }
       };
   template <class _Tp>
       struct bit_or : public binary_function<_Tp, _Tp, _Tp>
       {
         _Tp
         operator()(const _Tp& __x, const _Tp& __y) const
         { return __x | __y; }
       };
   template <class _Tp>
       struct bit_xor : public binary_function<_Tp, _Tp, _Tp>
       {
         _Tp
         operator()(const _Tp& __x, const _Tp& __y) const
         { return __x ^ __y; }
       };
};
#endif


#endif /* LIBHEXABUS_COMPAT_HPP */

