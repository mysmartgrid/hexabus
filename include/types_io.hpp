#ifndef TYPES_IO_HPP_
#define TYPES_IO_HPP_

#include "types.hpp"

#include <iostream>

std::string print_tabular(const Key& key);
std::string print_tabular(const Device& dev);
std::string print_tabular(const Phy& phy);
std::string print_tabular(const Seclevel& seclevel);

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Key& key)
{
	return os << print_tabular(key);
}

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Device& dev)
{
	return os << print_tabular(dev);
}

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Phy& phy)
{
	return os << print_tabular(phy);
}

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Seclevel& sl)
{
	return os << print_tabular(sl);
}

#endif
