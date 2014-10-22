#include "Lang/parser.hpp"

#include <cctype>
#include <list>
#include <set>

#include "Lang/ast.hpp"
#include "Util/memorybuffer.hpp"

#include "sloc_iterator.hpp"
#include "spirit_workarounds.hpp"

#include <boost/filesystem.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

namespace hbt {
namespace lang {

namespace {

enum {
	TOKEN_ANY = 1,
	TOKEN_SPACE = 2,
	TOKEN_LINECOMMENT = 3,
};

template<typename Lexer>
struct tokenizer : boost::spirit::lex::lexer<Lexer> {
	void add(boost::spirit::lex::token_def<>& token, int id = 0)
	{
		this->self.add(token, id);
	}

	tokenizer()
	{
		add(linecomment = R"((?-s:\/\/.*))", TOKEN_LINECOMMENT);
		add(blockcomment = R"(\/\*([^*]|\*+[^/*])*\*+\/)", TOKEN_SPACE);
		add(unterminated_blockcomment = R"(\/\*([^*]|\*+[^/*])*)");
		add(whitespace = R"([\r\n\t ]+)", TOKEN_SPACE);

		add(type.bool_ = "bool");
		add(type.float_ = "float");
		add(type.uint8_ = "uint8");
		add(type.uint16_ = "uint16");
		add(type.uint32_ = "uint32");
		add(type.uint64_ = "uint64");
		add(type.int8_ = "int8");
		add(type.int16_ = "int16");
		add(type.int32_ = "int32");
		add(type.int64_ = "int64");

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
		add(word.from = "from");
		add(word.entry = "entry");
		add(word.exit = "exit");
		add(word.periodic = "periodic");
		add(word.always = "always");

		add(lit.bool_ = "true|false");
		add(string = R"(\"[^\r\n"]*\")");

		add(lit.float_ = R"(-?(((\d+\.\d*)|(\d*\.\d+))([eE][-+]?\d+)?|(\d+[eE][-+]?\d+)|nan|inf))");
		add(lit.uint_ = R"(\d+)");
		add(lit.sint_ = R"(-\d+)");

		add(ident = R"([_a-zA-Z][_0-9a-zA-Z]*)");

		add(any = ".", TOKEN_ANY);
	}

	boost::spirit::lex::token_def<>
		lparen, rparen, lbrace, rbrace,
		ident, string,
		colon, comma, semicolon, dot,
		linecomment, blockcomment, unterminated_blockcomment, whitespace,
		any;

	struct {
		boost::spirit::lex::token_def<> bool_, float_, uint8_, uint16_, uint32_, uint64_, int8_, int16_, int32_, int64_;
	} type;
	struct {
		boost::spirit::lex::token_def<> bool_, float_, sint_, uint_;
	} lit;
	struct {
		boost::spirit::lex::token_def<>
			plus, minus, not_, neg,
			mul, div, mod,
			less, lesseq, greater, greatereq,
			equal, notequal,
			and_, or_, xor_,
			boolAnd, boolOr,
			qmark,
			assign,
			shl, shr;
	} op;
	struct {
		boost::spirit::lex::token_def<>
			machine, device, endpoint, include, read, write, global_write, broadcast, class_, state, on, if_, else_,
			switch_, case_, default_, goto_, packet, from, entry, exit, periodic, always;
	} word;
};

template<typename It>
struct whitespace : qi::grammar<It> {
	typedef typename It::base_iterator_type base_it;
	typedef boost::iterator_range<base_it> range;

	static std::string str(const range& range)
	{
		return std::string(range.begin(), range.end());
	}

	template<typename TokenDef>
	whitespace(const TokenDef& tok, std::vector<std::pair<unsigned, std::string>>* expectations)
		: whitespace::base_type(start)
	{
		using namespace qi;

		start =
			tok.whitespace
			| tok.linecomment[spirit_workarounds::fwd > [this, expectations] (range& r) {
				if (!expectations)
					return;

				if (!expectationRangesProcessed.insert({ r.begin(), r.end() }).second)
					return;

				std::string line = str(r);
				static const char prefix[] = "//#expect: ";
				if (line.substr(0, strlen(prefix)) == prefix) {
					expectations->emplace_back(getLine(r.begin()), line.substr(strlen(prefix)));
					lastLine = getLine(r.begin());
				}


				static const char prefixMore[] = "//#expect+: ";
				if (line.substr(0, strlen(prefixMore)) == prefixMore) {
					expectations->emplace_back(lastLine, line.substr(strlen(prefixMore)));
				}
			}]
			| tok.blockcomment
			| (tok.unterminated_blockcomment > unterminated);

		unterminated.name("blockcomment terminator");
		unterminated = !eps;
	}

