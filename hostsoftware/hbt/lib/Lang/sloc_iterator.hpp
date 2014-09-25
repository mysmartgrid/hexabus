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
	size_t _line, _col, _tabs;
	typename std::iterator_traits<Base>::value_type prev;

	friend class boost::iterator_core_access;

	void increment()
	{
		const auto& cur = *(this->base());

		if ((cur == '\r' && prev != '\n') || (cur == '\n' && prev != '\r')) {
			_line++;
			_col = 1;
			_tabs = 0;
		} else if (cur == '\t') {
			_tabs++;
		} else {
			_col++;
		}

		prev = cur;
		++this->base_reference();
	}

public:
	sloc_iterator() {}
	explicit sloc_iterator(Base base)
		: sloc_iterator::iterator_adaptor_(base), _line(1), _col(1), _tabs(0), prev(0)
	{}

	size_t line() const { return _line; }
	size_t col() const { return _col; }
	size_t tabs() const { return _tabs; }
};

template<typename Base>
size_t getLine(const sloc_iterator<Base>& it)
{
	return it.line();
}

template<typename Base>
size_t getColumn(const sloc_iterator<Base>& it, size_t tabwidth)
{
	return it.col() + tabwidth * it.tabs();
}

}
}

#endif
