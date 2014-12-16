#include "hbt/IR/codegen.hpp"

#include <list>
#include <stdexcept>

#include "hbt/IR/basicblock.hpp"
#include "hbt/IR/module.hpp"
#include "hbt/MC/builder.hpp"
#include "hbt/Util/fatal.hpp"
#include "hbt/Util/toposort.hpp"

namespace hbt {
namespace ir {

namespace {

struct CodegenCantDo : public std::runtime_error {
	CodegenCantDo(const std::string& msg)
		: runtime_error("codegen can't do " + msg)
	{}
};

static mc::MemType memtypeForType(Type t)
{
	switch (t) {
	case Type::Bool:   return mc::MemType::Bool;
	case Type::UInt8:  return mc::MemType::U8;
	case Type::UInt16: return mc::MemType::U16;
	case Type::UInt32: return mc::MemType::U32;
	case Type::UInt64: return mc::MemType::U64;
	case Type::Int8:   return mc::MemType::S8;
	case Type::Int16:  return mc::MemType::S16;
	case Type::Int32:  return mc::MemType::S32;
	case Type::Int64:  return mc::MemType::S64;
	case Type::Float:  return mc::MemType::Float;
	}

	hbt_unreachable();
}

static mc::Opcode arithOpToOpcode(ArithOp op)
{
	switch (op) {
	case ArithOp::Mul: return mc::Opcode::MUL;
	case ArithOp::Div: return mc::Opcode::DIV;
	case ArithOp::Mod: return mc::Opcode::MOD;
	case ArithOp::Add: return mc::Opcode::ADD;
	case ArithOp::Sub: return mc::Opcode::SUB;
	case ArithOp::And: return mc::Opcode::AND;
	case ArithOp::Or:  return mc::Opcode::OR;
	case ArithOp::Xor: return mc::Opcode::XOR;
	case ArithOp::Shl: return mc::Opcode::SHL;
	case ArithOp::Shr: return mc::Opcode::SHR;
	case ArithOp::Lt:  return mc::Opcode::CMP_LT;
	case ArithOp::Le:  return mc::Opcode::CMP_LE;
	case ArithOp::Gt:  return mc::Opcode::CMP_GT;
	case ArithOp::Ge:  return mc::Opcode::CMP_GE;
	case ArithOp::Eq:  return mc::Opcode::CMP_EQ;
	case ArithOp::Neq: return mc::Opcode::CMP_NEQ;
	}

	hbt_unreachable();
}

static mc::Opcode conversionForType(Type t)
{
	switch (t) {
	case Type::Bool:   return mc::Opcode::CONV_B;
	case Type::UInt8:  return mc::Opcode::CONV_U8;
	case Type::UInt16: return mc::Opcode::CONV_U16;
	case Type::UInt32: return mc::Opcode::CONV_U32;
	case Type::UInt64: return mc::Opcode::CONV_U64;
	case Type::Int8:   return mc::Opcode::CONV_S8;
	case Type::Int16:  return mc::Opcode::CONV_S16;
	case Type::Int32:  return mc::Opcode::CONV_S32;
	case Type::Int64:  return mc::Opcode::CONV_S64;
	case Type::Float:  return mc::Opcode::CONV_F;
	}

	hbt_unreachable();
}

static mc::DTMask datePartToDTMask(DatePart part)
{
	switch (part) {
	case DatePart::Second:  return mc::DTMask::second;
	case DatePart::Minute:  return mc::DTMask::minute;
	case DatePart::Hour:    return mc::DTMask::hour;
	case DatePart::Day:     return mc::DTMask::day;
	case DatePart::Month:   return mc::DTMask::month;
	case DatePart::Year:    return mc::DTMask::year;
	case DatePart::Weekday: return mc::DTMask::weekday;
	}

	hbt_unreachable();
}



class MachineBlockWithStack {
private:
	const Module& module;
	const BasicBlock* currentBlock;

	mc::Label _label;
	std::vector<std::pair<mc::Opcode, boost::optional<mc::Builder::immediate_t>>> _instructions;
	MachineBlockWithStack* _continueWith;