	qi::rule<It> start;
	qi::rule<It> unterminated;
	std::set<std::pair<base_it, base_it>> expectationRangesProcessed;
	unsigned lastLine;
};

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
	static std::vector<std::unique_ptr<T>> move(std::vector<ptr<T>>* vec)
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
	static std::vector<T> unpack(std::vector<Wrapper<T>>* vec)
	{
		return vec ? unpack(*vec) : std::vector<T>();
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

	Literal* convUI(const range& r, bool& pass)
	{
		if (*r.begin() == '-') {
			int64_t i;
			pass = qi::parse(r.begin(), r.end(), qi::int_parser<int64_t, 10, 1, 99>(), i);
			if (i >= INT32_MIN)
				return new TypedLiteral<int32_t>(locOf(r), i);
			else
				return new TypedLiteral<int64_t>(locOf(r), i);
		} else {
			uint64_t u;
			pass = qi::parse(r.begin(), r.end(), qi::int_parser<uint64_t, 10, 1, 99>(), u);
			if (u <= INT32_MAX)
				return new TypedLiteral<int32_t>(locOf(r), u);
			else if (u <= UINT32_MAX)
				return new TypedLiteral<uint32_t>(locOf(r), u);
			else if (u <= INT64_MAX)
				return new TypedLiteral<int64_t>(locOf(r), u);
			else
				return new TypedLiteral<uint64_t>(locOf(r), u);
		}
	}

	TypedLiteral<float>* convF(const range& r, bool& pass)
	{
		float val;
		pass = qi::parse(r.begin(), r.end(), qi::float_, val);
		if (str(r) != "inf" && str(r) != "-inf")
			pass &= std::fpclassify(val) != FP_INFINITE;

		return new TypedLiteral<float>(locOf(r), val);
	}

	template<typename TokenDef>
	grammar(const TokenDef& tok, const std::string* file, const SourceLocation* includedFrom, int tabWidth)
		: grammar::base_type(start), file(file), includedFrom(includedFrom), tabWidth(tabWidth)
	{
		using namespace qi;
		using namespace boost::phoenix;
		using namespace spirit_workarounds;

		unexpected = !eps;
#define expected(s) (omit[eps[fwd > [this] () { unexpected.name(s); }] > unexpected])

		start =
			*(
				machine_class[push_back(_val, _1)]
				| machine_spec[push_back(_val, _1)]
				| endpoint[push_back(_val, _1)]
				| device[push_back(_val, _1)]
				| include[push_back(_val, _1)]
				| !eoi >> expected("toplevel element")
			);

		e.primary.name("expression");
		e.primary =
			e.literal[_val = _1]
			| (tok.lparen > expr > tok.rparen)[_val = _2]
			| (identifier >> omit[tok.dot] > identifier)[fwd >= [this] (Identifier* dev, Identifier* ep) {
				return new EndpointExpr(dev->sloc(), *dev, *ep, Type::Int32);
			}]
			| tok.ident[fwd >= [this] (range& id) {
				return new IdentifierExpr(locOf(id), str(id), Type::Int32);
			}];

		e.callOrCast.name("expression");
		e.callOrCast =
			(
				datatype
				> omit[tok.lparen | expected("(")]
				> expr
				> omit[tok.rparen | expected(")")]
			)[fwd >= [this] (locd<Type>* dt, ptr<Expr>& e) {
				return new CastExpr(std::move(dt->loc), dt->val, e);
			}]
			| (
				identifier
				>> tok.lparen
				> -(expr % omit[tok.comma])
				> omit[tok.rparen | expected(")")]
			)[fwd >= [this] (Identifier* id, range& r, std::vector<ptr<Expr>>* args) {
				return new CallExpr(locOf(r), *id, move(args), Type::Int32);
			}]
			| e.primary[_val = _1];

		auto unop = [this] (UnaryOperator op) {
			return [this, op] (range& r) {
				return locd<UnaryOperator>{locOf(r), op};
			};
		};
		e.unop =
			tok.op.plus[fwd >= unop(UnaryOperator::Plus)]
			| tok.op.minus[fwd >= unop(UnaryOperator::Minus)]
			| tok.op.not_[fwd >= unop(UnaryOperator::Not)]
			| tok.op.neg[fwd >= unop(UnaryOperator::Negate)];
		e.unary.name("expression");
		e.unary =
			e.callOrCast[_val = _1]
			| (e.unop > e.unary)[fwd >= [this] (locd<UnaryOperator>* op, ptr<Expr>& e) {
				return new UnaryExpr(std::move(op->loc), op->val, e);
			}];

		auto binop = [this] (BinaryOperator op) {
			return [this, op] (range& r) {
				return locd<BinaryOperator>{locOf(r), op};
			};
		};
		auto updateBinop = [this] (ptr<Expr>& val, locd<BinaryOperator>* op, ptr<Expr>& e) {
			return new BinaryExpr(std::move(op->loc), val, op->val, e);
		};
		auto updateBinopT = [this] (BinaryOperator op) {
			return [this, op] (ptr<Expr>& val, range& r, ptr<Expr>& e) {
				return new BinaryExpr(locOf(r), val, op, e);
			};
		};

		e.binop.mul =
			tok.op.mul[fwd >= binop(BinaryOperator::Multiply)]
			| tok.op.div[fwd >= binop(BinaryOperator::Divide)]
			| tok.op.mod[fwd >= binop(BinaryOperator::Modulo)];
		e.mul.name("expression");
		e.mul =
			e.unary[_val = _1]
			>> *(
				(e.binop.mul > e.unary)[fwd >>= updateBinop]
			);

		e.binop.add =
			tok.op.plus[fwd >= binop(BinaryOperator::Plus)]
			| tok.op.minus[fwd >= binop(BinaryOperator::Minus)];
		e.add.name("expression");
		e.add =
			e.mul[_val = _1]
			>> *(
				(e.binop.add > e.mul)[fwd >>= updateBinop]
			);

		e.binop.shift =
			tok.op.shl[fwd >= binop(BinaryOperator::ShiftLeft)]
			| tok.op.shr[fwd >= binop(BinaryOperator::ShiftRight)];
		e.shift.name("expression");
		e.shift =
			e.add[_val = _1]
			>> *(
				(e.binop.shift > e.add)[fwd >>= updateBinop]
			);

		e.binop.rel =
			tok.op.lesseq[fwd >= binop(BinaryOperator::LessOrEqual)]
			| tok.op.less[fwd >= binop(BinaryOperator::LessThan)]
			| tok.op.greatereq[fwd >= binop(BinaryOperator::GreaterOrEqual)]
			| tok.op.greater[fwd >= binop(BinaryOperator::GreaterThan)];
		e.rel.name("expression");
		e.rel =
			e.shift[_val = _1]
			>> *(
				(e.binop.rel > e.shift)[fwd >>= updateBinop]
			);

		e.eq.name("expression");
		e.eq =
			e.rel[_val = _1]
			>> *(
				(tok.op.equal > e.rel)[fwd >>= updateBinopT(BinaryOperator::Equals)]
				| (tok.op.notequal > e.rel)[fwd >>= updateBinopT(BinaryOperator::NotEquals)]
			);

		e.and_.name("expression");
		e.and_ =
			e.eq[_val = _1]
			>> *(
				(tok.op.and_ > e.eq)[fwd >>= updateBinopT(BinaryOperator::And)]
			);

		e.xor_.name("expression");
		e.xor_ =
			e.and_[_val = _1]
			>> *(
				(tok.op.xor_ > e.and_)[fwd >>= updateBinopT(BinaryOperator::Or)]
			);

		e.or_.name("expression");
		e.or_ =
			e.xor_[_val = _1]
			>> *(
				(tok.op.or_ > e.xor_)[fwd >>= updateBinopT(BinaryOperator::Or)]
			);

		e.boolAnd.name("expression");
		e.boolAnd =
			e.or_[_val = _1]
			>> *(
				(tok.op.boolAnd > e.or_)[fwd >>= updateBinopT(BinaryOperator::BoolAnd)]
			);

		e.boolOr.name("expression");
		e.boolOr =
			e.boolAnd[_val = _1]
			>> *(
				(tok.op.boolOr > e.boolAnd)[fwd >>= updateBinopT(BinaryOperator::BoolOr)]
			);

		e.cond.name("expression");
		e.cond =
			(e.boolOr >> !tok.op.qmark)[_val = _1]
			| (
				e.boolOr
				> omit[tok.op.qmark]
				> expr
				> omit[tok.colon | expected(":")]
				> expr
			)[fwd >= [this] (ptr<Expr>& cond, ptr<Expr>& ifTrue, ptr<Expr>& ifFalse) {
				return new ConditionalExpr(cond->sloc(), cond, ifTrue, ifFalse);
			}];

		expr.name("expression");
		expr %=
			e.cond;

		e.literal =
			tok.lit.bool_[fwd >= [this] (range& r) {
				return new TypedLiteral<bool>(locOf(r), str(r) == "true");
			}]
			| tok.lit.uint_[fwd >= [this] (range& r, bool& pass) {
				return convUI(r, pass);
			}]
			| tok.lit.sint_[fwd >= [this] (range& r, bool& pass) {
				return convUI(r, pass);
			}]
			| tok.lit.float_[fwd >= [this] (range& r, bool& pass) {
				return convF(r, pass);
			}];

		statement.name("statement");
		statement %=
			s.assign | s.if_ | s.switch_ | s.block | s.decl | s.goto_;

		s.assign.name("assign statement");
		s.assign =
			(
				identifier
				>> !omit[tok.dot]
				>> tok.op.assign
				> expr
				> omit[tok.semicolon | expected(";")]
			)[fwd >= [this] (Identifier* id, range& r, ptr<Expr>& e) {
				return new AssignStmt(locOf(r), std::move(*id), e);
			}]
			| (
				identifier
				> omit[tok.dot | expected(". or =")]
				> identifier
				> &omit[tok.op.assign | expected("=")]
				> tok.op.assign
				> expr
				> omit[tok.semicolon | expected(";")]
			)[fwd >= [this] (Identifier* dev, Identifier* ep, range& r, ptr<Expr>& e) {
				return new WriteStmt(locOf(r), EndpointExpr(locOf(r), *dev, *ep, Type::Int32), e);
			}];

		s.if_.name("if statement");
		s.if_ =
			(
				tok.word.if_
				> omit[tok.lparen | expected("(")]
				> expr
				> omit[tok.rparen | expected(")")]
				> statement
				>> -(
					omit[tok.word.else_]
					> statement
				)
			)[fwd >= [this] (range& r, ptr<Expr>& cond, ptr<Stmt>& ifTrue, ptr<Stmt>* ifFalse) {
				return new IfStmt(locOf(r), cond, ifTrue, ifFalse ? *ifFalse : ptr<Stmt>());
			}];

		s.switch_.name("switch statement");
		s.switch_ =
			(
				tok.word.switch_
				> omit[tok.lparen | expected("(")]
				> expr
				> omit[
					(tok.rparen | expected(")"))
					>> (tok.lbrace | expected("{"))
				]
				> s.switch_body
				> omit[tok.rbrace | expected("} or labels")]
			)[fwd >= [this] (range& r, ptr<Expr>& e, ptr<std::vector<SwitchEntry>>& entries) {
				return new SwitchStmt(locOf(r), e, std::move(*entries));
			}];

		s.switch_body.name("switch body");
		s.switch_body =
			eps[_val = new_<std::vector<SwitchEntry>>()]
			>> +(
				(s.switch_labels > statement)[
					fwd >> [] (ptr<std::vector<SwitchEntry>>& entries, std::vector<ptr<SwitchLabel>>& labels, ptr<Stmt>& stmt) {
						entries->emplace_back(unpack(labels), stmt);
					}]
			);

		s.switch_labels.name("switch label set");
		s.switch_labels =
			+(
				(tok.word.case_ > expr > omit[tok.colon | expected(":")])[
					fwd >> [this] (std::vector<ptr<SwitchLabel>>& vec, range& r, ptr<Expr>& l) {
						vec.emplace_back(new SwitchLabel(locOf(r), l));
					}]
				| (tok.word.default_ > omit[tok.colon | expected(":")])[
					fwd >> [this] (std::vector<ptr<SwitchLabel>>& vec, range& r) {
						vec.emplace_back(new SwitchLabel(locOf(r), nullptr));
					}]
			);

		s.block.name("block statement");
		s.block =
			(tok.lbrace > *statement > omit[tok.rbrace | expected("statements or }")])[
				fwd >= [this] (range& r, std::vector<ptr<Stmt>>& block) {
					return new BlockStmt(locOf(r), move(block));
				}];

		s.decl.name("declaration statement");
		s.decl =
			(
				datatype
				> identifier
				> omit[tok.op.assign | expected("=")]
				> expr
				> omit[tok.semicolon | expected(";")]
			)[fwd >= [this] (locd<Type>* dt, Identifier* id, ptr<Expr>& e) {
				return new DeclarationStmt(std::move(dt->loc), dt->val, std::move(*id), e);
			}];

		s.goto_.name("goto statement");
		s.goto_ =
			(
				tok.word.goto_
				> identifier
				> omit[tok.semicolon | expected(";")]
			)[fwd >= [this] (range& r, Identifier* to) {
				return new GotoStmt(locOf(r), std::move(*to));
			}];

		auto onSimple = [this] (OnSimpleTrigger trigger) {
			return [this, trigger] (range& r, ptr<BlockStmt>& block) {
				return new OnSimpleBlock(locOf(r), trigger, block);
			};
		};
		on_block =
			(tok.word.on >> omit[tok.word.entry] > s.block)[fwd >= onSimple(OnSimpleTrigger::Entry)]
			| (tok.word.on >> omit[tok.word.exit] > s.block)[fwd >= onSimple(OnSimpleTrigger::Exit)]
			| (tok.word.on >> omit[tok.word.periodic] > s.block)[fwd >= onSimple(OnSimpleTrigger::Periodic)]
			| (
				tok.word.on
				>> omit[
					tok.word.packet
					> tok.word.from
				]
				> identifier
				> s.block
			)[fwd >= [this] (range& r, Identifier* from, ptr<BlockStmt>& block) {
				return new OnPacketBlock(locOf(r), *from, block);
			}]
			| (
				tok.word.on
				>> omit[tok.lparen]
				> expr
				> omit[tok.rparen | expected(")")]
				> s.block
			)[fwd >= [this] (range& r, ptr<Expr>& e, ptr<BlockStmt>& block) {
				return new OnExprBlock(locOf(r), e, block);
			}]
			| (tok.word.on >> expected("on event specifier"));

		state =
			(
				tok.word.state
				> identifier
				> omit[tok.lbrace | expected("{")]
				> *s.decl
				> *on_block
				> -(omit[tok.word.always] > s.block)
				> omit[tok.rbrace | expected("}")]
			)[fwd >= [this] (range& r, Identifier* id, std::vector<ptr<DeclarationStmt>>& decls,
					std::vector<ptr<OnBlock>>& onBlocks, ptr<BlockStmt>* always) {
				return new State(locOf(r), std::move(*id), unpack(decls), move(onBlocks), always ? *always : nullptr);
			}];

		classParam.name("class parameter");
		classParam =
			(tok.word.device > tok.ident)[fwd >= [this] (range& r, range& id) {
				return new CPDevice(locOf(r), str(id));
			}]
			| (tok.word.endpoint > tok.ident)[fwd >= [this] (range& r, range& id) {
				return new CPEndpoint(locOf(r), str(id));
			}]
			| (datatype > tok.ident)[fwd >= [this] (locd<Type>* t, range& id) {
				return new CPValue(t->loc, str(id), t->val);
			}];

		machine_class =
			(
				tok.word.class_
				> identifier
				> omit[tok.lparen | expected("(")]
				> -(classParam % tok.comma)
				> omit[tok.rparen | expected(")")]
				> omit[tok.lbrace | expected("{")]
				> *s.decl
				> *state
				> omit[
					(tok.rbrace | expected("}"))
					>> (tok.semicolon | expected(";"))
				]
			)[fwd >= [this] (range& r, Identifier* id, std::vector<ptr<ClassParameter>>* params,
					std::vector<ptr<DeclarationStmt>>& decls, std::vector<ptr<State>>& states) {
				if (params)
					return new MachineClass(locOf(r), std::move(*id), move(*params), move(decls), unpack(states));
				else
					return new MachineClass(locOf(r), std::move(*id), {}, move(decls), unpack(states));
			}];

		machine_spec =
			(
				tok.word.machine
				>> identifier
				>> !tok.lbrace
				>> omit[tok.colon | expected(":")]
				> identifier
				> omit[tok.lparen | expected("(")]
				> -(expr % tok.comma)
				> omit[
					(tok.rparen | expected(")"))
					>> (tok.semicolon | expected(";"))
				]
			)[fwd >= [this] (range& r, Identifier* name, Identifier* class_, std::vector<ptr<Expr>>* args) {
				return new MachineInstantiation(locOf(r), std::move(*name), *class_, move(args));
			}]
			| (
				tok.word.machine
				> identifier
				> omit[tok.lbrace | expected("{")]
				> *s.decl
				> *state
				> omit[
					(tok.rbrace | expected("}"))
					>> (tok.semicolon | expected(";"))
				]
			)[fwd >= [this] (range& r, Identifier* id, std::vector<ptr<DeclarationStmt>>& decls,
					std::vector<ptr<State>>& states) {
				return new MachineDefinition(locOf(r), std::move(*id), move(decls), unpack(states));
			}];

		endpoint =
			(
				tok.word.endpoint
				> identifier
				> omit[tok.lparen | expected("(")]
				> &omit[tok.lit.uint_ | expected("endpoint id")]
				> tok.lit.uint_
				> omit[
					(tok.rparen | expected(")"))
					> (tok.colon | expected(":"))
				]
				> datatype
				> omit[tok.lparen | expected("(")]
				> endpoint_access
				> omit[
					(tok.rparen | expected("endpoint access specification"))
					> (tok.semicolon | expected(";"))
				]
			)[fwd >= [this] (range& r, Identifier* name, range& eid, locd<Type>* dt, EndpointAccess access, bool& pass) {
				uint32_t eidVal;
				pass = qi::parse(eid.begin(), eid.end(), qi::uint_parser<uint32_t, 10, 1, 99>(), eidVal);
				return new Endpoint(locOf(r), *name, eidVal, dt->val, access);
			}];

		device =
			(
				tok.word.device
				> identifier
				> omit[tok.lparen | expected("(")]
				> ip_addr
				> omit[
					(tok.rparen | expected(")"))
					 > (tok.colon | expected(":"))
				]
				> -(identifier % omit[tok.comma])
				> omit[tok.semicolon | expected(";")]
			)[fwd >= [this] (range& r, Identifier* name, std::array<uint8_t, 16>& ip, std::vector<opt<Identifier>>* eps) {
				return new Device(locOf(r), *name, ip, unpack(eps));
			}];

		include =
			(
				tok.word.include
				> &omit[tok.string | expected("include file path")]
				> tok.string
				> omit[tok.semicolon | expected(";")]
			)[fwd >= [this] (range& r, range& file) {
				return new IncludeLine(locOf(r), std::string(file.begin().base() + 1, file.end().base() - 1));
			}];

		ip_addr.name("IP address");
		ip_addr =
			lexeme[+(tok.colon | tok.dot | tok.lit.uint_ | tok.ident)][fwd >=
				[this] (std::vector<range>& addr, bool& pass) {
					std::array<uint8_t, 16> result;
					std::string str(addr.front().begin(), addr.back().end());
					try {
						auto parsed = boost::asio::ip::address_v6::from_string(str);
						auto bytes = parsed.to_bytes();
						std::copy(bytes.begin(), bytes.end(), result.begin());
					} catch (...) {
						badIP = str;
						pass = false;
					}
					return result;
				}];

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

		auto dt = [this] (Type t) {
			return [this, t] (range& r) {
				return locd<Type>{locOf(r), t};
			};
		};
		datatype.name("datatype");
		datatype =
			tok.type.bool_[fwd >= dt(Type::Bool)]
			| tok.type.uint8_[fwd >= dt(Type::UInt8)]
			| tok.type.uint16_[fwd >= dt(Type::UInt16)]
			| tok.type.uint32_[fwd >= dt(Type::UInt32)]
			| tok.type.uint64_[fwd >= dt(Type::UInt64)]
			| tok.type.int8_[fwd >= dt(Type::Int8)]
			| tok.type.int16_[fwd >= dt(Type::Int16)]
			| tok.type.int32_[fwd >= dt(Type::Int32)]
			| tok.type.int64_[fwd >= dt(Type::Int64)]
			| tok.type.float_[fwd >= dt(Type::Float)];

		identifier.name("identifier");
		identifier =
			tok.ident[fwd >= [this] (range& id) {
				return Identifier(locOf(id), str(id));
			}];

#undef expected
	}

	const std::string* file;
	const SourceLocation* includedFrom;

	std::string badIP;

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
		rule<ptr<Stmt>()> assign, if_, switch_;
		rule<ptr<BlockStmt>()> block;
		rule<ptr<DeclarationStmt>()> decl;
		rule<ptr<Stmt>()> goto_;

		rule<ptr<std::vector<SwitchEntry>>()> switch_body;
		rule<std::vector<ptr<SwitchLabel>>()> switch_labels;
	} s;

