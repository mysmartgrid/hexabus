#ifndef BIGINT__HPP_56A73EB79E16A52B
#define BIGINT__HPP_56A73EB79E16A52B

#include <limits>

#include <cln/integer.h>
#include <cln/integer_io.h>

namespace hbt {
namespace util {

class Bigint {
private:
	cln::cl_I _val;

	Bigint(cln::cl_I val) : _val(std::move(val)) {}

public:
	Bigint(int32_t i) : _val(int64_t(i)) {}
	Bigint(int64_t i) : _val(int64_t(i)) {}
	Bigint(uint32_t i) : _val(uint64_t(i)) {}
	Bigint(uint64_t i) : _val(uint64_t(i)) {}

	template<typename T>
	bool fitsInto() const
	{
		typedef std::numeric_limits<T> lim;
		return !lim::is_bounded || (lim::min() <= *this && *this <= lim::max());
	}

	uint32_t toU32() const { return cl_I_to_UL(_val); }
	uint64_t toU64() const { return cl_I_to_UQ(_val); }
	int32_t toS32() const { return cl_I_to_L(_val); }
	int64_t toS64() const { return cl_I_to_Q(_val); }

	Bigint& operator++() { _val++; return *this; }
	Bigint operator++(int) { Bigint res(*this); ++_val; return res; }
	Bigint& operator--() { _val--; return *this; }
	Bigint operator--(int) { Bigint res(*this); --_val; return res; }

	Bigint operator+() const { return *this; }
	Bigint operator-() const { return -_val; }
	Bigint operator!() const { return _val == 0; }
	Bigint operator~() const { return ~_val; }

	Bigint& operator*=(const Bigint& other) { _val *= other._val; return *this; }
	Bigint& operator/=(const Bigint& other) { _val = truncate1(_val, other._val); return *this; }
	Bigint& operator%=(const Bigint& other) { _val = rem(_val, other._val); return *this; }
	Bigint& operator+=(const Bigint& other) { _val += other._val; return *this; }
	Bigint& operator-=(const Bigint& other) { _val -= other._val; return *this; }
	Bigint& operator<<=(const Bigint& other) { _val <<= other._val; return *this; }
	Bigint& operator>>=(const Bigint& other) { _val >>= other._val; return *this; }
	Bigint& operator&=(const Bigint& other) { _val &= other._val; return *this; }
	Bigint& operator|=(const Bigint& other) { _val |= other._val; return *this; }
	Bigint& operator^=(const Bigint& other) { _val ^= other._val; return *this; }

	friend bool operator<(const Bigint&, const Bigint&);
	friend bool operator<=(const Bigint&, const Bigint&);
	friend bool operator>(const Bigint&, const Bigint&);
	friend bool operator>=(const Bigint&, const Bigint&);
	friend bool operator==(const Bigint&, const Bigint&);
	friend bool operator!=(const Bigint&, const Bigint&);

	template<typename CharT, typename Traits>
	friend std::basic_ostream<CharT, Traits>&
	operator<<(std::basic_ostream<CharT, Traits>&, const Bigint&);
};

inline bool operator<(const Bigint& a, const Bigint& b) { return a._val < b._val; }
inline bool operator<=(const Bigint& a, const Bigint& b) { return a._val <= b._val; }
inline bool operator>(const Bigint& a, const Bigint& b) { return a._val > b._val; }
inline bool operator>=(const Bigint& a, const Bigint& b) { return a._val >= b._val; }
inline bool operator==(const Bigint& a, const Bigint& b) { return a._val == b._val; }
inline bool operator!=(const Bigint& a, const Bigint& b) { return a._val != b._val; }

inline Bigint operator*(const Bigint& l, const Bigint& r) { return Bigint(l) *= r; }
inline Bigint operator/(const Bigint& l, const Bigint& r) { return Bigint(l) /= r; }
inline Bigint operator%(const Bigint& l, const Bigint& r) { return Bigint(l) %= r; }
inline Bigint operator+(const Bigint& l, const Bigint& r) { return Bigint(l) += r; }
inline Bigint operator-(const Bigint& l, const Bigint& r) { return Bigint(l) -= r; }
inline Bigint operator<<(const Bigint& l, const Bigint& r) { return Bigint(l) <<= r; }
inline Bigint operator>>(const Bigint& l, const Bigint& r) { return Bigint(l) >>= r; }
inline Bigint operator&(const Bigint& l, const Bigint& r) { return Bigint(l) &= r; }
inline Bigint operator|(const Bigint& l, const Bigint& r) { return Bigint(l) |= r; }
inline Bigint operator^(const Bigint& l, const Bigint& r) { return Bigint(l) ^= r; }

template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& o, const Bigint& b)
{
	return o << b._val;
}

}
}

#endif
