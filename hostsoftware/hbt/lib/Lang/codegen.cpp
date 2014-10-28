#include "Lang/codegen.hpp"

#include <map>
#include <set>
#include <tuple>
#include <vector>

#include "MC/builder.hpp"
#include "MC/program.hpp"
#include "Lang/ast.hpp"

namespace hbt {
namespace lang {

namespace {

struct CodegenCantDo : public std::runtime_error {
	using runtime_error::runtime_error;
};

[[gnu::noreturn]]
void die(const std::string& msg)
{
	throw CodegenCantDo("codegen can't do " + msg);
}



struct CodegenBlock {
	mc::Label entryLabel;
	std::vector<std::tuple<boost::optional<mc::Label>, mc::Opcode, boost::optional<mc::Builder::immediate_t>>> instructions;

	boost::optional<mc::Label> nextLabel;

	CodegenBlock(mc::Label label)
		: entryLabel(label), nextLabel(label)
	{}

	void append(mc::Opcode op)
	{
		instructions.emplace_back(nextLabel, op, boost::none_t());
		nextLabel.reset();
	}

	void append(mc::Opcode op, mc::Builder::immediate_t immed)
	{
		instructions.emplace_back(nextLabel, op, std::move(immed));
		nextLabel.reset();
	}

	void append(CodegenBlock&& block)
	{
		if (!instructions.size())
			die("blocks with multiple entry labels");

		instructions.reserve(instructions.size() + block.instructions.size());
		std::copy(
			std::make_move_iterator(block.instructions.begin()),
			std::make_move_iterator(block.instructions.end()),
			std::back_inserter(instructions));
	}

	void emit(mc::Builder& into)
	{
		for (auto& insn : instructions) {
			if (std::get<2>(insn)) {
				into.insert(std::get<0>(insn), std::get<1>(insn), std::move(*std::get<2>(insn)), 0);
			} else {
				into.insert(std::get<0>(insn), std::get<1>(insn), 0);
			}
		}
	}
};



class CodegenVisitor : public ASTVisitor {
private:
	Device* currentDevice;

	mc::Builder builder;

	std::vector<CodegenBlock*> blocks;

	CodegenBlock& current()
	{
		return *blocks.back();
	}

	std::map<std::string, unsigned> variableAddresses;

	struct Scope {
		std::map<std::string, unsigned> variables;
	};

	std::vector<Scope> scopes;
	unsigned temporarySlotsUsed;

	unsigned slotFor(const std::string& var)
	{
		unsigned baseSlot = 0;
		for (auto& scope : scopes) {
			auto it = scope.variables.find(var);
			if (it != scope.variables.end())
				return baseSlot + it->second;

			baseSlot += scope.variables.size();
		}

		die("unknown vars");
	}

	unsigned topSlot()
	{
		unsigned result = 0;
		for (auto& scope : scopes)
			result += scope.variables.size();

		return result + temporarySlotsUsed;
	}

	unsigned distanceTo(const std::string& var)
	{
		unsigned res = topSlot() - slotFor(var);
		if (res >= 256)
			die("deep stacks");
		return res;
	}

	void declareVariable(const std::string& var)
	{
		for (auto& scope : scopes)
			if (scope.variables.count(var))
				die("duplicate variable names");

		scopes.back().variables.insert({ var, scopes.back().variables.size() });
	}

	Device* currentPacketSource;

public:
	CodegenVisitor()
		: currentDevice(nullptr), builder(0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}), temporarySlotsUsed(0)
	{}

	virtual void visit(IdentifierExpr&) override;
	virtual void visit(TypedLiteral<bool>&) override;
	virtual void visit(TypedLiteral<uint8_t>&) override;
	virtual void visit(TypedLiteral<uint16_t>&) override;
	virtual void visit(TypedLiteral<uint32_t>&) override;
	virtual void visit(TypedLiteral<uint64_t>&) override;
	virtual void visit(TypedLiteral<int8_t>&) override;
	virtual void visit(TypedLiteral<int16_t>&) override;
	virtual void visit(TypedLiteral<int32_t>&) override;
	virtual void visit(TypedLiteral<int64_t>&) override;
	virtual void visit(TypedLiteral<float>&) override;
	virtual void visit(CastExpr&) override;
	virtual void visit(UnaryExpr&) override;
	virtual void visit(BinaryExpr&) override;
	virtual void visit(ConditionalExpr&) override;
	virtual void visit(EndpointExpr&) override;
	virtual void visit(CallExpr&) override;

