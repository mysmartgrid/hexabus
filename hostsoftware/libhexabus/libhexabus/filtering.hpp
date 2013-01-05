#ifndef LIBHEXABUS_FILTERING_HPP
#define LIBHEXABUS_FILTERING_HPP 1

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/bool.hpp>
#include <algorithm>

#include "packet.hpp"

namespace hexabus {
namespace filtering {

	template<typename T>
	struct is_filter : boost::mpl::false_ {};

	template<typename Type>
	struct IsOfType {
		typedef bool value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const Type*>(&packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return *value(from, packet);
		}
	};

	struct IsError : IsOfType<ErrorPacket> {};

	template<>
	struct is_filter<IsError> : boost::mpl::true_ {};

	struct EID {
		typedef uint32_t value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const EIDPacket*>(&packet)
				? result_type(static_cast<const EIDPacket&>(packet).eid())
				: result_type();
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			result_type v = value(from, packet);
			return v && *v;
		}
	};

	template<>
	struct is_filter<EID> : boost::mpl::true_ {};

	struct IsQuery : IsOfType<QueryPacket> {};

	template<>
	struct is_filter<IsQuery> : boost::mpl::true_ {};

	struct IsEndpointQuery : IsOfType<EndpointQueryPacket> {};

	template<>
	struct is_filter<IsEndpointQuery> : boost::mpl::true_ {};

	template<typename TValue>
	struct Value {
		typedef TValue value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const ValuePacket<TValue>*>(&packet)
				? result_type(static_cast<const ValuePacket<TValue>&>(packet).value())
				: result_type();
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			result_type v = value(from, packet);
			return v && *v;
		}
	};

	template<typename T>
	struct is_filter<Value<T> > : boost::mpl::true_ {};

	template<typename TValue>
	struct IsInfo : IsOfType<InfoPacket<TValue> > {};

	template<typename T>
	struct is_filter<IsInfo<T> > : boost::mpl::true_ {};

	template<typename TValue>
	struct IsWrite : IsOfType<WritePacket<TValue> > {};

	template<typename T>
	struct is_filter<IsWrite<T> > : boost::mpl::true_ {};

	struct IsEndpointInfo : IsOfType<EndpointInfoPacket> {};

	template<>
	struct is_filter<IsEndpointInfo> : boost::mpl::true_ {};

	struct Source {
		typedef boost::asio::ip::address_v6 value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return from;
		}
	};

	template<>
	struct is_filter<Source> : boost::mpl::true_ {};

	template<typename TValue>
	struct Constant {
		private:
			TValue _value;

		public:
			typedef TValue value_type;
			typedef boost::optional<value_type> result_type;

			Constant(const TValue& value)
				: _value(value)
			{}

			result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
			{
				return _value;
			}

			bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
			{
				return _value;
			}
	};

	template<typename TValue>
	struct is_filter<Constant<TValue> > : boost::mpl::true_ {};

	struct Any {
		typedef bool value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return true;
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return true;
		}
	};

	template<>
	struct is_filter<Any> : boost::mpl::true_ {};

	static inline IsError isError() { return IsError(); }
	static inline EID eid() { return EID(); }
	static inline IsQuery isQuery() { return IsQuery(); }
	static inline IsEndpointQuery isEndpointQuery() { return IsEndpointQuery(); }
	template<typename TValue>
	static inline Value<TValue> value() { return Value<TValue>(); }
	template<typename TValue>
	static inline IsInfo<TValue> isInfo() { return IsInfo<TValue>(); }
	template<typename TValue>
	static inline IsWrite<TValue> isWrite() { return IsWrite<TValue>(); }
	static inline IsEndpointInfo isEndpointInfo() { return IsEndpointInfo(); }
	static inline Source source() { return Source(); }
	template<typename TValue>
	static inline Constant<TValue> constant(const TValue& value) { return Constant<TValue>(value); }
	static inline Any any() { return Any(); }

	namespace ast {

		template<typename Item>
		struct HasExpression {
			private:
				Item _item;

			public:
				typedef bool value_type;
				typedef boost::optional<value_type> result_type;

				HasExpression(const Item& item)
					: _item(item)
				{}

				result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					return bool(_item.value(from, packet));
				}

				bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					return *value(from, packet);
				}
		};

	}
	
	template<typename Exp>
	struct is_filter<ast::HasExpression<Exp> > : boost::mpl::true_ {};

	namespace ast {

		template<typename Exp, typename Op>
		struct UnaryExpression {
			private:
				Exp _exp;

			public:
				typedef typename boost::result_of<Op(Exp)>::type value_type;
				typedef boost::optional<value_type> result_type;

				UnaryExpression(const Exp& exp)
					: _exp(exp)
				{}

				result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					typename Exp::result_type v = _exp.value(from, packet);
					return v && Op()(*v);
				}

				bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					result_type v = value(from, packet);
					return v && *v;
				}
		};

	}

	template<typename Exp, typename Op>
	struct is_filter<ast::UnaryExpression<Exp, Op> > : boost::mpl::true_ {};

	namespace ast {

		template<typename Left, typename Right, typename Op>
		struct BinaryExpression {
			private:
				Left _left;
				Right _right;

			public:
				typedef typename boost::result_of<Op(Left, Right)>::type value_type;
				typedef boost::optional<value_type> result_type;

				BinaryExpression(const Left& left, const Right& right)
					: _left(left), _right(right)
				{}

				result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					typename Left::result_type l = _left.value(from, packet);

					if (!l) {
						return result_type();
					}

					typename Right::result_type r = _right.value(from, packet);

					return l && r
						? result_type(Op()(*l, *r))
						: result_type();
				}

				bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					result_type v = value(from, packet);
					return v && *v;
				}
		};

	}

	template<typename Left, typename Right, typename Op>
	struct is_filter<ast::BinaryExpression<Left, Right, Op> > : boost::mpl::true_ {};

	namespace ast {

		template<typename Left, typename Right, typename Op>
		struct BooleanShortcutExpression {
			private:
				Left _left;
				Right _right;

			public:
				typedef bool value_type;
				typedef boost::optional<value_type> result_type;

				BooleanShortcutExpression(const Left& left, const Right& right)
					: _left(left), _right(right)
				{}

				result_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					typename Left::result_type l = _left.value(from, packet);
					bool operatorShortcutResult = Op()(true, false);

					if (l && (*l == operatorShortcutResult)) {
						return operatorShortcutResult;
					}

					typename Right::result_type r = _right.value(from, packet);

					return l && r
						? result_type(Op()(*l, *r))
						: result_type();
				}

				bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					result_type v = value(from, packet);
					return v && *v;
				}
		};

	}

	template<typename Left, typename Right, typename Op>
	struct is_filter<ast::BooleanShortcutExpression<Left, Right, Op> > : boost::mpl::true_ {};

	template<typename Exp>
	ast::HasExpression<Exp> has(const Exp& exp, typename boost::enable_if<is_filter<Exp> >::type* = 0)
	{ return ast::HasExpression<Exp>(exp); }

	template<typename Exp>
	typename boost::enable_if<is_filter<Exp>, ast::UnaryExpression<Exp, std::logical_not<bool> > >::type
		operator!(const Exp& exp)
	{ return ast::UnaryExpression<Exp, std::logical_not<bool> >(exp); }