	rule<ptr<OnBlock>()> on_block;

	rule<ptr<State>()> state;

	rule<ptr<ProgramPart>()> machine_class;
	rule<ptr<ProgramPart>()> machine_spec;
	rule<ptr<ProgramPart>()> endpoint;
	rule<ptr<ProgramPart>()> device;
	rule<ptr<ProgramPart>()> include;

	rule<std::array<uint8_t, 16>()> ip_addr;
	rule<EndpointAccess()> endpoint_access;
	rule<opt<locd<Type>>()> datatype;
	rule<opt<Identifier>()> identifier;
	rule<ptr<ClassParameter>()> classParam;

	rule<> unexpected;

	int tabWidth;

	SourceLocation locOf(const range& range)
	{
		return SourceLocation(file, getLine(range.begin()), getColumn(range.begin(), tabWidth), includedFrom);
	}
};

}



static SourceLocation* cloneSloc(const SourceLocation* sloc, std::vector<std::unique_ptr<std::string>>& strings,
		std::vector<std::unique_ptr<SourceLocation>>& locs)
{
	if (sloc) {
		strings.emplace_back(new std::string(sloc->file()));
		locs.emplace_back(
			new SourceLocation(
				strings.back().get(),
				sloc->line(),
				sloc->col(),
				cloneSloc(sloc->parent(), strings, locs)));
		return locs.back().get();
	} else {
		return nullptr;
	}
}