	virtual void visit(AssignStmt&) override;
	virtual void visit(WriteStmt&) override;
	virtual void visit(IfStmt&) override;
	virtual void visit(SwitchStmt&) override;
	virtual void visit(BlockStmt&) override;
	virtual void visit(DeclarationStmt&) override;
	virtual void visit(GotoStmt&) override;

	virtual void visit(OnSimpleBlock&) override;
	virtual void visit(OnExprBlock&) override;
	virtual void visit(OnUpdateBlock&) override;

	virtual void visit(Endpoint&) override;
	virtual void visit(Device&) override;
	virtual void visit(MachineClass&) override;
	virtual void visit(MachineDefinition&) override;
	virtual void visit(MachineInstantiation&) override;
	virtual void visit(IncludeLine&) override;
	virtual void visit(TranslationUnit&) override;

	std::unique_ptr<mc::Program> finish()
	{
		return builder.finish();
	}
};

static mc::MemType memtypeFor(Type t)
{
	switch (t) {
	case Type::Bool: return mc::MemType::Bool;
	case Type::UInt8: return mc::MemType::U8;
	case Type::UInt16: return mc::MemType::U16;
	case Type::UInt32: return mc::MemType::U32;
	case Type::UInt64: return mc::MemType::U64;
	case Type::Int8: return mc::MemType::S8;
	case Type::Int16: return mc::MemType::S16;
	case Type::Int32: return mc::MemType::S32;
	case Type::Int64: return mc::MemType::S64;
	case Type::Float: return mc::MemType::Float;
	}
	die("unknown types");
}

void CodegenVisitor::visit(IdentifierExpr& e)
{
	if (variableAddresses.count(e.name()))
		current().append(mc::Opcode::LD_MEM, std::make_tuple(memtypeFor(e.type()), variableAddresses[e.name()]));
	else
		current().append(mc::Opcode::DUP_I, uint8_t(distanceTo(e.name()) - 1));
}

void CodegenVisitor::visit(TypedLiteral<bool>& l)
{
	current().append(l.value() ? mc::Opcode::LD_TRUE : mc::Opcode::LD_FALSE);
}

void CodegenVisitor::visit(TypedLiteral<uint8_t>& l) { current().append(mc::Opcode::LD_U8, l.value()); }
void CodegenVisitor::visit(TypedLiteral<uint16_t>& l) { current().append(mc::Opcode::LD_U16, l.value()); }
void CodegenVisitor::visit(TypedLiteral<uint32_t>& l) { current().append(mc::Opcode::LD_U32, l.value()); }
void CodegenVisitor::visit(TypedLiteral<uint64_t>& l) { current().append(mc::Opcode::LD_U64, l.value()); }
void CodegenVisitor::visit(TypedLiteral<int8_t>& l) { current().append(mc::Opcode::LD_S8, l.value()); }
void CodegenVisitor::visit(TypedLiteral<int16_t>& l) { current().append(mc::Opcode::LD_S16, l.value()); }
void CodegenVisitor::visit(TypedLiteral<int32_t>& l) { current().append(mc::Opcode::LD_S32, l.value()); }
void CodegenVisitor::visit(TypedLiteral<int64_t>& l) { current().append(mc::Opcode::LD_S64, l.value()); }
void CodegenVisitor::visit(TypedLiteral<float>& l) { current().append(mc::Opcode::LD_FLOAT, l.value()); }

void CodegenVisitor::visit(CastExpr& c)
{
	c.expr().accept(*this);
	switch (c.type()) {
	case Type::Bool:   current().append(mc::Opcode::CONV_B); return;
	case Type::UInt8:  current().append(mc::Opcode::CONV_U8); return;
	case Type::UInt16: current().append(mc::Opcode::CONV_U16); return;
	case Type::UInt32: current().append(mc::Opcode::CONV_U32); return;
	case Type::UInt64: current().append(mc::Opcode::CONV_U64); return;
	case Type::Int8:   current().append(mc::Opcode::CONV_S8); return;
	case Type::Int16:  current().append(mc::Opcode::CONV_S16); return;
	case Type::Int32:  current().append(mc::Opcode::CONV_S32); return;
	case Type::Int64:  current().append(mc::Opcode::CONV_S64); return;
	case Type::Float:  current().append(mc::Opcode::CONV_F); return;
	}
	die("unknown types");
}

void CodegenVisitor::visit(UnaryExpr& e)
{
	e.expr().accept(*this);
	switch (e.op()) {
	case UnaryOperator::Plus:
		break;

	case UnaryOperator::Minus:
		current().append(mc::Opcode::LD_FALSE);
		current().append(mc::Opcode::ROT);
		current().append(mc::Opcode::SUB);
		break;

	case UnaryOperator::Not:
	case UnaryOperator::Negate:
		if (e.expr().type() == Type::Bool) {
			current().append(mc::Opcode::LD_TRUE);
			current().append(mc::Opcode::XOR);
		} else {
			current().append(mc::Opcode::LD_FALSE);
			current().append(mc::Opcode::LD_TRUE);
			current().append(mc::Opcode::SUB);
			if (e.expr().type() == Type::UInt64 || e.expr().type() == Type::Int64)
				current().append(mc::Opcode::CONV_U64);
			current().append(mc::Opcode::XOR);
		}
		break;
	}
}

void CodegenVisitor::visit(BinaryExpr& e)
{
	bool isBoolShortcut = e.op() == BinaryOperator::BoolAnd || e.op() == BinaryOperator::BoolOr;

	e.left().accept(*this);
	if (isBoolShortcut && e.left().type() != Type::Bool)
		current().append(mc::Opcode::CONV_B);

	temporarySlotsUsed++;
	e.right().accept(*this);
	if (isBoolShortcut && e.right().type() != Type::Bool)
		current().append(mc::Opcode::CONV_B);
	temporarySlotsUsed--;

	switch (e.op()) {
	case BinaryOperator::Plus: current().append(mc::Opcode::ADD); break;
	case BinaryOperator::Minus: current().append(mc::Opcode::SUB); break;
	case BinaryOperator::Multiply: current().append(mc::Opcode::MUL); break;
	case BinaryOperator::Divide: current().append(mc::Opcode::DIV); break;
	case BinaryOperator::Modulo: current().append(mc::Opcode::MOD); break;

	case BinaryOperator::BoolAnd:
	case BinaryOperator::And:
		current().append(mc::Opcode::AND);
		break;

	case BinaryOperator::BoolOr:
	case BinaryOperator::Or:
		current().append(mc::Opcode::OR);
		break;

	case BinaryOperator::Xor: current().append(mc::Opcode::XOR); break;
	case BinaryOperator::Equals: current().append(mc::Opcode::CMP_EQ); break;
	case BinaryOperator::NotEquals: current().append(mc::Opcode::CMP_NEQ); break;
	case BinaryOperator::LessThan: current().append(mc::Opcode::CMP_LT); break;
	case BinaryOperator::LessOrEqual: current().append(mc::Opcode::CMP_LE); break;
	case BinaryOperator::GreaterThan: current().append(mc::Opcode::CMP_GT); break;
	case BinaryOperator::GreaterOrEqual: current().append(mc::Opcode::CMP_GE); break;
	case BinaryOperator::ShiftLeft: current().append(mc::Opcode::SHL); break;
	case BinaryOperator::ShiftRight: current().append(mc::Opcode::SHR); break;
	}
}

void CodegenVisitor::visit(ConditionalExpr& c)
{
	c.condition().accept(*this);

	CodegenBlock ifTrue(builder.createLabel());
	CodegenBlock ifFalse(builder.createLabel());
	mc::Label exit(builder.createLabel());

	blocks.push_back(&ifTrue);
	c.ifTrue().accept(*this);
	blocks.pop_back();

	blocks.push_back(&ifFalse);
	c.ifFalse().accept(*this);
	blocks.pop_back();

	current().append(mc::Opcode::JZ, ifFalse.entryLabel);
	ifTrue.append(mc::Opcode::JUMP, exit);
	current().append(std::move(ifTrue));
	current().append(std::move(ifFalse));

	current().nextLabel = exit;
	current().append(mc::Opcode::LD_FALSE);
	current().append(mc::Opcode::POP);
}

void CodegenVisitor::visit(EndpointExpr&)
{
	die("packet value access by endpoints");
}

void CodegenVisitor::visit(CallExpr& c)
{
	for (auto& arg : c.arguments()) {
		arg->accept(*this);
		temporarySlotsUsed++;
	}

	if (c.name().name() == "second")
		current().append(mc::Opcode::DT_DECOMPOSE, mc::DTMask::second);
	else if (c.name().name() == "minute")
		current().append(mc::Opcode::DT_DECOMPOSE, mc::DTMask::minute);
	else if (c.name().name() == "hour")
		current().append(mc::Opcode::DT_DECOMPOSE, mc::DTMask::hour);
	else if (c.name().name() == "day")
		current().append(mc::Opcode::DT_DECOMPOSE, mc::DTMask::day);
	else if (c.name().name() == "month")
		current().append(mc::Opcode::DT_DECOMPOSE, mc::DTMask::month);
	else if (c.name().name() == "year")
		current().append(mc::Opcode::DT_DECOMPOSE, mc::DTMask::year);
	else if (c.name().name() == "weekday")
		current().append(mc::Opcode::DT_DECOMPOSE, mc::DTMask::weekday);
	else if (c.name().name() == "now")
		current().append(mc::Opcode::LD_SYSTIME);
	else
		die("unknown functions");

	temporarySlotsUsed -= c.arguments().size();
}

void CodegenVisitor::visit(AssignStmt& s)
{
	s.value().accept(*this);
	if (variableAddresses.count(s.target().name()))
		current().append(
			mc::Opcode::ST_MEM,
			std::make_tuple(memtypeFor(s.targetDecl()->type()), variableAddresses[s.target().name()]));
	else
		current().append(mc::Opcode::EXCHANGE, uint8_t(distanceTo(s.target().name()) - 1));
}

void CodegenVisitor::visit(WriteStmt& w)
{
	if (currentDevice && currentDevice != w.target().device())
		die("multiple devices written by a single machine");

	currentDevice = static_cast<Device*>(w.target().device());
	current().append(mc::Opcode::LD_U32, static_cast<Endpoint*>(w.target().endpoint())->eid());
	temporarySlotsUsed++;
	w.value().accept(*this);
	temporarySlotsUsed--;
	current().append(mc::Opcode::WRITE);
}

void CodegenVisitor::visit(IfStmt& i)
{
	i.condition().accept(*this);

	CodegenBlock ifTrue(builder.createLabel());
	CodegenBlock ifFalse(builder.createLabel());
	mc::Label exit(builder.createLabel());

	blocks.push_back(&ifTrue);
	i.ifTrue().accept(*this);
	blocks.pop_back();

	if (i.ifFalse()) {
		blocks.push_back(&ifFalse);
		i.ifFalse()->accept(*this);
		blocks.pop_back();
	}

	if (ifTrue.instructions.size()) {
		if (ifFalse.instructions.size()) {
			current().append(mc::Opcode::JZ, ifFalse.entryLabel);
			ifTrue.append(mc::Opcode::JUMP, exit);
			current().append(std::move(ifTrue));
			current().append(std::move(ifFalse));
		} else {
			current().append(mc::Opcode::JZ, exit);
			current().append(std::move(ifTrue));
		}
	} else if (ifFalse.instructions.size()) {
		current().append(mc::Opcode::JZ, exit);
		current().append(std::move(ifFalse));
	} else {
		current().append(mc::Opcode::POP);
	}
	current().nextLabel = exit;
	current().append(mc::Opcode::LD_FALSE);
	current().append(mc::Opcode::POP);
}

void CodegenVisitor::visit(SwitchStmt& s)
{
	std::vector<mc::SwitchEntry> labels;
	std::map<uint32_t, CodegenBlock*> blocks;
	CodegenBlock* defaultBlock = nullptr;
	std::vector<std::unique_ptr<CodegenBlock>> blockPtrs;
	mc::Label exit(builder.createLabel());

	for (auto& e : s.entries()) {
		blockPtrs.emplace_back(new CodegenBlock(builder.createLabel()));

		for (auto& l : e.labels()) {
			if (!l.expr()) {
				defaultBlock = blockPtrs.back().get();
				continue;
			}

			if (l.expr()->constexprValue() < 0)
				die("negative switch labels");
			if (l.expr()->constexprValue() > 0xffffffff)
				die("large switch labels");

			labels.push_back({ cl_I_to_UL(l.expr()->constexprValue()), blockPtrs.back()->entryLabel });
			blocks[labels.back().label] = blockPtrs.back().get();
		}
		this->blocks.push_back(blockPtrs.back().get());
		e.statement().accept(*this);
		current().append(mc::Opcode::JUMP, exit);
		this->blocks.pop_back();
	}

	if (blocks.size() > 255)
		die("large switch blocks");

	s.expr().accept(*this);
	current().append(mc::Opcode::SWITCH_32, mc::SwitchTable(labels.begin(), labels.end()));
	if (defaultBlock)
		current().append(mc::Opcode::JUMP, defaultBlock->entryLabel);
	current().append(mc::Opcode::JUMP, exit);

	for (auto& block : blockPtrs)
		current().append(std::move(*block));

	current().nextLabel = exit;
	current().append(mc::Opcode::LD_FALSE);
	current().append(mc::Opcode::POP);
}

void CodegenVisitor::visit(BlockStmt& b)
{
	scopes.push_back({});

	for (auto& s : b.statements())
		s->accept(*this);

	for (auto& e : scopes.back().variables)
		current().append(mc::Opcode::POP);

	scopes.pop_back();
}

void CodegenVisitor::visit(DeclarationStmt& d)
{
	d.value().accept(*this);
	declareVariable(d.name().name());
}

void CodegenVisitor::visit(GotoStmt&)
{
	current().append(mc::Opcode::RET);
}

void CodegenVisitor::visit(OnSimpleBlock& o)
{
	if (o.trigger() != OnSimpleTrigger::Entry && o.trigger() != OnSimpleTrigger::Periodic)
		die("simple on blocks that aren't 'entry' or 'periodic'");

	o.block().accept(*this);
}

void CodegenVisitor::visit(OnExprBlock&) {die("expression-triggered on blocks");}
void CodegenVisitor::visit(OnUpdateBlock&) {die("value-triggered on blocks");}
void CodegenVisitor::visit(MachineInstantiation&) { die("machine instantiations"); }

// nothing to do here
void CodegenVisitor::visit(Endpoint& e) {}
void CodegenVisitor::visit(Device& d) {}
void CodegenVisitor::visit(MachineClass&) {}
void CodegenVisitor::visit(IncludeLine&) {}

void CodegenVisitor::visit(MachineDefinition& m)
{
	if (m.states().size() != 1)
		die("machines with not exactly one state");

	scopes.push_back({});

	unsigned totalVarSize = 0;
	for (auto& var : m.variables()) {
		variableAddresses.insert({ var->name().name(), totalVarSize });
		totalVarSize += sizeOf(var->type());
	}

	std::map<OnSimpleTrigger, std::unique_ptr<CodegenBlock>> simpleBlocks;
	CodegenBlock packetSwitchBlock(builder.createLabel());
	std::map<const Device*, std::unique_ptr<CodegenBlock>> packetBlocks;
	CodegenBlock stmtBlock(builder.createLabel());

	for (auto& on : m.states()[0].onBlocks()) {
		if (auto* simple = dynamic_cast<OnSimpleBlock*>(on.get())) {
			if (simpleBlocks.count(simple->trigger()))
				die("multiple simple on blocks of same type");

			simpleBlocks.emplace(simple->trigger(), std::unique_ptr<CodegenBlock>(new CodegenBlock(builder.createLabel())));
			CodegenBlock* block = simpleBlocks[simple->trigger()].get();

			blocks.push_back(block);
			currentPacketSource = nullptr;
			on->accept(*this);
			blocks.pop_back();
		}
	}

	if (packetSwitchBlock.instructions.size()) {
		packetSwitchBlock.append(mc::Opcode::RET);
		builder.onPacket(packetSwitchBlock.entryLabel);
	}

	if (m.states()[0].always()) {
		currentPacketSource = nullptr;
		blocks.push_back(&stmtBlock);

		m.states()[0].always()->accept(*this);

		blocks.pop_back();
	}

	if (stmtBlock.instructions.size()) {
		for (auto& block : simpleBlocks)
			block.second->append(mc::Opcode::JUMP, stmtBlock.entryLabel);
		for (auto& block : packetBlocks)
			block.second->append(mc::Opcode::JUMP, stmtBlock.entryLabel);
	}

	for (auto& block : simpleBlocks) {
		switch (block.first) {
		case OnSimpleTrigger::Entry: builder.onInit(block.second->entryLabel); break;
		case OnSimpleTrigger::Periodic: builder.onPeriodic(block.second->entryLabel); break;
		default: break;
		}
		block.second->emit(builder);
	}

	packetSwitchBlock.emit(builder);
	for (auto& block : packetBlocks)
		block.second->emit(builder);

	stmtBlock.emit(builder);

	scopes.pop_back();
}

void CodegenVisitor::visit(TranslationUnit& tu)
{
	for (auto& p : tu.items())
		p->accept(*this);
}

}

std::unique_ptr<mc::Program> generateMachineCodeFor(TranslationUnit& tu)
{
	CodegenVisitor cv;

	tu.accept(cv);
	return cv.finish();
}

}
}
