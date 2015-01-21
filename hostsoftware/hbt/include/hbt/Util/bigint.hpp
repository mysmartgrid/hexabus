#ifndef BIGINT__HPP_56A73EB79E16A52B
#define BIGINT__HPP_56A73EB79E16A52B

#include <limits>

#include <boost/multiprecision/cpp_int.hpp>

namespace hbt {
namespace util {

class Bigint {
private:
	boost::multiprecision::cpp_int _val;

	Bigint(boost::multiprecision::cpp_int val) : _val(std::move(val)) {}

public:
	template<typename T>
	Bigint(T i) : _val(i) {}

	template<typename T>
	bool fitsInto() const
	{
		typedef std::numeric_limits<T> lim;
		return !lim::is_bounded || (lim::min() <= *this && *this <= lim::max());
	}

	template<typename T>
	T convertTo() const
	{
		if (!fitsInto<T>())
			throw std::out_of_range("this");

		return _val.convert_to<T>();
	}

	Bigint& operator++() { _val++; return *this; }
	Bigint operator++(int) { Bigint res(*this); ++_val; return res; }
	Bigint& operator--() { _val--; return *this; }
	Bigint operator--(int) { Bigint res(*this); --_val; return res; }

	Bigint operator+() const { return *this; }
	Bigint operator-() const { return Bigint(-_val); }
	Bigint operator!() const { return _val == 0; }
	Bigint operator~() const { return Bigint(~_val); }

	Bigint& operator*=(const Bigint& other) { _val *= other._val; return *this; }
	Bigint& operator/=(const Bigint& other) { _val /= other._val; return *this; }
	Bigint& operator%=(const Bigint& other) { _val %= other._val; return *this; }
	Bigint& operator+=(const Bigint& other) { _val += other._val; return *this; }
	Bigint& operator-=(const Bigint& other) { _val -= other._val; return *this; }
	Bigint& operator<<=(const Bigint& other) { _val <<= other.convertTo<uint32_t>(); return *this; }
	Bigint& operator>>=(const Bigint& other) { _val >>= other.convertTo<uint32_t>(); return *this; }
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