	std::set<MachineBlockWithStack*> _successors, _predecessors;

	std::map<const Value*, unsigned> materializedValues;
	std::map<const Value*, const Instruction*> unmaterializedValues;
	unsigned anonymousSlotCount;

	bool isValueLiveAfterUse(const Value* v, const Instruction* user) const
	{
		auto& lastUses = module.lastValueUsesIn(currentBlock);
		if (lastUses.count(v) && lastUses.at(v) != user)
			return true;

		for (auto& succ : module.successorsOf(currentBlock)) {
			if (module.liveValuesIn(succ).count(v))
				return true;
		}

		return false;
	}

	void materializeArith(const ArithmeticInsn& a);
	void materializeCast(const CastInsn& c);
	void materializeLoadSpecial(const LoadSpecialInsn& l);
	void materializeLoadInt(const LoadIntInsn& l);
	void materializeLoadFloat(const LoadFloatInsn& l);
	void materializeLoad(const LoadInsn& l);
	void materializeCompareIP(const CompareIPInsn& c);
	void materializeExtractDatePart(const ExtractDatePartInsn& e);
	void materialize(const Value* v);

public:
	MachineBlockWithStack(const Module& module, const BasicBlock* currentBlock, mc::Label label)
		: module(module), currentBlock(currentBlock), _label(label), _continueWith(nullptr), anonymousSlotCount(0)
	{}

	MachineBlockWithStack(MachineBlockWithStack&&) = default;
	MachineBlockWithStack& operator=(MachineBlockWithStack&&) = default;

	void loadForUser(const Value* v, const Instruction* user, bool inhibitKill = false);
	void provide(const Value* v, const Instruction* provider);

	MachineBlockWithStack fork(const BasicBlock* newBlock, mc::Label newLabel) const;
	void emitInterblockGlue(const BasicBlock* from);

	void append(mc::Opcode op)
	{
		_instructions.push_back({ op, {} });
	}

	template<typename Arg>
	void append(mc::Opcode op, Arg&& arg)
	{
		_instructions.push_back({ op, {{ std::forward<Arg>(arg) }} });
	}

	void materializeAllLoads();
	void materializeRemainingLiveValues();

	void jumpsTo(MachineBlockWithStack* next)
	{
		_successors.insert(next);
		next->_predecessors.insert(this);
	}

	mc::Label label() const { return _label; }
	auto instructions() -> decltype(_instructions) { return _instructions; }
	MachineBlockWithStack* continueWith() { return _continueWith; }
	std::set<MachineBlockWithStack*> successors() { return _successors; }
	std::set<MachineBlockWithStack*> predecessors() { return _predecessors; }

