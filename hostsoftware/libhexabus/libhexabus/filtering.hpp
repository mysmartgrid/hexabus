#ifndef LIBHEXABUS_FILTERING_HPP
#define LIBHEXABUS_FILTERING_HPP 1

#include <boost/asio.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/bool.hpp>
#include <algorithm>

#include "packet.hpp"

namespace hexabus {
namespace filtering {

	template<typename T>
	struct is_filter_expression : boost::mpl::false_ {};

	template<typename T>
	struct is_filter : is_filter_expression<T> {};

	struct Error {
		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const ErrorPacket*>(&packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<>
	struct is_filter<Error> : boost::mpl::true_ {};

	struct EID {
		typedef uint32_t value_type;

		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const EIDPacket*>(&packet);
		}

		value_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return static_cast<const EIDPacket&>(packet).eid();
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet) && value(from, packet);
		}
	};

	template<>
	struct is_filter_expression<EID> : boost::mpl::true_ {};

	struct Query {
		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const QueryPacket*>(&packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<>
	struct is_filter<Query> : boost::mpl::true_ {};

	struct EndpointQuery {
		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const EndpointQueryPacket*>(&packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<>
	struct is_filter<EndpointQuery> : boost::mpl::true_ {};

	template<typename TValue>
	struct Value {
		typedef TValue value_type;

		value_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return static_cast<const ValuePacket<TValue>&>(packet).value();
		}

		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const ValuePacket<TValue>*>(&packet) && value(from, packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<typename T>
	struct is_filter_expression<Value<T> > : boost::mpl::true_ {};

	template<typename TValue>
	struct Info {
		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const InfoPacket<TValue>*>(&packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<typename T>
	struct is_filter<Info<T> > : boost::mpl::true_ {};

	template<typename TValue>
	struct Write {
		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const WritePacket<TValue>*>(&packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<typename T>
	struct is_filter<Write<T> > : boost::mpl::true_ {};

	struct EndpointInfo {
		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return dynamic_cast<const EndpointInfoPacket*>(&packet);
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<>
	struct is_filter<EndpointInfo> : boost::mpl::true_ {};

	struct Source {
		typedef boost::asio::ip::address_v6 value_type;

		bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return true;
		}

		value_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return from;
		}

		bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
		{
			return match(from, packet);
		}
	};

	template<>
	struct is_filter_expression<Source> : boost::mpl::true_ {};

	template<typename TValue>
	struct Constant {
		private:
			TValue _value;

		public:
			typedef TValue value_type;

			Constant(const TValue& value)
				: _value(value)
			{}

			bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
			{
				return true;
			}

			value_type value(const boost::asio::ip::address_v6& from, const Packet& packet) const
			{
				return _value;
			}
	};

	template<typename TValue>
	struct is_filter_expression<Constant<TValue> > : boost::mpl::true_ {};

	struct Any {
			bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
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

	static inline Error isError() { return Error(); }
	static inline EID eid() { return EID(); }
	static inline Query isQuery() { return Query(); }
	static inline EndpointQuery isEndpointQuery() { return EndpointQuery(); }
	template<typename TValue>
	static inline Value<TValue> value() { return Value<TValue>(); }
	template<typename TValue>
	static inline Info<TValue> isInfo() { return Info<TValue>(); }
	template<typename TValue>
	static inline Write<TValue> isWrite() { return Write<TValue>(); }
	static inline EndpointInfo isEndpointInfo() { return EndpointInfo(); }
	static inline Source source() { return Source(); }
	template<typename TValue>
	static inline Constant<TValue> constant(const TValue& value) { return Constant<TValue>(value); }

	namespace ast {

		template<typename Item>
		struct HasExpression {
			bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
			{
				return Item().match(from, packet);
			}

			bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
			{
				return match(from, packet);
			}
		};

	}
	
	template<typename Exp>
	struct is_filter_expression<ast::HasExpression<Exp> > : boost::mpl::true_ {};

	namespace ast {

		template<typename Exp, typename Op>
		struct UnaryExpression {
			private:
				Exp _exp;

			public:
				UnaryExpression(const Exp& exp)
					: _exp(exp)
				{}

				bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					return _exp.match(from, packet);
				}

				bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					return Op()(_exp(from, packet));
				}
		};

	}

	template<typename Exp, typename Op>
	struct is_filter_expression<ast::UnaryExpression<Exp, Op> > : boost::mpl::true_ {};

	namespace ast {

		template<typename Left, typename Right, typename Op>
		struct BinaryExpression {
			private:
				Left _left;
				Right _right;

				template<typename T>
				static typename boost::enable_if<is_filter<T>, bool>::type
					valueOf(const T& filter, const boost::asio::ip::address_v6& from, const Packet& packet)
				{
					return filter(from, packet);
				}

				template<typename T>
				static typename boost::enable_if_c<!is_filter<T>::value && is_filter_expression<T>::value, typename T::value_type>::type
					valueOf(const T& filter, const boost::asio::ip::address_v6& from, const Packet& packet)
				{
					return filter.value(from, packet);
				}

			public:
				BinaryExpression(const Left& left, const Right& right)
					: _left(left), _right(right)
				{}

				bool match(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					return _left.match(from, packet) && _right.match(from, packet);
				}

				bool operator()(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					return Op()(valueOf(_left, from, packet), valueOf(_right, from, packet));
				}

				bool value(const boost::asio::ip::address_v6& from, const Packet& packet) const
				{
					return operator()(from, packet);
				}
		};

	}

	template<typename Left, typename Right, typename Op>
	struct is_filter_expression<ast::BinaryExpression<Left, Right, Op> > : boost::mpl::true_ {};

	template<typename Exp>
	ast::HasExpression<Exp> has(const Exp& exp, typename boost::enable_if<is_filter<Exp> >::type* = 0)
	{ return ast::HasExpression<Exp>(exp); }

	template<typename Exp>
	typename boost::enable_if<is_filter_expression<Exp>, ast::UnaryExpression<Exp, std::logical_not<bool> > >::type
		operator!(const Exp& exp)
	{ return ast::UnaryExpression<Exp, std::logical_not<bool> >(exp); }

#define BINARY_OP(Sym, Op) \
	template<typename Left, typename Right> \
	typename boost::enable_if_c<is_filter<Left>::value && is_filter<Right>::value, \
			ast::BinaryExpression<Left, Right, Op<typename Left::value_type> > >::type \
		operator Sym(const Left& left, const Right& right) \
		{ return ast::BinaryExpression<Left, Right, Op<typename Left::value_type> >(left, right); } \
	\
	template<typename Left> \
	typename boost::enable_if_c<is_filter<Left>::value, ast::BinaryExpression<Left, Constant<typename Left::value_type>, Op<typename Left::value_type> > >::type \
		operator Sym(const Left& left, const typename Left::value_type& right) \
		{ return ast::BinaryExpression<Left, Constant<typename Left::value_type>, Op<typename Left::value_type> >(left, right); } \
	\
	template<typename Right> \
	typename boost::enable_if_c<is_filter<Right>::value, ast::BinaryExpression<Constant<typename Right::value_type>, Right, Op<typename Right::value_type> > >::type \
		operator Sym(const typename Right::value_type& left, const Right& right) \
		{ return ast::BinaryExpression<Constant<typename Right::value_type>, Right, Op<typename Right::value_type> >(left, right); }

	BINARY_OP(<, std::less)
	BINARY_OP(<=, std::less_equal)
	BINARY_OP(>, std::greater)
	BINARY_OP(>=, std::greater_equal)
	BINARY_OP(==, std::equal_to)
	BINARY_OP(!=, std::not_equal_to)

#undef BINARY_OP

#define BINARY_OP(Sym, Op) \
	template<typename Left, typename Right> \
	typename boost::enable_if_c<is_filter<Left>::value && is_filter<Right>::value, ast::BinaryExpression<Left, Right, Op<bool> > >::type \
		operator Sym(const Left& left, const Right& right) \
		{ return ast::BinaryExpression<Left, Right, Op<bool> >(left, right); } \
	\
	template<typename Left> \
	typename boost::enable_if_c<is_filter<Left>::value, ast::BinaryExpression<Left, Constant<bool>, Op<bool> > >::type \
		operator Sym(const Left& left, const typename Left::value_type& right) \
		{ return ast::BinaryExpression<Left, Constant<bool>, Op<bool> >(left, right); } \
	\
	template<typename Right> \
	typename boost::enable_if_c<is_filter<Right>::value, ast::BinaryExpression<Constant<bool>, Right, Op<bool> > >::type \
		operator Sym(const typename Right::value_type& left, const Right& right) \
		{ return ast::BinaryExpression<Constant<bool>, Right, Op<bool> >(left, right); }

	BINARY_OP(&&, std::logical_and)
	BINARY_OP(||, std::logical_or)
	BINARY_OP(^, std::bit_xor)

#undef BINARY_OP


}}

#endif
