#include "Lang/parser.hpp"

#include <list>
#include <map>

#include "Lang/ast.hpp"
#include "Util/memorybuffer.hpp"

#include "sloc_iterator.hpp"
#include "spirit_workarounds.hpp"

#include <boost/asio/ip/address_v6.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

namespace hbt {
namespace lang {

namespace {

enum {
	TOKEN_ANY = 1000,
};

template<typename Lexer>
struct tokenizer : boost::spirit::lex::lexer<Lexer> {
	void add(boost::spirit::lex::token_def<>& token, int id = 0)
	{
		this->self.add(token, id);
	}

	tokenizer()
	{
		add(linecomment = R"((?-s:\/\/.*))");
		add(blockcomment = R"((?s:\/\*.*?\*\/))");
		add(whitespace = R"([\r\n\t ]+)");

		add(type.bool_ = "bool");
		add(type.float_ = "float");
		add(type.uint8_ = "uint8");
		add(type.uint32_ = "uint32");
		add(type.uint64_ = "uint64");

		add(colon = ":");
		add(comma = ",");
		add(semicolon = ";");
		add(dot = R"(\.)");
		add(lparen = R"(\()");
		add(rparen = R"(\))");
		add(lbrace = R"(\{)");
		add(rbrace = R"(\})");

		add(op.notequal = "!=");
		add(op.not_ = "!");
		add(op.shl = "<<");
		add(op.shr = ">>");
		add(op.lesseq = "<=");
		add(op.greatereq = ">=");
		add(op.less = "<");
		add(op.greater = ">");
		add(op.boolAnd = "&&");
		add(op.boolOr = R"(\|\|)");

		add(op.plus = R"(\+)");
		add(op.minus = "-");
		add(op.neg = "~");
		add(op.mul = R"(\*)");
		add(op.div = R"(\/)");
		add(op.mod = "%");
		add(op.equal = "==");
		add(op.and_ = "&");
		add(op.or_ = R"(\|)");
		add(op.xor_ = R"(\^)");
		add(op.qmark = R"(\?)");
		add(op.assign = "=");
		add(op.write = ":=");

		add(word.machine = "machine");
		add(word.device = "device");
		add(word.endpoint = "endpoint");
		add(word.include = "include");
		add(word.read = "read");
		add(word.write = "write");
		add(word.global_write = "global_write");
		add(word.broadcast = "broadcast");
		add(word.class_ = "class");
		add(word.state = "state");
		add(word.on = "on");
		add(word.if_ = "if");
		add(word.else_ = "else");
		add(word.switch_ = "switch");
		add(word.case_ = "case");
		add(word.default_ = "default");
		add(word.goto_ = "goto");
		add(word.packet = "packet");
		add(word.eid = "eid");
		add(word.from = "from");
		add(word.entry = "entry");
		add(word.exit = "exit");
		add(word.periodic = "periodic");
		add(word.timeout = "timeout");

		add(lit.bool_ = "true|false");
		add(ident = R"([_a-zA-Z][_0-9a-zA-Z]*)");
		add(string = R"(\"[^\r\n"]*\")");

		add(lit.float_ = R"(((\d+\.\d*)|(\d*\.\d+))([eE][-+]\d+)?)");
		add(lit.uint8_ = R"(\d+[sS])");
		add(lit.uint32_ = R"(\d+)");
		add(lit.uint64_ = R"(\d+[lL])");

		add(any = ".", TOKEN_ANY);
	}

	boost::spirit::lex::token_def<>
		lparen, rparen, lbrace, rbrace,
		ident, string,
		colon, comma, semicolon, dot,
		linecomment, blockcomment, whitespace,
		any;

