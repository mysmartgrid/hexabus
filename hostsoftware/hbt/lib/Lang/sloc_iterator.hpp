#ifndef LIB_LANG_SLOC__ITERATOR_HPP_56B0B79C00122521
#define LIB_LANG_SLOC__ITERATOR_HPP_56B0B79C00122521

#include <boost/iterator_adaptors.hpp>

namespace hbt {
namespace lang {

template<typename Base>
class sloc_iterator : public boost::iterator_adaptor<
	sloc_iterator<Base>,
	Base,
	boost::use_default,
	boost::forward_traversal_tag> {
private:
	size_t _line, _col, _tabWidth;
	typename std::iterator_traits<Base>::value_type prev;

	friend class boost::iterator_core_access;

	void increment()
	{
		const auto& cur = *(this->base());

		if ((cur == '\r' && prev != '\n') || (cur == '\n' && prev != '\r')) {
			_line++;
			_col = 1;
		} else if (cur == '\t') {
			_col += _tabWidth - _col % _tabWidth;
		} else {
			_col++;
		}

		prev = cur;
		++this->base_reference();
	}

public:
	sloc_iterator() {}
	sloc_iterator(Base base, size_t tabWidth)
		: sloc_iterator::iterator_adaptor_(base), _line(1), _col(1), _tabWidth(tabWidth), prev(0)
	{}

	size_t line() const { return _line; }
	size_t col() const { return _col; }

	friend bool operator<(const sloc_iterator& left, const sloc_iterator& right)
	{
		return left.base_reference() < right.base_reference();
	}
};

}
}

#endif
