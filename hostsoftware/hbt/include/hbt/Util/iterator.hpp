#ifndef ITERATOR__HPP_753642F4F9ED4132
#define ITERATOR__HPP_753642F4F9ED4132

#include <iterator>
#include <memory>
#include <type_traits>

namespace hbt {
namespace util {

template<typename Container, typename Context = void>
class const_uptr_value_iterator {
	friend Context;
private:
	typedef typename Container::const_iterator It;
	typedef const_uptr_value_iterator type;

public:
	static_assert(
		std::is_base_of<
			std::bidirectional_iterator_tag,
			typename It::iterator_category>::value,
		"requires bidirectional iterator");

	typedef typename It::difference_type difference_type;
	typedef const typename It::value_type::element_type* value_type;
	typedef const value_type* pointer;
	typedef const value_type& reference;
	typedef std::bidirectional_iterator_tag iterator_category;

private:
	It _base;
	value_type _p;

	const It& baseIterator() const { return _base; }

public:
	const_uptr_value_iterator() = default;

	const_uptr_value_iterator(It base)
		: _base(base)
	{}

	reference operator*()
	{
		_p = _base->get();
		return _p;
	}
	pointer operator->() { return &operator*(); }

	type& operator++()
	{
		++_base;
		return *this;
	}

	type operator++(int)
	{
		type res(*this);
		++*this;
		return res;
	}

	type& operator--()
	{
		--_base;
		return *this;
	}

	type operator--(int)
	{
		type res(*this);
		--*this;
		return res;
	}

	friend bool operator==(const type& a, const type& b)
	{
		return a._base == b._base;
	}
	friend bool operator!=(const type& a, const type& b)
	{
		return a._base != b._base;
	}
};

}
}

#endif