ParseError::ParseError(const SourceLocation& at, const std::string& expected, const std::string& got)
	: runtime_error(expected), _at(cloneSloc(&at, _strings, _locs)), _expected(expected), _got(got)
{
}

static std::list<std::unique_ptr<ProgramPart>> parseBuffer(const util::MemoryBuffer& input,
		const std::string* fileName, const SourceLocation* includedFrom, int tabWidth,
		std::vector<std::pair<unsigned, std::string>>* expectations)
{
	typedef sloc_iterator<const char*> iter;
	typedef boost::spirit::lex::lexertl::token<iter> token_type;
	typedef boost::spirit::lex::lexertl::lexer<token_type> lexer_type;
	typedef typename tokenizer<lexer_type>::iterator_type iterator_type;

	auto ctext = input.crange<char>();

	std::list<std::unique_ptr<ProgramPart>> parts;

	tokenizer<lexer_type> t;
	grammar<iterator_type> g(t, fileName, includedFrom, tabWidth);
	whitespace<iterator_type> ws(t, expectations);

	try {
		iter first(ctext.begin()), last(ctext.end());
		bool result = boost::spirit::lex::tokenize_and_phrase_parse(first, last, t, g, ws, parts);

		if (!result || first != last)
			throw ParseError(SourceLocation(fileName, 0, 0, includedFrom), "...not this anyway...", "<internal error>");
	} catch (const qi::expectation_failure<iterator_type>& ef) {
		std::string badToken;
		int line = 0, col = 0;
		const std::string& expected = ef.what_.tag;

		if (ef.first->is_valid()) {
			iter errit = ef.first->value().begin();

			if (expected == g.ip_addr.name())
				badToken = g.badIP;
			else
				boost::spirit::lex::tokenize(errit, iter(ctext.end()), t,
					[&badToken, &line, &col, tabWidth] (const token_type& token) {
						if (token.id() == TOKEN_SPACE || token.id() == TOKEN_LINECOMMENT)
							return true;

						if (token.id() == TOKEN_ANY && !std::isprint(*token.value().begin())) {
							unsigned val = (unsigned char) *token.value().begin();
							char buf[16];
							sprintf(buf, "\\x%02x", val);
							badToken = buf;
						} else {
							badToken = '\'' + std::string(token.value().begin(), token.value().end()) + '\'';
						}
						line = getLine(token.value().begin());
						col = getColumn(token.value().begin(), tabWidth);
						return false;
					});
		} else {
			for (auto it = iter(ctext.begin()), end = iter(ctext.end()); it != end; ++it) {
				line = getLine(it);
				col = getColumn(it, tabWidth);
			}
			badToken = "<EOF>";
		}

		throw ParseError(SourceLocation(fileName, line, col, includedFrom), expected, badToken);
	}

	return parts;
}