	void continueWith(MachineBlockWithStack* m)
	{
		_continueWith = m;
		jumpsTo(m);
	}
};

void MachineBlockWithStack::materializeArith(const ArithmeticInsn& a)
{
	loadForUser(a.left(), &a, a.left() == a.right());
	anonymousSlotCount++;
	loadForUser(a.right(), &a);
	append(arithOpToOpcode(a.op()));
	anonymousSlotCount--;
}

void MachineBlockWithStack::materializeCast(const CastInsn& c)
{
	loadForUser(c.value(), &c);
	append(conversionForType(c.type()));
}

void MachineBlockWithStack::materializeLoadSpecial(const LoadSpecialInsn& l)
{
	switch (l.val()) {
	case SpecialVal::SourceEID: append(mc::Opcode::LD_SOURCE_EID); break;
	case SpecialVal::SourceVal: append(mc::Opcode::LD_SOURCE_VAL); break;
	case SpecialVal::SysTime:   append(mc::Opcode::LD_SYSTIME); break;
	}
}

void MachineBlockWithStack::materializeLoadInt(const LoadIntInsn& l)
{
	switch (l.type()) {
	case Type::Bool:   append(l.value() == 0 ? mc::Opcode::LD_FALSE : mc::Opcode::LD_TRUE); break;
	case Type::UInt8:  append(mc::Opcode::LD_U8, uint8_t(l.value().toU64())); break;
	case Type::UInt16: append(mc::Opcode::LD_U16, uint16_t(l.value().toU64())); break;
	case Type::UInt32: append(mc::Opcode::LD_U32, uint32_t(l.value().toU64())); break;
	case Type::UInt64: append(mc::Opcode::LD_U64, uint64_t(l.value().toU64())); break;
	case Type::Int8:   append(mc::Opcode::LD_S8, int8_t(l.value().toS64())); break;
	case Type::Int16:  append(mc::Opcode::LD_S16, int16_t(l.value().toS64())); break;
	case Type::Int32:  append(mc::Opcode::LD_S32, int32_t(l.value().toS64())); break;
	case Type::Int64:  append(mc::Opcode::LD_S64, int64_t(l.value().toS64())); break;
	case Type::Float:  break;
	}
}

void MachineBlockWithStack::materializeLoadFloat(const LoadFloatInsn& l)
{
	append(mc::Opcode::LD_FLOAT, l.value());
}

void MachineBlockWithStack::materializeLoad(const LoadInsn& l)
{
	append(mc::Opcode::LD_MEM, std::make_tuple(memtypeForType(l.type()), unsigned(l.address())));
}

void MachineBlockWithStack::materializeCompareIP(const CompareIPInsn& c)
{
	append(mc::Opcode::CMP_SRC_IP, mc::BlockPart(c.start(), c.length(), c.block()));
}

void MachineBlockWithStack::materializeExtractDatePart(const ExtractDatePartInsn& e)
{
	loadForUser(e.value(), &e);
	append(mc::Opcode::DT_DECOMPOSE, datePartToDTMask(e.part()));
}

void MachineBlockWithStack::materialize(const Value* v)
{
	auto* generatingInstruction = unmaterializedValues.at(v);
	unmaterializedValues.erase(v);

	switch (generatingInstruction->insnType()) {
	case InsnType::Arithmetic:
		materializeArith(static_cast<const ArithmeticInsn&>(*generatingInstruction));
		break;
	case InsnType::Cast:
		materializeCast(static_cast<const CastInsn&>(*generatingInstruction));
		break;
	case InsnType::SpecialValue:
		materializeLoadSpecial(static_cast<const LoadSpecialInsn&>(*generatingInstruction));
		break;
	case InsnType::IntegralConstant:
		materializeLoadInt(static_cast<const LoadIntInsn&>(*generatingInstruction));
		break;
	case InsnType::FloatConstant:
		materializeLoadFloat(static_cast<const LoadFloatInsn&>(*generatingInstruction));
		break;
	case InsnType::Load:
		materializeLoad(static_cast<const LoadInsn&>(*generatingInstruction));
		break;
	case InsnType::CompareIP:
		materializeCompareIP(static_cast<const CompareIPInsn&>(*generatingInstruction));
		break;
	case InsnType::ExtractDatePart:
		materializeExtractDatePart(static_cast<const ExtractDatePartInsn&>(*generatingInstruction));
		break;

	case InsnType::Store:
	case InsnType::Switch:
	case InsnType::Jump:
	case InsnType::Return:
	case InsnType::Write:
		throw CodegenCantDo("non-value value sources");
	case InsnType::Phi:
		throw CodegenCantDo("lazy phi materialization");
	}

	materializedValues.insert({ v, materializedValues.size() + anonymousSlotCount });
}

void MachineBlockWithStack::loadForUser(const Value* v, const Instruction* user, bool inhibitKill)
{
	if (unmaterializedValues.count(v))
		materialize(v);

	unsigned topSlot = materializedValues.size() - 1 + anonymousSlotCount;
	unsigned slotOffset = topSlot - materializedValues.at(v);

	if (slotOffset > 255)
		throw CodegenCantDo("large stacks");

	if ((user && isValueLiveAfterUse(v, user)) || inhibitKill) {
		if (slotOffset == 0)
			append(mc::Opcode::DUP);
		else
			append(mc::Opcode::DUP_I, uint8_t(slotOffset));
	} else {
		materializedValues.erase(v);
		for (auto& slot : materializedValues) {
			if (slot.second >= topSlot - slotOffset)
				slot.second -= 1;
		}

		if (!user) {
			if (slotOffset == 0)
				append(mc::Opcode::POP);
			else
				append(mc::Opcode::POP_I, uint8_t(slotOffset));
			return;
		}

		if (slotOffset == 0)
			return;

		if (slotOffset == 1)
			append(mc::Opcode::ROT);
		else
			append(mc::Opcode::ROT_I, uint8_t(slotOffset));
	}
}

void MachineBlockWithStack::provide(const Value* v, const Instruction* provider)
{
	if (!materializedValues.count(v))
		unmaterializedValues.insert({ v, provider });
}

MachineBlockWithStack MachineBlockWithStack::fork(const BasicBlock* newBlock, mc::Label newLabel) const
{
	MachineBlockWithStack result(module, newBlock, newLabel);

	result.materializedValues = materializedValues;
	result.unmaterializedValues = unmaterializedValues;

	return result;
}

void MachineBlockWithStack::emitInterblockGlue(const BasicBlock* from)
{
	std::map<unsigned, const Value*> newlyDeadValues;

	for (auto* oldLive : module.liveValuesIn(from)) {
		if (materializedValues.count(oldLive) && !module.liveValuesIn(currentBlock).count(oldLive))
			newlyDeadValues.insert({ unsigned(-1) - materializedValues.at(oldLive), oldLive });
	}

	for (auto& dead : newlyDeadValues)
		loadForUser(dead.second, nullptr);

	for (auto& insn : currentBlock->instructions()) {
		if (insn->insnType() != InsnType::Phi)
			break;

		auto& phi = static_cast<const PhiInsn&>(*insn);
		auto* value = phi.sources().at(from);
		loadForUser(value, &phi);
		materializedValues.insert({ &phi, materializedValues.size() });
	}
}

void MachineBlockWithStack::materializeAllLoads()
{
	bool moreWork = true;
	while (moreWork) {
		moreWork = false;
		for (auto& val : unmaterializedValues) {
			if (val.second->insnType() != InsnType::Load)
				continue;

			materialize(val.first);
			moreWork = true;
			break;
		}
	}
}

void MachineBlockWithStack::materializeRemainingLiveValues()
{
	for (auto* target : module.successorsOf(currentBlock)) {
		for (auto* live : module.liveValuesIn(target))
			if (unmaterializedValues.count(live))
				materialize(live);
	}
}



class Codegen {
private:
	const Module& module;
	mc::Builder builder;