	struct {
		boost::spirit::lex::token_def<> bool_, float_, uint8_, uint32_, uint64_;
	} lit, type;
	struct {
		boost::spirit::lex::token_def<>
			plus, minus, not_, neg,
			mul, div, mod,
			less, lesseq, greater, greatereq,
			equal, notequal,
			and_, or_, xor_,
			boolAnd, boolOr,
			qmark,
			assign, write,
			shl, shr;
	} op;
	struct {
		boost::spirit::lex::token_def<>
			machine, device, endpoint, include, read, write, global_write, broadcast, class_, state, on, if_, else_,
			switch_, case_, default_, goto_, packet, eid, from, entry, exit, periodic, timeout;
	} word;
};

template<typename It>
struct whitespace : qi::grammar<It> {
	template<typename TokenDef>
	whitespace(const TokenDef& tok) : whitespace::base_type(start)
	{
		using namespace qi;

		start =
			tok.whitespace | tok.linecomment | tok.blockcomment;
	}

	qi::rule<It> start;
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

template<typename Fn, typename... Args>
static auto bindl(Fn fn, Args&&... args)
	-> decltype(boost::phoenix::bind(lambda_binder<Fn>{fn}, args...))
{
	return boost::phoenix::bind(lambda_binder<Fn>{fn}, args...);
}

template<typename It>
struct grammar : qi::grammar<It, std::list<std::unique_ptr<ProgramPart>>(), whitespace<It>> {
	typedef boost::iterator_range<typename It::base_iterator_type> range;

	template<typename T>
	using opt = boost::optional<T>;

	template<typename T>
	using ptr = spirit_workarounds::ptr<T>;

	template<typename... T>
	using rule = qi::rule<It, T..., whitespace<It>>;

	template<typename T>
	static std::vector<std::unique_ptr<T>> move(std::vector<ptr<T>>& vec)
	{
		std::vector<std::unique_ptr<T>> result;

		result.reserve(vec.size());
		for (auto& p : vec) {
			if (p) {
				result.emplace_back(std::move(p));
			} else {
				result.emplace_back(nullptr);
			}
		}

		return result;
	}

	template<typename T>
	static std::vector<std::unique_ptr<T>> move(opt<std::vector<ptr<T>>>& vec)
	{
		return vec ? move(*vec) : std::vector<std::unique_ptr<T>>();
	}

	template<typename T, template<typename> class Wrapper>
	static std::vector<T> unpack(std::vector<Wrapper<T>>& vec)
	{
		std::vector<T> result;

		result.reserve(vec.size());
		for (auto& p : vec)
			result.emplace_back(std::move(*p));

		return result;
	}

	template<typename T, template<typename> class Wrapper>
	static std::vector<T> unpack(opt<std::vector<Wrapper<T>>>& vec)
	{
		return vec ? unpack(*vec) : std::vector<T>();
	}

	template<typename Fn, typename... Args>
	static auto make(Fn fn, Args&&... args)
		-> decltype(qi::_val = bindl(fn, args...))
	{
		return qi::_val = bindl(fn, args...);
	}

	static std::string str(const range& range)
	{
		return std::string(range.begin(), range.end());
	}

	template<typename T>
	struct locd {
		SourceLocation loc;
		T val;
	};

	template<typename T>
	static T parseUI(const range& r, bool& pass)
	{
		T val;
		pass = qi::parse(r.begin(), r.end(), qi::uint_parser<T, 10, 1, 99>(), val);
		return val;
	}

	template<typename T>
	TypedLiteral<T>* convUI(const range& r, bool& pass)
	{
		return new TypedLiteral<T>(locOf(r), parseUI<T>(r, pass));
	}

	TypedLiteral<float>* convF(const range& r, bool& pass)
	{
		float val;
		pass = qi::parse(r.begin(), r.end(), qi::float_, val);

		return new TypedLiteral<float>(locOf(r), val);
	}