Parser::Parser(std::vector<std::string> includePaths, int tabWidth, std::vector<std::pair<unsigned, std::string>>* expectations)
	: _includePaths(std::move(includePaths)), _tabWidth(tabWidth), _expectations(expectations)
{
	for (const auto& path : _includePaths)
		if (!boost::filesystem::path(path).is_absolute())
			throw std::invalid_argument("includePaths");
}

Parser::FileData Parser::loadFile(const std::string& file, const std::string* extraSearchDir)
{
	std::unique_ptr<util::MemoryBuffer> buf;

	if (file.find("./") != 0) {
		for (auto& includeDir : _includePaths) {
			std::string beneathIncludeDir = (boost::filesystem::path(includeDir) / file).native();
			if ((buf = util::MemoryBuffer::tryLoadFile(beneathIncludeDir)))
				return { std::move(*buf), beneathIncludeDir };
		}
	}

	if (extraSearchDir) {
		std::string beneathExtraDir = (boost::filesystem::path(*extraSearchDir) / file).native();
		if ((buf = util::MemoryBuffer::tryLoadFile(beneathExtraDir)))
			return { std::move(*buf), beneathExtraDir };
	}

	return {};
}

std::list<std::unique_ptr<ProgramPart>> Parser::parseFile(const FileData& fileData, const std::string* pathPtr,
		const SourceLocation* includedFrom)
{
	auto parsed = parseBuffer(fileData.buf, pathPtr, includedFrom, _tabWidth, _expectations);
	for (auto it = parsed.begin(), end = parsed.end(); it != end; ++it) {
		auto* inc = dynamic_cast<IncludeLine*>(it->get());
		if (!inc)
			continue;

		++it;
		parsed.splice(it, parseRecursive(*inc, &boost::filesystem::path(fileData.fullPath).parent_path().native()));
	}

	return parsed;
}