	std::list<const BasicBlock*> emitterQueue;

	std::list<std::unique_ptr<MachineBlockWithStack>> machineBlocks;
	std::map<const BasicBlock*, MachineBlockWithStack*> machineBlocksByBB;

	MachineBlockWithStack& machineBlockFor(const BasicBlock* block, MachineBlockWithStack* from = nullptr)
	{
		if (!machineBlocksByBB.count(block)) {
			std::unique_ptr<MachineBlockWithStack> next;

			if (from)
				next.reset(new auto(from->fork(block, builder.createLabel(block->name()))));
			else
				next.reset(new MachineBlockWithStack(module, block, builder.createLabel(block->name())));

			machineBlocksByBB.insert({ block, next.get() });
			machineBlocks.push_back(std::move(next));
		}

		return *machineBlocksByBB.at(block);
	}

	void emitBlock(const BasicBlock* block);

public:
	Codegen(const Module& module)
		: module(module), builder(module.version())
	{}

	std::unique_ptr<mc::Builder> run();
};

void Codegen::emitBlock(const BasicBlock* block)
{
	auto& mb = machineBlockFor(block);

	auto entryBlockForJumpTo = [&] (const BasicBlock* target) -> MachineBlockWithStack& {
		std::unique_ptr<MachineBlockWithStack> glue(new auto(mb.fork(target, builder.createLabel())));

		glue->emitInterblockGlue(block);
		auto& next = machineBlockFor(target, glue.get());

		if (glue->instructions().empty()) {
			mb.jumpsTo(&next);
			return next;
		} else {
			glue->continueWith(&next);
			mb.jumpsTo(glue.get());
			machineBlocks.push_back(std::move(glue));
			return *machineBlocks.back();
		}
	};

	auto emitJumpTo = [&] (const BasicBlock* target) {
		auto& glue = entryBlockForJumpTo(target);
		mb.continueWith(&glue);
	};

	for (auto& insn : block->instructions()) {
		switch (insn->insnType()) {
		case InsnType::Arithmetic:
		case InsnType::Cast:
		case InsnType::SpecialValue:
		case InsnType::IntegralConstant:
		case InsnType::FloatConstant:
		case InsnType::Load:
		case InsnType::CompareIP:
		case InsnType::ExtractDatePart:
			mb.provide(dynamic_cast<const Value*>(insn), insn);
			break;

		case InsnType::Store: {
			auto& s = static_cast<const StoreInsn&>(*insn);

			mb.materializeAllLoads();
			mb.loadForUser(s.value(), &s);
			mb.append(mc::Opcode::ST_MEM, std::make_tuple(memtypeForType(s.value()->type()), unsigned(s.address())));
			break;
		}

		case InsnType::Switch: {
			auto& s = static_cast<const SwitchInsn&>(*insn);

			mb.materializeRemainingLiveValues();

			bool isBoolSwitch = s.value()->type() == Type::Bool || (s.labels().size() == 1 && s.labels().count(0));
			bool isSigned = s.value()->type() == Type::Int8 || s.value()->type() == Type::Int16 ||
				s.value()->type() == Type::Int32 || s.value()->type() == Type::Int64;

			if (isBoolSwitch) {
				auto* trueBranch = s.labels().count(1) ? s.labels().at(1) : s.defaultLabel();
				auto* falseBranch = s.labels().count(0) ? s.labels().at(0) : s.defaultLabel();

				mb.loadForUser(s.value(), &s);
				auto& glue = entryBlockForJumpTo(falseBranch);
				mb.append(mc::Opcode::JZ, glue.label());
				emitJumpTo(trueBranch);
				return;
			}

			std::map<util::Bigint, const BasicBlock*> entries8, entries16, entries32, entries64;
			for (auto& label : s.labels()) {
				if ((isSigned && label.first.fitsInto<int8_t>()) || (!isSigned && label.first.fitsInto<uint8_t>()))
					entries8.insert({ label.first, label.second });
				else if ((isSigned && label.first.fitsInto<int16_t>()) || (!isSigned && label.first.fitsInto<uint16_t>()))
					entries16.insert({ label.first, label.second });
				else if ((isSigned && label.first.fitsInto<int32_t>()) || (!isSigned && label.first.fitsInto<uint32_t>()))
					entries32.insert({ label.first, label.second });
				else
					entries64.insert({ label.first, label.second });
			}

			unsigned e64done = 0;
			for (auto& e : entries64) {
				bool more = entries8.size() || entries16.size() || entries32.size();
				e64done++;
				mb.loadForUser(s.value(), &s, more || e64done != entries64.size());
				if (isSigned)
					mb.append(mc::Opcode::LD_S64, e.first.toS64());
				else
					mb.append(mc::Opcode::LD_U64, e.first.toU64());
				mb.append(mc::Opcode::CMP_EQ);
				auto& glue = entryBlockForJumpTo(e.second);
				mb.append(mc::Opcode::JNZ, glue.label());
			}

			auto emitPartialSwitch = [&] (const std::map<util::Bigint, const BasicBlock*>& parts, mc::Opcode op, bool more) {
				if (!parts.size())
					return;

				uint32_t mask = op == mc::Opcode::SWITCH_8
					? std::numeric_limits<uint8_t>::max()
					: op == mc::Opcode::SWITCH_16
						? std::numeric_limits<uint16_t>::max()
						: std::numeric_limits<uint32_t>::max();

				std::vector<mc::SwitchEntry> entries;
				unsigned total = 0;

				auto flush = [&] () {
					if (!entries.size())
						return;

					total += entries.size();
					mb.append(op, mc::SwitchTable(entries.begin(), entries.end()));
					entries.clear();
					if (parts.size() > total)
						mb.loadForUser(s.value(), &s, more || parts.size() - total > 255);
				};

				mb.loadForUser(s.value(), &s, more || parts.size() - total > 255);
				for (auto& e : parts) {
					auto& glue = entryBlockForJumpTo(e.second);
					if (isSigned)
						entries.push_back({ mask & uint32_t(e.first.toS32()), glue.label() });
					else
						entries.push_back({ mask & e.first.toU32(), glue.label() });

					if (entries.size() == 255)
						flush();
				}
				flush();
			};

			emitPartialSwitch(entries8, mc::Opcode::SWITCH_8, entries16.size() || entries32.size());
			emitPartialSwitch(entries16, mc::Opcode::SWITCH_16, entries32.size());
			emitPartialSwitch(entries32, mc::Opcode::SWITCH_32, false);

			if (s.defaultLabel())
				emitJumpTo(s.defaultLabel());

			return;
		}

		case InsnType::Jump: {
			auto& j = static_cast<const JumpInsn&>(*insn);

			mb.materializeRemainingLiveValues();
			emitJumpTo(j.target());
			return;
		}

		case InsnType::Return:
			mb.append(mc::Opcode::RET);
			return;

		case InsnType::Write: {
			auto& w = static_cast<const WriteInsn&>(*insn);

			mb.loadForUser(w.value(), &w);
			switch (w.value()->type()) {
			case Type::Bool: mb.append(mc::Opcode::CONV_B); break;
			case Type::UInt8: mb.append(mc::Opcode::CONV_U8); break;
			case Type::UInt16: mb.append(mc::Opcode::CONV_U16); break;
			case Type::Int8: mb.append(mc::Opcode::CONV_S8); break;
			case Type::Int16: mb.append(mc::Opcode::CONV_S16); break;

			case Type::UInt32:
			case Type::Int32:
			case Type::UInt64:
			case Type::Int64:
			case Type::Float:
				break;
			}
			mb.append(mc::Opcode::LD_U32, w.eid());
			mb.append(mc::Opcode::WRITE);
			break;
		}

		case InsnType::Phi:
			break;
		}
	}
}

std::unique_ptr<mc::Builder> Codegen::run()
{
	auto&& basicBlocks = util::topoSort(module.blocks(),
		[] (const BasicBlock* b) {
			return b;
		},
		[&] (const BasicBlock* b) {
			return module.predeccorsOf(b);
		});
	for (auto* block : basicBlocks)
		emitBlock(block);

	auto queue = util::topoSort(machineBlocks,
		[] (const std::unique_ptr<MachineBlockWithStack>& b) {
			return b.get();
		},
		[] (MachineBlockWithStack* mb) {
			return mb->predecessors();
		});

	std::set<MachineBlockWithStack*> blocksDone;
	unsigned line = 5;

	while (!queue.empty()) {
		auto* block = queue.front();
		queue.pop_front();

		if (!blocksDone.insert(block).second)
			continue;

		line++;

		struct {
			boost::optional<mc::Label> _label;

			operator boost::optional<mc::Label>()
			{
				boost::optional<mc::Label> result = _label;
				_label.reset();
				return result;
			}
		} label = { block->label() };
		for (auto& insn : block->instructions()) {
			if (!insn.second) {
				builder.insert(label, insn.first, line++);
				continue;
			}

			builder.insert(label, insn.first, std::move(*insn.second), line++);
		}

		if (block->continueWith())
			builder.insert(label, mc::Opcode::JUMP, block->continueWith()->label(), line++);
	}

	if (module.onInit())
		builder.onInit(machineBlocksByBB.at(module.onInit())->label());
	if (module.onPacket())
		builder.onPacket(machineBlocksByBB.at(module.onPacket())->label());
	if (module.onPeriodic())
		builder.onPeriodic(machineBlocksByBB.at(module.onPeriodic())->label());

	return std::unique_ptr<mc::Builder>(new mc::Builder(std::move(builder)));
}

}

std::unique_ptr<mc::Builder> generateMachineCode(const Module& module)
{
	return Codegen(module).run();
}

}
}