	template<typename TokenDef>
	grammar(const TokenDef& tok, int tabWidth)
		: grammar::base_type(start), tabWidth(tabWidth)
	{
		using namespace qi;
		using namespace boost::phoenix;

		unexpected = !eps;
		auto expected = [this] (std::string msg) {
			return eps[bindl([this, msg] () {
					unexpected.name(msg);
				})] > unexpected;
		};

		start =
			*(
				machine_class[push_back(_val, _1)]
				| machine_def[push_back(_val, _1)]
				| machine_inst[push_back(_val, _1)]
				| endpoint[push_back(_val, _1)]
				| device[push_back(_val, _1)]
				| include[push_back(_val, _1)]
				| !eoi >> expected("toplevel element")
			);

		e.primary =
			e.literal[_val = _1]
			| (tok.lparen >> expr >> tok.rparen)[_val = _2]
			| (tok.word.timeout)[make(
				[this] (range& r) {
					return new TimeoutExpr(locOf(r));
				}, _1)]
			| (tok.word.packet >> tok.dot >> tok.word.eid)[make(
				[this] (range& r) {
					return new PacketEIDExpr(locOf(r));
				}, _1)]
			| (identifier >> tok.dot >> identifier)[make(
				[this] (opt<Identifier>& dev, opt<Identifier>& ep) {
					return new EndpointExpr(dev->sloc(), *dev, *ep, Type::Unknown);
				}, _1, _3)]
			| (tok.ident)[make(
				[this] (range& id) {
					return new IdentifierExpr(locOf(id), str(id), Type::Unknown);
				}, _1)];

		e.callOrCast =
			(datatype >> tok.lparen >> expr >> tok.rparen)[make(
				[this] (opt<locd<Type>>& dt, ptr<Expr>& e) {
					return new CastExpr(std::move(dt->loc), dt->val, e);
				}, _1, _3)]
			| (identifier >> tok.lparen >> -(expr % omit[tok.comma]) >> tok.rparen)[make(
				[this] (opt<Identifier>& id, opt<std::vector<ptr<Expr>>>& args) {
					return new CallExpr(id->sloc(), id->name(), move(args), Type::Unknown);
				}, _1, _3)]
			| e.primary[_val = _1];

		auto unop = [this] (range& r, UnaryOperator op) {
			return locd<UnaryOperator>{locOf(r), op};
		};
		e.unop =
			tok.op.plus[make(unop, _1, UnaryOperator::Plus)]
			| tok.op.minus[make(unop, _1, UnaryOperator::Minus)]
			| tok.op.not_[make(unop, _1, UnaryOperator::Not)]
			| tok.op.neg[make(unop, _1, UnaryOperator::Negate)];
		e.unary =
			e.callOrCast[_val = _1]
			| (e.unop >> e.unary)[make(
				[this] (opt<locd<UnaryOperator>>& op, ptr<Expr>& e) {
					return new UnaryExpr(std::move(op->loc), op->val, e);
				}, _1, _2)];

		auto binop = [this] (range& r, BinaryOperator op) {
			return locd<BinaryOperator>{locOf(r), op};
		};
		auto updateBinop = [this] (opt<locd<BinaryOperator>>& op, ptr<Expr>& e, ptr<Expr>& val) {
			return new BinaryExpr(std::move(op->loc), val, op->val, e);
		};
		auto updateBinopT = [this] (range& r, BinaryOperator op, ptr<Expr>& e, ptr<Expr>& val) {
			return new BinaryExpr(locOf(r), val, op, e);
		};

		e.binop.mul =
			tok.op.mul[make(binop, _1, BinaryOperator::Multiply)]
			| tok.op.div[make(binop, _1, BinaryOperator::Divide)]
			| tok.op.mod[make(binop, _1, BinaryOperator::Modulo)];
		e.mul =
			e.unary[_val = _1]
			>> *(
				(e.binop.mul >> e.unary)[make(updateBinop, _1, _2, _val)]
			);

		e.binop.add =
			tok.op.plus[make(binop, _1, BinaryOperator::Plus)]
			| tok.op.minus[make(binop, _1, BinaryOperator::Minus)];
		e.add =
			e.mul[_val = _1]
			>> *(
				(e.binop.add >> e.mul)[make(updateBinop, _1, _2, _val)]
			);

		e.binop.shift =
			tok.op.shl[make(binop, _1, BinaryOperator::ShiftLeft)]
			| tok.op.shr[make(binop, _1, BinaryOperator::ShiftRight)];
		e.shift =
			e.add[_val = _1]
			>> *(
				(e.binop.shift >> e.add)[make(updateBinop, _1, _2, _val)]
			);

		e.binop.rel =
			tok.op.lesseq[make(binop, _1, BinaryOperator::LessOrEqual)]
			| tok.op.less[make(binop, _1, BinaryOperator::LessThan)]
			| tok.op.greatereq[make(binop, _1, BinaryOperator::GreaterOrEqual)]
			| tok.op.greater[make(binop, _1, BinaryOperator::GreaterThan)];
		e.rel =
			e.shift[_val = _1]
			>> *(
				(e.binop.rel >> e.shift)[make(updateBinop, _1, _2, _val)]
			);

		e.and_ =
			e.rel[_val = _1]
			>> *(
				(tok.op.and_ >> e.rel)[make(updateBinopT, _1, BinaryOperator::And, _2, _val)]
			);

		e.or_ =
			e.and_[_val = _1]
			>> *(
				(tok.op.or_ >> e.and_)[make(updateBinopT, _1, BinaryOperator::Or, _2, _val)]
			);

		e.boolAnd =
			e.or_[_val = _1]
			>> *(
				(tok.op.boolAnd >> e.or_)[make(updateBinopT, _1, BinaryOperator::BoolAnd, _2, _val)]
			);

		e.boolOr =
			e.boolAnd[_val = _1]
			>> *(
				(tok.op.boolOr >> e.boolAnd)[make(updateBinopT, _1, BinaryOperator::BoolOr, _2, _val)]
			);

		e.cond =
			(e.boolOr >> !tok.op.qmark)[_val = _1]
			| (e.boolOr >> tok.op.qmark >> expr >> tok.colon >> expr)[make(
				[this] (ptr<Expr>& cond, ptr<Expr>& ifTrue, ptr<Expr>& ifFalse) {
					return new ConditionalExpr(cond->sloc(), cond, ifTrue, ifFalse);
				}, _1, _3, _5)];

		expr %=
			e.cond;

		e.literal =
			tok.lit.bool_[make(
				[this] (range& r) {
					return new TypedLiteral<bool>(locOf(r), str(r) == "true");
				}, _1)]
			| tok.lit.uint8_[make(
				[this] (range& r, bool& pass) {
					return convUI<uint8_t>(r, pass);
				}, _1, _pass)]
			| tok.lit.uint32_[make(
				[this] (range& r, bool& pass) {
					return convUI<uint32_t>(r, pass);
				}, _1, _pass)]
			| tok.lit.uint64_[make(
				[this] (range& r, bool& pass) {
					return convUI<uint64_t>(r, pass);
				}, _1, _pass)]
			| tok.lit.float_[make(
				[this] (range& r, bool& pass) {
					return convF(r, pass);
					}, _1, _pass)];

		statement %=
			s.assign | s.if_ | s.switch_ | s.write | s.block | s.decl | s.goto_;

		s.assign =
			(identifier >> tok.op.assign >> expr >> tok.semicolon)[make(
				[this] (opt<Identifier>& id, range& r, ptr<Expr>& e) {
					return new AssignStmt(locOf(r), std::move(*id), e);
				}, _1, _2, _3)];

		s.if_ =
			(tok.word.if_ >> tok.lparen >> expr >> tok.rparen >> statement >> -(omit[tok.word.else_] >> statement))[
				make(
					[this] (range& r, ptr<Expr>& cond, ptr<Stmt>& ifTrue, opt<ptr<Stmt>>& ifFalse) {
						return new IfStmt(locOf(r), cond, ifTrue, ifFalse ? *ifFalse : ptr<Stmt>());
					}, _1, _3, _5, _6)];

		s.switch_ =
			(tok.word.switch_ >> tok.lparen >> expr >> tok.rparen >> tok.lbrace >> s.switch_body >> tok.rbrace)[
				make(
					[this] (range& r, ptr<Expr>& e, ptr<std::vector<SwitchEntry>>& entries) {
						return new SwitchStmt(locOf(r), e, std::move(*entries));
					}, _1, _3, _6)];

		auto appendSwitchEntry = [] (ptr<std::vector<SwitchEntry>>& entries, std::vector<ptr<Expr>>& labels, ptr<Stmt>& stmt) {
			entries->emplace_back(move(labels), stmt);
			return entries;
		};
		s.switch_body =
			eps[_val = new_<std::vector<SwitchEntry>>()]
			>> +((s.switch_labels >> statement)[make(appendSwitchEntry, _val, _1, _2)]);

		s.switch_labels =
			+(
				(tok.word.case_ >> expr >> tok.colon)[push_back(_val, _2)]
				| (tok.word.default_ >> tok.colon)[push_back(_val, nullptr)]
			);

		s.write =
			(identifier >> tok.dot >> identifier >> tok.op.write >> expr >> tok.semicolon)[
				make(
					[this] (opt<Identifier>& dev, opt<Identifier>& ep, range& r, ptr<Expr>& e) {
						return new WriteStmt(locOf(r), *dev, *ep, e);
					}, _1, _3, _4, _5)];

		s.block =
			(tok.lbrace >> *statement >> tok.rbrace)[make(
				[this] (range& r, std::vector<ptr<Stmt>>& block) {
					return new BlockStmt(locOf(r), move(block));
				}, _1, _2)];

		s.decl =
			(datatype >> identifier >> -(omit[tok.op.assign] >> expr) >> tok.semicolon)[make(
				[this] (opt<locd<Type>>& dt, opt<Identifier>& id, opt<ptr<Expr>>& e) {
					return new DeclarationStmt(std::move(dt->loc), dt->val, std::move(*id), e ? *e : ptr<Expr>());
				}, _1, _2, _3)];

		s.goto_ =
			(tok.word.goto_ >> identifier >> tok.semicolon)[make(
				[this] (range& r, opt<Identifier> to) {
					return new GotoStmt(locOf(r), std::move(*to));
				}, _1, _2)];

		auto onSimple = [this] (range& r, OnBlockTrigger trigger, ptr<BlockStmt>& block) {
			return new OnBlock(locOf(r), trigger, block);
		};
		on_block =
			(tok.word.on >> tok.word.entry >> s.block)[make(onSimple, _1, OnBlockTrigger::Entry, _3)]
			| (tok.word.on >> tok.word.exit >> s.block)[make(onSimple, _1, OnBlockTrigger::Exit, _3)]
			| (tok.word.on >> tok.word.periodic >> s.block)[make(onSimple, _1, OnBlockTrigger::Periodic, _3)]
			| (tok.word.on >> tok.word.packet >> tok.word.from >> identifier >> s.block)[make(
				[this] (range& r, opt<Identifier>& from, ptr<BlockStmt>& block) {
					return new OnPacketBlock(locOf(r), *from, block);
				}, _1, _4, _5)]
			| (tok.word.on >> tok.lparen >> expr >> tok.rparen >> s.block)[make(
				[this] (range& r, ptr<Expr>& e, ptr<BlockStmt>& block) {
					return new OnExprBlock(locOf(r), e, block);
				}, _1, _3, _5)];

		state =
			(tok.word.state >> identifier >> tok.lbrace >> *s.decl >> *on_block >> *statement >> tok.rbrace)[
				make(
					[this] (range& r, opt<Identifier>& id, std::vector<ptr<DeclarationStmt>>& decls,
							std::vector<ptr<OnBlock>>& onBlocks, std::vector<ptr<Stmt>>& stmts) {
						return new State(locOf(r), std::move(*id), unpack(decls), move(onBlocks), move(stmts));
					}, _1, _2, _4, _5, _6)];

		machine_class =
			(tok.word.class_ >> identifier >> tok.lparen >> -(identifier % tok.comma) >> tok.rparen >>
				tok.lbrace >> *s.decl >> *state >> tok.rbrace)[make(
					[this] (range& r, opt<Identifier>& id, opt<std::vector<opt<Identifier>>>& params,
							std::vector<ptr<DeclarationStmt>>& decls, std::vector<ptr<State>>& states) {
						return new MachineClass(locOf(r), std::move(*id), unpack(params), unpack(decls), unpack(states));
					}, _1, _2, _4, _7, _8)];

		machine_def =
			(tok.word.machine >> identifier >> tok.colon >> tok.lbrace >> *s.decl >> *state >> tok.rbrace)[make(
				[this] (range& r, opt<Identifier>& id, std::vector<ptr<DeclarationStmt>>& decls, std::vector<ptr<State>>& states) {
					return new MachineDefinition(locOf(r), std::move(*id), unpack(decls), unpack(states));
				}, _1, _2, _5, _6)];

		machine_inst =
			(tok.word.machine >> identifier >> tok.colon >> identifier >> tok.lparen >> -(expr % tok.comma) >>
				tok.rparen >> tok.semicolon)[make(
					[this] (range& r, opt<Identifier>& name, opt<Identifier>& class_, opt<std::vector<ptr<Expr>>>& args) {
						return new MachineInstantiation(locOf(r), std::move(*name), *class_, move(args));
					}, _1, _2, _4, _6)];

		endpoint =
			(tok.word.endpoint >> identifier >> omit[tok.lparen] >> tok.lit.uint32_ >> omit[tok.rparen] >> tok.colon
				>> datatype >> tok.lparen >> endpoint_access >> tok.rparen >> tok.semicolon)[make(
					[this] (range& r, opt<Identifier>& name, range& eid, opt<locd<Type>>& dt, EndpointAccess access, bool& pass) {
						return new Endpoint(locOf(r), *name, parseUI<uint32_t>(eid, pass), dt->val, access);
					}, _1, _2, _3, _5, _7, _pass)];

		device =
			(tok.word.device > identifier > tok.lparen > ip_addr > tok.rparen > tok.colon
				> -(identifier % omit[tok.comma]) > tok.semicolon)[make(
					[this] (range& r, opt<Identifier>& name, std::array<uint8_t, 16>& ip,
							opt<std::vector<opt<Identifier>>>& eps) {
						return new Device(locOf(r), *name, ip, unpack(eps));
					}, _1, _2, _4, _7)];

		include =
			(tok.word.include >> tok.string >> tok.semicolon)[make(
				[this] (range& r, range& file) {
					return new IncludeLine(locOf(r), std::string(file.begin().base() + 1, file.end().base() - 1));
				}, _1, _2)];

		ip_addr =
			lexeme[+(tok.colon | tok.dot | tok.lit.uint32_ | tok.ident)][make(
				[] (std::vector<range>& addr, bool& pass) {
					std::array<uint8_t, 16> result;
					try {
						std::string str(addr.front().begin(), addr.back().end());
						auto parsed = boost::asio::ip::address_v6::from_string(str);
						auto bytes = parsed.to_bytes();
						std::copy(bytes.begin(), bytes.end(), result.begin());
					} catch (...) {
						pass = false;
					}
					return result;
				}, _1, _pass)];

		endpoint_access =
			eps[_val = EndpointAccess(0)]
			>> -(
				(
					tok.word.read[_val |= EndpointAccess::Read]
					| tok.word.write[_val |= EndpointAccess::Write]
					| tok.word.global_write[_val |= EndpointAccess::NonLocalWrite | EndpointAccess::Write]
					| tok.word.broadcast[_val |= EndpointAccess::Broadcast]
				) % tok.comma
			);

		datatype.name("datatype");
		datatype =
			(
				tok.type.bool_[_a = Type::Bool]
				| tok.type.uint8_[_a = Type::UInt8]
				| tok.type.uint32_[_a = Type::UInt32]
				| tok.type.uint64_[_a = Type::UInt64]
				| tok.type.float_[_a = Type::Float]
			)[make([this] (range& r, Type t) {
					return locd<Type>{locOf(r), t};
				}, _1, _a)];

		identifier.name("identifier");
		identifier =
			tok.ident[make(
				[this] (range& id) {
					return Identifier(locOf(id), str(id));
				}, _1)];
	}