#define BINARY_OP(Sym, Op, Class) \
	template<typename Left, typename Right> \
	typename boost::enable_if_c<is_filter<Left>::value && is_filter<Right>::value, Class<Left, Right, Op<typename Left::value_type> > >::type \
		operator Sym(const Left& left, const Right& right) \
		{ return Class<Left, Right, Op<typename Left::value_type> >(left, right); } \
	\
	template<typename Left> \
	typename boost::enable_if_c<is_filter<Left>::value, Class<Left, Constant<typename Left::value_type>, Op<typename Left::value_type> > >::type \
		operator Sym(const Left& left, const typename Left::value_type& right) \
		{ return Class<Left, Constant<typename Left::value_type>, Op<typename Left::value_type> >(left, right); } \
	\
	template<typename Right> \
	typename boost::enable_if_c<is_filter<Right>::value, Class<Constant<typename Right::value_type>, Right, Op<typename Right::value_type> > >::type \
		operator Sym(const typename Right::value_type& left, const Right& right) \
		{ return Class<Constant<typename Right::value_type>, Right, Op<typename Right::value_type> >(left, right); }

	BINARY_OP(<, std::less, ast::BinaryExpression)
	BINARY_OP(<=, std::less_equal, ast::BinaryExpression)
	BINARY_OP(>, std::greater, ast::BinaryExpression)
	BINARY_OP(>=, std::greater_equal, ast::BinaryExpression)
	BINARY_OP(==, std::equal_to, ast::BinaryExpression)
	BINARY_OP(!=, std::not_equal_to, ast::BinaryExpression)
	BINARY_OP(+, std::plus, ast::BinaryExpression)
	BINARY_OP(-, std::minus, ast::BinaryExpression)
	BINARY_OP(*, std::multiplies, ast::BinaryExpression)
	BINARY_OP(/, std::divides, ast::BinaryExpression)
	BINARY_OP(%, std::modulus, ast::BinaryExpression)
	BINARY_OP(&, std::bit_and, ast::BinaryExpression)
	BINARY_OP(|, std::bit_or, ast::BinaryExpression)
	BINARY_OP(^, std::bit_xor, ast::BinaryExpression)
	BINARY_OP(&&, std::logical_and, ast::BooleanShortcutExpression)
	BINARY_OP(||, std::logical_or, ast::BooleanShortcutExpression)

#undef BINARY_OP

}}

#endif
