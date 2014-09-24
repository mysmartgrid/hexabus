#ifndef LIB_LANG_SPIRIT__WORKAROUNDS_HPP_F8B035D7BD93432C
#define LIB_LANG_SPIRIT__WORKAROUNDS_HPP_F8B035D7BD93432C

#include <memory>

#include <boost/fusion/include/vector.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/phoenix.hpp>

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

namespace detail {

template<size_t...>
struct list{};

template<size_t Max, size_t I = 0, size_t... L>
struct make_list {
	typedef typename make_list<Max, I + 1, L..., I>::type type;
};
template<size_t Max, size_t... L>
struct make_list<Max, Max, L...> {
	typedef list<L...> type;
};

template<typename T>
static std::tuple<const T&> tupleize(const T& t)
{
	return std::tuple<const T&>(t);
}

template<typename T>
static std::tuple<T&> tupleize(T& t)
{
	return std::tuple<T&>(t);
}

template<template<typename...> class Vector, typename... VecArgs,
	typename = typename std::enable_if<boost::fusion::traits::is_sequence<Vector<VecArgs...>>::value>::type>
static auto tupleize(Vector<VecArgs...>& vec)
	-> decltype(tupleize(vec, typename make_list<sizeof...(VecArgs)>::type()));

template<template<typename...> class Vector, typename... VecArgs, size_t... L>
static auto tupleize(Vector<VecArgs...>& vec, list<L...>)
	-> decltype(std::tuple_cat((tupleize(boost::fusion::at_c<L>(vec)))...))
{
	return std::tuple_cat((tupleize(boost::fusion::at_c<L>(vec)))...);
}

template<template<typename...> class Vector, typename... VecArgs, typename>
static auto tupleize(Vector<VecArgs...>& vec)
	-> decltype(tupleize(vec, typename make_list<sizeof...(VecArgs)>::type()))
{
	return tupleize(vec, typename make_list<sizeof...(VecArgs)>::type());
}

template<typename Fn, bool ForwardVal = false>
struct fwd_s {
	Fn f;

	template<typename... Args, size_t... L>
	auto call_with(const std::tuple<Args...>& args, list<L...>) const
		-> decltype(f(std::get<L>(args)...))
	{
		return f(std::get<L>(args)...);
	}

	template<typename... Args>
	auto call_with(const std::tuple<Args...>& args) const
		-> decltype(call_with(args, typename make_list<sizeof...(Args)>::type()))
	{
		return call_with(args, typename make_list<sizeof...(Args)>::type());
	}

	template<typename Ctx>
	static std::tuple<> val_tuple(Ctx&, std::integral_constant<bool, false>)
	{
		return {};
	}

	template<typename Ctx>
	static auto val_tuple(Ctx& ctx, std::integral_constant<bool, true>)
		-> std::tuple<decltype(boost::fusion::at_c<0>(ctx.attributes))>
	{
		auto& val =  boost::fusion::at_c<0>(ctx.attributes);
		return std::tuple<decltype(val)>(val);
	}

	template<typename... Args>
	auto pass_tuple(const std::tuple<Args...>& args, bool& pass, int) const
		-> typename std::enable_if<
				!decltype(call_with(std::tuple_cat(args, std::tuple<bool&>(pass))), 0)(),
				std::tuple<bool&>
			>::type
	{
		return std::tuple<bool&>(pass);
	}

	template<typename... Args>
	static std::tuple<> pass_tuple(const std::tuple<Args...>& args, bool& pass, void*)
	{
		return std::tuple<>();
	}

	template<typename... Args>
	auto call_maybe_append_pass(const std::tuple<Args...>& args, bool& pass) const
		-> decltype(call_with(std::tuple_cat(args, pass_tuple(args, pass, 0))))
	{
		return call_with(std::tuple_cat(args, pass_tuple(args, pass, 0)));
	}

	template<typename... Args, typename Context>
	auto call_maybe_prepend_val(const std::tuple<Args...>& args, Context& ctx, bool& pass) const
		-> decltype(call_maybe_append_pass(std::tuple_cat(val_tuple(ctx, std::integral_constant<bool, ForwardVal>()), args), pass))
	{
		return call_maybe_append_pass(std::tuple_cat(val_tuple(ctx, std::integral_constant<bool, ForwardVal>()), args), pass);
	}

	template<typename Attrib, typename Context,
		typename = typename std::enable_if<boost::fusion::traits::is_sequence<Attrib>::value>::type>
	auto operator()(Attrib& a, Context& ctx, bool& pass) const
		-> decltype(call_maybe_prepend_val(tupleize(a), ctx, pass))
	{
		return call_maybe_prepend_val(tupleize(a), ctx, pass);
	}

	template<typename Attrib, typename Context,
		typename = typename std::enable_if<!boost::fusion::traits::is_sequence<Attrib>::value>::type>
	auto operator()(Attrib& a, Context& ctx, bool& pass) const
		-> decltype(call_maybe_prepend_val(std::tuple<Attrib&>(a), ctx, pass))
	{
		return call_maybe_prepend_val(std::tuple<Attrib&>(a), ctx, pass);
	}
};

template<typename Fn, bool ForwardVal = false>
struct makefwd_s {
	fwd_s<Fn, ForwardVal> fn;

	template<typename Attrib, typename Context>
	void operator()(Attrib& a, Context& ctx, bool& pass) const
	{
		boost::fusion::at_c<0>(ctx.attributes) = fn(a, ctx, pass);
	}
};

template<typename Fn>
struct lambda_binder {
	Fn fn;

	// apparently, Boost lies a lot about what result_of does.
	template<typename... Args>
	struct result {
		typedef decltype(std::declval<Fn>()(std::declval<Args&>()...)) type;
	};
	template<typename F, typename... Args>
	struct result<F(Args...)> {
		typedef decltype(std::declval<Fn>()(std::declval<Args>()...)) type;
	};

	template<typename Sig>
	struct result_of;
	template<typename R, typename... Args>
	struct result_of<R (Fn::*)(Args...) const> {
		typedef R type;
	};
	typedef typename result_of<decltype(&Fn::operator())>::type result_type;

	template<typename... Args>
	auto operator()(const Args&... args) const
		-> decltype(fn(const_cast<Args&>(args)...))
	{
		return fn(const_cast<Args&>(args)...);
	}
};

}

static constexpr struct {
	template<typename Fn>
	detail::fwd_s<Fn> operator%(Fn fn) const
	{
		return { fn };
	}

	template<typename Fn>
	detail::fwd_s<Fn, true> operator&(Fn fn) const
	{
		return { fn };
	}
} fwd = {};

static constexpr struct {
	template<typename Fn>
	detail::makefwd_s<Fn> operator%(Fn fn) const
	{
		return { fn };
	}

	template<typename Fn>
	detail::makefwd_s<Fn, true> operator&(Fn fn) const
	{
		return { fn };
	}
} makefwd = {};

template<typename Fn, typename... Args>
static auto bindl(Fn fn, Args&&... args)
	-> decltype(boost::phoenix::bind(detail::lambda_binder<Fn>{fn}, args...))
{
	return boost::phoenix::bind(detail::lambda_binder<Fn>{fn}, args...);
}

}
}
}

#endif