	std::shared_ptr<std::string> file;

	rule<std::list<std::unique_ptr<ProgramPart>>()> start;

	rule<ptr<Expr>()> expr;
	struct {
		rule<ptr<Expr>()> primary, callOrCast, unary, mul, add, shift, rel, eq, and_, or_, xor_, boolAnd, boolOr, cond;
		rule<ptr<Expr>()> literal;

		struct {
			rule<opt<locd<BinaryOperator>>()> mul, add, shift, rel;
		} binop;

		rule<opt<locd<UnaryOperator>>()> unop;
	} e;

	rule<ptr<Stmt>()> statement;
	struct {
		rule<ptr<Stmt>()> assign, if_, switch_, write;
		rule<ptr<BlockStmt>()> block;
		rule<ptr<DeclarationStmt>()> decl;
		rule<ptr<Stmt>()> goto_;

		rule<ptr<std::vector<SwitchEntry>>()> switch_body;
		rule<std::vector<ptr<Expr>>()> switch_labels;
	} s;

	rule<ptr<OnBlock>()> on_block;

	rule<ptr<State>()> state;

	rule<ptr<ProgramPart>()> machine_class;
	rule<ptr<ProgramPart>()> machine_def;
	rule<ptr<ProgramPart>()> machine_inst;
	rule<ptr<ProgramPart>()> endpoint;
	rule<ptr<ProgramPart>()> device;
	rule<ptr<ProgramPart>()> include;