std::list<std::unique_ptr<ProgramPart>> Parser::parseRecursive(IncludeLine& include, const std::string* extraSearchDir)
{
	auto fileData = loadFile(include.file(), extraSearchDir);

	if (!fileData.fullPath.size())
		throw ParseError(include.sloc(), "included file '" + include.file() + "' to exist", "nothing");

	if (_currentIncludeStack.count(fileData.fullPath))
		throw ParseError(include.sloc(), "included file '" + include.file() + "' to be nonrecursive", "just that");

	if (_filesAlreadyIncluded.count(fileData.fullPath))
		return {};

	_currentIncludeStack.insert(fileData.fullPath);
	include.fullPath(fileData.fullPath);

	auto parsed = parseFile(fileData, &include.fullPath(), &include.sloc());

	_filesAlreadyIncluded.insert(fileData.fullPath);
	_currentIncludeStack.erase(fileData.fullPath);

	return parsed;
}

std::unique_ptr<TranslationUnit> Parser::parse(const std::string& fileName)
{
	if (!boost::filesystem::path(fileName).is_absolute())
		throw std::invalid_argument("fileName");

	_filesAlreadyIncluded.clear();
	_currentIncludeStack.clear();
	_currentIncludeStack.insert(fileName);

	std::unique_ptr<std::string> filePtr(new std::string(fileName));

	auto parsed = parseFile({ util::MemoryBuffer::loadFile(fileName), fileName }, filePtr.get());

	return std::unique_ptr<TranslationUnit>(
		new TranslationUnit(
			std::move(filePtr),
			std::vector<std::unique_ptr<ProgramPart>>(
				std::make_move_iterator(parsed.begin()),
				std::make_move_iterator(parsed.end()))));
}

}
}
