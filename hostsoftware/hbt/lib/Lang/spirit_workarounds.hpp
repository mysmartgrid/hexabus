#ifndef LIB_LANG_SPIRIT__WORKAROUNDS_HPP_F8B035D7BD93432C
#define LIB_LANG_SPIRIT__WORKAROUNDS_HPP_F8B035D7BD93432C

#include <boost/optional.hpp>
#include <memory>

namespace hbt {
namespace lang {
namespace spirit_workarounds {

template<typename T>
class ptr {
	template<typename T2>
	friend class ptr;

private:
	mutable std::unique_ptr<T> p;

public:
	ptr() = default;

	ptr(T* p)
		: p(p)
	{
	}

	template<typename T2>
	ptr(T2* p)
		: p(p)
	{
	}

	ptr(const ptr& other)
		: p(std::move(other.p))
	{
	}

	template<typename T2>
	ptr(const ptr<T2>& other)
		: p(std::move(other.p))
	{
	}

	ptr& operator=(const ptr& other)
	{
		std::swap(p, other.p);
		return *this;
	}

	operator std::unique_ptr<T>() const
	{
		return std::move(p);
	}

	T* release()
	{
		return p.release();
	}

	operator bool()
	{
		return p.get();
	}

	T* operator->()
	{
		return p.get();
	}

	T& operator*()
	{
		return *p;
	}
};

}
}
}

#endif