	rule<std::array<uint8_t, 16>()> ip_addr;
	rule<EndpointAccess()> endpoint_access;
	rule<opt<locd<Type>>(), qi::locals<Type>> datatype;
	rule<opt<Identifier>()> identifier;

	rule<> unexpected;

	int tabWidth;

	SourceLocation locOf(const range& range)
	{
		return SourceLocation(file.get(), getLine(range.begin()), getColumn(range.begin(), tabWidth));
	}
};

}



std::unique_ptr<TranslationUnit> parse(const hbt::util::MemoryBuffer& input)
{
	typedef sloc_iterator<const char*> iter;
	typedef boost::spirit::lex::lexertl::token<iter> token_type;
	typedef boost::spirit::lex::lexertl::lexer<token_type> lexer_type;
	typedef typename tokenizer<lexer_type>::iterator_type iterator_type;

	auto ctext = input.crange<char>();

	std::list<std::unique_ptr<ProgramPart>> parts;
	int tabWidth = 4;

	tokenizer<lexer_type> t;
	grammar<iterator_type> g(t, tabWidth);
	whitespace<iterator_type> ws(t);

	try {
		iter first(ctext.begin()), last(ctext.end());
		bool result = boost::spirit::lex::tokenize_and_phrase_parse(first, last, t, g, ws, parts);

		if (!result || first != last)
			throw ParseError(0, 0, "...not this anyway...", "parsing aborted (internal error)", "problems");
	} catch (const qi::expectation_failure<iterator_type>& ef) {
		std::string badToken;
		int line = -1, col = -1;

		if (ef.first->is_valid()) {
			if (ef.first->id() == TOKEN_ANY) {
				unsigned val = (unsigned char) *ef.first->value().begin();
				char buf[16];
				sprintf(buf, "\\x%02x", val);
				badToken = buf;
			} else {
				badToken = std::string(ef.first->value().begin(), ef.first->value().end());
			}
			line = getLine(ef.first->value().begin());
			col = getColumn(ef.first->value().begin(), tabWidth);
		} else {
			badToken = "<EOF>";
		}

		std::string rule_name = ef.what_.tag;
		std::string expected, detail;

		if (rule_name[0] == '~') {
			size_t sep = rule_name.find('~', 1);
			expected = rule_name.substr(1, sep - 1);
			detail = rule_name.substr(sep + 1);
		} else {
			expected = rule_name;
		}

		throw ParseError(line, col, expected, detail, badToken);
	}

	return std::unique_ptr<TranslationUnit>(
		new TranslationUnit(
			std::unique_ptr<std::string>(),
			std::vector<std::unique_ptr<ProgramPart>>(
				std::make_move_iterator(parts.begin()),
				std::make_move_iterator(parts.end()))));
}

}
}
