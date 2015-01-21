#ifndef LIBHEXABUS_FILTERING_HPP
#define LIBHEXABUS_FILTERING_HPP 1

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <functional>

#include "packet.hpp"

namespace hexabus {
namespace filtering {

	template<typename T>
	struct is_filter : std::false_type {};

	template<typename Type>
	struct IsOfType {
		typedef bool value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return dynamic_cast<const Type*>(&packet);
		}

		bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return *value(packet, from);
		}
	};

	struct IsError : IsOfType<ErrorPacket> {};

	template<>
	struct is_filter<IsError> : std::true_type {};

	struct EID {
		typedef uint32_t value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return dynamic_cast<const EIDPacket*>(&packet)
				? result_type(static_cast<const EIDPacket&>(packet).eid())
				: result_type();
		}

		bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			result_type v = value(packet, from);
			return v && *v;
		}
	};

	template<>
	struct is_filter<EID> : std::true_type {};

	struct IsQuery : IsOfType<QueryPacket> {};

	template<>
	struct is_filter<IsQuery> : std::true_type {};

	struct IsEndpointQuery : IsOfType<EndpointQueryPacket> {};

	template<>
	struct is_filter<IsEndpointQuery> : std::true_type {};

	struct IsPropertyQuery : IsOfType<PropertyQueryPacket> {};

	template<>
	struct is_filter<IsPropertyQuery> : boost::mpl::true_ {};

	template<typename TValue>
	struct Value {
		typedef TValue value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return dynamic_cast<const ValuePacket<TValue>*>(&packet)
				? result_type(static_cast<const ValuePacket<TValue>&>(packet).value())
				: result_type();
		}

		bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			result_type v = value(packet, from);
			return v && *v;
		}
	};

	template<typename T>
	struct is_filter<Value<T> > : std::true_type {};

	struct Cause {
		typedef uint16_t value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			const CausedPacket* cause = dynamic_cast<const CausedPacket*>(&packet);
			return cause
				? cause->cause()
				: result_type();
		}

		bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			result_type v = value(packet, from);
			return v && *v;
		}
	};

	template<>
	struct is_filter<Cause> : boost::mpl::true_ {};

	template<typename TValue>
	struct IsInfo : IsOfType<InfoPacket<TValue> > {};

	template<typename T>
	struct is_filter<IsInfo<T> > : std::true_type {};

	template<typename TValue>
	struct IsReport : IsOfType<ReportPacket<TValue> > {};

	template<typename T>
	struct is_filter<IsReport<T> > : boost::mpl::true_ {};

	template<typename TValue>
	struct IsProxyInfo : IsOfType<ProxyInfoPacket<TValue> > {};

	template<typename T>
	struct is_filter<IsProxyInfo<T> > : boost::mpl::true_ {};

	template<typename TValue>
	struct IsWrite : IsOfType<WritePacket<TValue> > {};

	template<typename T>
	struct is_filter<IsWrite<T> > : std::true_type {};

	struct IsEndpointInfo : IsOfType<EndpointInfoPacket> {};

	template<>
	struct is_filter<IsEndpointInfo> : std::true_type {};

	struct IsEndpointReport : IsOfType<EndpointReportPacket> {};

	template<>
	struct is_filter<IsEndpointReport> : boost::mpl::true_ {};

	struct IsAck : IsOfType<AckPacket> {};

	template<>
	struct is_filter<IsAck> : boost::mpl::true_ {};

	template<typename TValue>
	struct IsPropertyReport : IsOfType<PropertyReportPacket<TValue> > {};

	template<typename T>
	struct is_filter<IsPropertyReport<T> > : boost::mpl::true_ {};

	template<typename TValue>
	struct IsPropertyWrite : IsOfType<PropertyWritePacket<TValue> > {};

	template<typename T>
	struct is_filter<IsPropertyWrite<T> > : boost::mpl::true_ {};

	struct SourceIP {
		typedef boost::asio::ip::address_v6 value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return from.address().to_v6();
		}
	};

	template<>
	struct is_filter<SourceIP> : std::true_type {};

	struct OriginIP {
		typedef boost::asio::ip::address_v6 value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			const ProxiedPacket* pinfo = dynamic_cast<const ProxiedPacket*>(&packet);
			return pinfo
				? pinfo->origin()
				: result_type();
		}
	};

	template<>
	struct is_filter<OriginIP> : boost::mpl::true_ {};

	struct SourcePort {
		typedef uint16_t value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return from.port();
		}

		bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return *value(packet, from);
		}
	};

	template<>
	struct is_filter<SourcePort> : std::true_type {};

	template<typename TValue>
	struct Constant {
		public:
			typedef typename std::decay<TValue>::type value_type;
			typedef boost::optional<value_type> result_type;

		private:
			value_type _value;

		public:
			Constant(const TValue& value)
				: _value(value)
			{}

			result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
			{
				return _value;
			}

			bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
			{
				return _value;
			}
	};

	template<typename TValue>
	struct is_filter<Constant<TValue> > : std::true_type {};

	struct Any {
		typedef bool value_type;
		typedef boost::optional<value_type> result_type;

		result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return true;
		}

		bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
		{
			return true;
		}
	};

	template<>
	struct is_filter<Any> : std::true_type {};

	static inline IsError isError() { return IsError(); }
	static inline EID eid() { return EID(); }
	static inline IsQuery isQuery() { return IsQuery(); }
	static inline IsEndpointQuery isEndpointQuery() { return IsEndpointQuery(); }
	static inline IsPropertyQuery isPropertyQuery() { return IsPropertyQuery(); }
	template<typename TValue>
	static inline Value<TValue> value() { return Value<TValue>(); }
	template<typename TValue>
	static inline IsInfo<TValue> isInfo() { return IsInfo<TValue>(); }
	template<typename TValue>
	static inline IsPropertyReport<TValue> isPropertyReport() { return IsPropertyReport<TValue>(); }
	template<typename TValue>
	static inline IsReport<TValue> isReport() { return IsReport<TValue>(); }
	template<typename TValue>
	static inline IsProxyInfo<TValue> isProxyInfo() { return IsProxyInfo<TValue>(); }
	template<typename TValue>
	static inline IsWrite<TValue> isWrite() { return IsWrite<TValue>(); }
	template<typename TValue>
	static inline IsPropertyWrite<TValue> isPropertyWrite() { return IsPropertyWrite<TValue>(); }
	static inline IsEndpointInfo isEndpointInfo() { return IsEndpointInfo(); }
	static inline IsEndpointReport isEndpointReport() { return IsEndpointReport(); }
	static inline IsAck isAck() { return IsAck(); }
	static inline Cause cause() { return Cause(); }
	static inline OriginIP originIP() { return OriginIP(); }
	static inline SourceIP sourceIP() { return SourceIP(); }
	static inline SourcePort sourcePort() { return SourcePort(); }
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

				result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					return bool(_item.value(packet, from));
				}

				bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					return *value(packet, from);
				}
		};

	}

	template<typename Exp>
	struct is_filter<ast::HasExpression<Exp> > : std::true_type {};

	namespace ast {

		template<typename Exp, typename Op>
		struct UnaryExpression {
			private:
				Exp _exp;

			public:
				typedef typename std::result_of<Op(typename Exp::value_type)>::type value_type;
				typedef boost::optional<value_type> result_type;

				UnaryExpression(const Exp& exp)
					: _exp(exp)
				{}

				result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					typename Exp::result_type v = _exp.value(packet, from);
					return v
						? result_type(Op()(*v))
						: result_type();
				}

				bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					result_type v = value(packet, from);
					return v && *v;
				}
		};

	}

	template<typename Exp, typename Op>
	struct is_filter<ast::UnaryExpression<Exp, Op> > : std::true_type {};

	namespace ast {

		template<typename Left, typename Right, template<typename T> class OpClass>
		struct BinaryExpression {
			private:
				Left _left;
				Right _right;

				typedef OpClass<typename std::common_type<typename Left::value_type, typename Right::value_type>::type> Op;

			public:
				typedef typename std::result_of<Op(typename Left::value_type, typename Right::value_type)>::type value_type;
				typedef boost::optional<value_type> result_type;

				BinaryExpression(const Left& left, const Right& right)
					: _left(left), _right(right)
				{}

				result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					typename Left::result_type l = _left.value(packet, from);

					if (!l) {
						return result_type();
					}

					typename Right::result_type r = _right.value(packet, from);

					return l && r
						? result_type(Op()(*l, *r))
						: result_type();
				}

				bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					result_type v = value(packet, from);
					return v && *v;
				}
		};

	}

	template<typename Left, typename Right, template<typename T> class Op>
	struct is_filter<ast::BinaryExpression<Left, Right, Op> > : std::true_type {};

	namespace ast {

		template<typename Left, typename Right, bool IsAnd>
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

				result_type value(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					typename Left::result_type l = _left.value(packet, from);

					if (l) {
						if (IsAnd && l && !*l)
							return false;
						else if (!IsAnd && l && *l)
							return true;
					}

					typename Right::result_type r = _right.value(packet, from);

					if (r) {
						if (IsAnd && r && !*r)
							return false;
						else if (!IsAnd && r && *r)
							return true;
					}

					return l && r
						? result_type(IsAnd ? *l && *r : *l || *r)
						: result_type();
				}

				bool operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from) const
				{
					result_type v = value(packet, from);
					return v && *v;
				}
		};

	}

	template<typename Left, typename Right, bool IsAnd>
	struct is_filter<ast::BooleanShortcutExpression<Left, Right, IsAnd> > : std::true_type {};

	template<typename Exp>
	ast::HasExpression<Exp> has(const Exp& exp, typename std::enable_if<is_filter<Exp>::value >::type* = 0)
	{ return ast::HasExpression<Exp>(exp); }

	template<typename Exp>
	typename std::enable_if<is_filter<Exp>::value, ast::UnaryExpression<Exp, std::logical_not<typename Exp::value_type> > >::type
		operator!(const Exp& exp)
	{ return ast::UnaryExpression<Exp, std::logical_not<typename Exp::value_type> >(exp); }

#define BINARY_OP(Sym, Op, Class) \
	template<typename Left, typename Right> \
	typename std::enable_if<is_filter<Left>::value && is_filter<Right>::value, Class<Left, Right, Op> >::type \
	operator Sym(const Left& left, const Right& right) \
	{ return Class<Left, Right, Op>(left, right); } \
	\
	template<typename Left, typename Right> \
	typename std::enable_if<is_filter<Left>::value && !is_filter<Right>::value, Class<Left, Constant<Right>, Op> >::type \
	operator Sym(const Left& left, const Right& right) \
	{ return Class<Left, Constant<Right>, Op>(left, right); } \
	\
	template<typename Left, typename Right> \
	typename std::enable_if<!is_filter<Left>::value && is_filter<Right>::value, Class<Constant<Left>, Right, Op> >::type \
	operator Sym(const Left& left, const Right& right) \
	{ return Class<Constant<Left>, Right, Op>(left, right); }

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
	BINARY_OP(&&, true, ast::BooleanShortcutExpression)
	BINARY_OP(||, false, ast::BooleanShortcutExpression)

#undef BINARY_OP

}}

#endif
