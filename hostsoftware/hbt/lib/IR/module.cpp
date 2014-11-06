#include "hbt/IR/module.hpp"

#include "hbt/IR/basicblock.hpp"
#include "hbt/Util/toposort.hpp"

#include <algorithm>
#include <list>
#include <queue>
#include <sstream>
#include <vector>

namespace hbt {
namespace ir {

namespace {

class ModuleVerifier {
	friend ModuleBuilder;
private:
	ModuleBuilder& module;
	std::stringstream* diagStream;
	bool errorsDetected;
	unsigned insnNumber;

	std::map<const BasicBlock*, std::set<const BasicBlock*>> successors;
	std::map<const BasicBlock*, std::set<const BasicBlock*>> predecessors;

	std::map<const BasicBlock*, std::map<const Value*, const Instruction*>>
		lastValueUsesByBlock;
	std::map<const BasicBlock*, std::set<const Value*>> liveValuesByBlock;

	void toplevelError(const std::string& msg);
	void insnError(const BasicBlock& in, const std::string& msg);

	bool checkPointerValidity();
	bool checkNameUniqueness();
	bool checkBlockFormat();
	bool buildBlockMaps();
	bool checkBlockInstructions();
	bool buildValueLivenessMap();

	void verifyInsn(const BasicBlock* in, const Instruction& insn);

public:
	ModuleVerifier(ModuleBuilder& module, std::stringstream* diagStream)
		: module(module), diagStream(diagStream), errorsDetected(false)
	{}

	bool verify();
};

void ModuleVerifier::toplevelError(const std::string& msg)
{
	if (diagStream)
		*diagStream << "error: " << msg << '\n';
	errorsDetected = true;
}

void ModuleVerifier::insnError(const BasicBlock& in, const std::string& msg)
{
	if (diagStream)
		*diagStream << "error in block " << in.name() << ":" << insnNumber << ": "
			<< msg << '\n';

	errorsDetected = true;
}

bool ModuleVerifier::checkPointerValidity()
{
	bool onInitSeen = false;
	bool onPacketSeen = false;
	bool onPeriodicSeen = false;

	for (auto& block : module.blocks()) {
		onInitSeen |= module.onInit() == block.get();
		onPacketSeen |= module.onPacket() == block.get();
		onPeriodicSeen |= module.onPeriodic() == block.get();

		insnNumber = 0;
		bool terminated = false;
		for (auto& insn : block->instructions()) {
			insnNumber++;

			if (terminated) {
				insnError(*block, "insn after terminator");
				break;
			}

			auto check = [&] (const Value* v) {
				if (!v)
					insnError(*block, "value nullptr");
			};

			switch (insn->insnType()) {
			case InsnType::Arithmetic: {
				auto& a = static_cast<const ArithmeticInsn&>(*insn);

				check(a.left());
				check(a.right());
				break;
			}

			case InsnType::Cast:
			case InsnType::Store:
			case InsnType::Write:
			case InsnType::ExtractDatePart:
				check(static_cast<const UnaryInsn&>(*insn).value());
				break;

			case InsnType::Switch: {
				auto& s = static_cast<const SwitchInsn&>(*insn);

				for (auto& label : s.labels()) {
					if (!label.second)
						insnError(*block, "block nullptr");
				}

				check(s.value());
				break;
			}

			case InsnType::Phi: {
				auto& p = static_cast<const PhiInsn&>(*insn);

				for (auto& src : p.sources()) {
					if (!src.first)
						insnError(*block, "block nullptr");
					check(src.second);
				}

				break;
			}

			case InsnType::SpecialValue:
			case InsnType::IntegralConstant:
			case InsnType::FloatConstant:
			case InsnType::Load:
			case InsnType::CompareIP:
				break;

			case InsnType::Return:
				terminated = true;
				break;

			case InsnType::Jump:
				if (!static_cast<const JumpInsn&>(*insn).target())
					insnError(*block, "block nullptr");
				terminated = true;
				break;
			}
		}
	}

	if (module.onInit() && !onInitSeen)
		toplevelError("on_init block not in module");
	if (module.onPacket() && !onPacketSeen)
		toplevelError("on_packet block not in module");
	if (module.onPeriodic() && !onPeriodicSeen)
		toplevelError("on_periodic block not in module");

	return !errorsDetected;
}

bool ModuleVerifier::checkNameUniqueness()
{
	std::set<std::string> valueNames, blockNames;

	for (auto& block : module.blocks()) {
		if (!blockNames.insert(block->name()).second)
			toplevelError("block name " + block->name() + " reused");

		insnNumber = 0;
		for (auto& insn : block->instructions()) {
			insnNumber++;

			if (auto* val = dynamic_cast<const Value*>(insn.get())) {
				if (!valueNames.insert(val->name()).second)
					insnError(*block, "value name " + val->name() + " reused");
			}
		}
	}

	return !errorsDetected;
}

bool ModuleVerifier::checkBlockFormat()
{
	for (auto& block : module.blocks()) {
		bool onlyPhiSeen = true;
		insnNumber = 0;
		for (auto& insn : block->instructions()) {
			insnNumber++;

			if (insn->insnType() == InsnType::Phi && !onlyPhiSeen)
				insnError(*block, "block contains phi in the middle");

			onlyPhiSeen &= insn->insnType() == InsnType::Phi;
		}

		if (!block->instructions().size() || !block->instructions().back()->isTerminator())
			toplevelError("block " + block->name() + " not terminated");
	}

	return !errorsDetected;
}

bool ModuleVerifier::buildBlockMaps()
{
	std::set<const BasicBlock*> blocksProvided;

	for (auto& block : module.blocks()) {
		blocksProvided.insert(block.get());
		auto& sucs = successors[block.get()];

		for (auto& insn : block->instructions()) {
			switch (insn->insnType()) {
			case InsnType::Switch: {
				auto& s = static_cast<const SwitchInsn&>(*insn);

				for (auto& label : s.labels())
					sucs.insert(label.second);

				if (s.defaultLabel())
					sucs.insert(s.defaultLabel());

				break;
			}

			case InsnType::Jump:
				sucs.insert(static_cast<const JumpInsn&>(*insn).target());
				break;

			case InsnType::Arithmetic:
			case InsnType::Cast:
			case InsnType::Store:
			case InsnType::Write:
			case InsnType::ExtractDatePart:
			case InsnType::SpecialValue:
			case InsnType::IntegralConstant:
			case InsnType::FloatConstant:
			case InsnType::Load:
			case InsnType::CompareIP:
			case InsnType::Phi:
			case InsnType::Return:
				break;
			}
		}
	}

	for (auto& block : module.blocks())
		predecessors.insert({ block.get(), {} });

	for (auto& sucPair : successors) {
		for (auto& next : sucPair.second) {
			if (!blocksProvided.count(next))
				toplevelError("block " + next->name() + " not in module");

			predecessors[next].insert(sucPair.first);
		}
	}

	auto circularPredecessors = predecessors;
	std::list<const BasicBlock*> queue;

	for (auto& pair : predecessors) {
		if (pair.second.empty())
			queue.push_back(pair.first);
	}

	while (!queue.empty()) {
		auto* block = queue.front();
		queue.pop_front();

		for (auto& succ : successors[block]) {
			auto& preds = circularPredecessors[succ];
			preds.erase(block);
			if (preds.empty())
				queue.push_back(succ);
		}
	}

	for (auto& circ: circularPredecessors) {
		if (!circ.second.empty())
			toplevelError("block " + circ.first->name() + " is circular");
	}

	return !errorsDetected;
}

bool ModuleVerifier::checkBlockInstructions()
{
	for (auto& block : module.blocks()) {
		insnNumber = 0;
		for (auto& insn : block->instructions()) {
			insnNumber++;
			verifyInsn(block.get(), *insn);
		}
	}

	return !errorsDetected;
}

bool ModuleVerifier::buildValueLivenessMap()
{
	std::map<const BasicBlock*, std::set<const BasicBlock*>> predecessorsNotSeen(predecessors);

	std::list<const BasicBlock*> blockQueue = util::topoSort(
		module.blocks(),
		[] (const std::unique_ptr<BasicBlock>& b) -> const BasicBlock* {
			return b.get();
		},
		[&] (const BasicBlock* b) {
			return predecessors[b];
		});

	std::map<const BasicBlock*, std::set<const Value*>> availableValues;
	std::map<const BasicBlock*, std::set<const Value*>> valueUses, phisInSuccessors, phiOnlyUses;
	std::map<const BasicBlock*, std::set<const Value*>> valueDefinitions;

	for (auto* block : blockQueue) {
		if (!predecessors[block].size()) {
			availableValues.insert({ block, {} });
		} else {
			availableValues[block] = availableValues[*predecessors[block].begin()];
			for (auto* pred : predecessors[block]) {
				std::set<const Value*> previouslyAvailable;

				swap(previouslyAvailable, availableValues[block]);
				std::set_intersection(
					previouslyAvailable.begin(), previouslyAvailable.end(),
					availableValues[pred].begin(), availableValues[pred].end(),
					std::inserter(availableValues[block], availableValues[block].end()));
			}
		}

		insnNumber = 0;
		for (auto& insn : block->instructions()) {
			insnNumber++;

			auto use = [&] (const Value* v, const BasicBlock* usedFrom = nullptr) {
				if (!availableValues[usedFrom ? usedFrom : block].count(v))
					insnError(*block, "used value isn't live");

				if (usedFrom && !valueUses[block].count(v)) {
					phisInSuccessors[usedFrom].insert(v);
					phiOnlyUses[block].insert(v);
				} else {
					phiOnlyUses[block].erase(v);
					valueUses[block].insert(v);
				}

				lastValueUsesByBlock[block][v] = insn;
			};

			switch (insn->insnType()) {
			case InsnType::Arithmetic: {
				auto& a = static_cast<const ArithmeticInsn&>(*insn);

				use(a.left());
				use(a.right());
				break;
			}

			case InsnType::Cast:
			case InsnType::Store:
			case InsnType::Write:
			case InsnType::Switch:
			case InsnType::ExtractDatePart:
				use(static_cast<const UnaryInsn&>(*insn).value());
				break;

			case InsnType::Phi:
				for (auto& src : static_cast<const PhiInsn&>(*insn).sources())
					use(src.second, src.first);

				break;

			case InsnType::SpecialValue:
			case InsnType::IntegralConstant:
			case InsnType::FloatConstant:
			case InsnType::Load:
			case InsnType::CompareIP:
			case InsnType::Jump:
			case InsnType::Return:
				break;
			}

			if (auto* v = dynamic_cast<const Value*>(insn)) {
				valueDefinitions[block].insert(v);
				availableValues[block].insert(v);
			}
		}
	}

	std::list<const BasicBlock*> livenessQueue = util::topoSort(
		module.blocks(),
		[] (const std::unique_ptr<BasicBlock>& b) -> const BasicBlock* {
			return b.get();
		},
		[&] (const BasicBlock* b) {
			return successors[b];
		});

	for (auto* block : livenessQueue) {
		for (auto* succ : successors[block]) {
			std::set_difference(
				liveValuesByBlock[succ].begin(), liveValuesByBlock[succ].end(),
				valueDefinitions[succ].begin(), valueDefinitions[succ].end(),
				std::inserter(liveValuesByBlock[block], liveValuesByBlock[block].end()));
		}

		liveValuesByBlock[block].insert(valueUses[block].begin(), valueUses[block].end());
		liveValuesByBlock[block].insert(phisInSuccessors[block].begin(), phisInSuccessors[block].end());
	}
	for (auto& phiOnly : phiOnlyUses)
		liveValuesByBlock[phiOnly.first].insert(phiOnly.second.begin(), phiOnly.second.end());

	return !errorsDetected;
}

static bool intFitsType(const util::Bigint& val, Type  t)
{
	switch (t) {
	case Type::Bool: return val.fitsInto<bool>();
	case Type::UInt8: return val.fitsInto<uint8_t>();
	case Type::UInt16: return val.fitsInto<uint16_t>();
	case Type::UInt32: return val.fitsInto<uint32_t>();
	case Type::UInt64: return val.fitsInto<uint64_t>();
	case Type::Int8: return val.fitsInto<int8_t>();
	case Type::Int16: return val.fitsInto<int16_t>();
	case Type::Int32: return val.fitsInto<int32_t>();
	case Type::Int64: return val.fitsInto<int64_t>();
	default: return false;
	}
}

void ModuleVerifier::verifyInsn(const BasicBlock* in, const Instruction& insn)
{
	switch (insn.insnType()) {
	case InsnType::Arithmetic: {
		auto& a = static_cast<const ArithmeticInsn&>(insn);

		Type targetType;
		bool argTypesCorrect = a.left()->type() == a.right()->type();
		switch (a.op()) {
		case ArithOp::Mul:
		case ArithOp::Div:
		case ArithOp::Mod:
		case ArithOp::Add:
		case ArithOp::Sub:
		case ArithOp::And:
		case ArithOp::Or:
		case ArithOp::Xor:
			targetType = a.left()->type();
			break;
		case ArithOp::Shl:
		case ArithOp::Shr:
			targetType = a.left()->type();
			argTypesCorrect = a.right()->type() == Type::UInt32;
			break;
		case ArithOp::Lt:
		case ArithOp::Le:
		case ArithOp::Gt:
		case ArithOp::Ge:
		case ArithOp::Eq:
		case ArithOp::Neq:
			targetType = Type::Bool;
			break;
		}
		if (!argTypesCorrect || a.type() != targetType)
			insnError(*in, "type mismatch in " + a.name());

		break;
	}

	case InsnType::SpecialValue: {
		auto& s = static_cast<const LoadSpecialInsn&>(insn);

		if ((s.val() == SpecialVal::SourceEID && s.type() != Type::UInt32) ||
			(s.val() == SpecialVal::SysTime && s.type() != Type::Int64))
			insnError(*in, "incorrect value type for " + s.name());

		break;
	}

	case InsnType::IntegralConstant: {
		auto& i = static_cast<const LoadIntInsn&>(insn);

		if (!intFitsType(i.value(), i.type()))
			insnError(*in, "value for " + i.name() + " exceeds range");

		break;
	}

	case InsnType::Cast:
	case InsnType::Jump:
	case InsnType::Return:
	case InsnType::FloatConstant:
	case InsnType::Write:
	case InsnType::ExtractDatePart:
		break;

	case InsnType::Load: {
		auto& l = static_cast<const LoadInsn&>(insn);

		if (l.address() >= 1 << 12)
			insnError(*in, "invalid address");

		break;
	}

	case InsnType::Store: {
		auto& s = static_cast<const StoreInsn&>(insn);

		if (s.address() >= 1 << 12)
			insnError(*in, "invalid address");

		break;
	}

	case InsnType::Switch: {
		auto& s = static_cast<const SwitchInsn&>(insn);

		if (s.value()->type() == Type::Float)
			insnError(*in, "switch on float invalid");

		std::set<util::Bigint> valuesUsed;
		for (auto& label : s.labels()) {
			if (!intFitsType(label.first, s.value()->type()))
				insnError(*in, "switch label overflows type");

			if (!valuesUsed.insert(label.first).second)
				insnError(*in, "label value duplicated");
		}

		bool exhaustive = s.defaultLabel();
		switch (s.value()->type()) {
		case Type::Bool:
			exhaustive |= valuesUsed.size() == 2;
			break;

		case Type::UInt8:
		case Type::Int8:
			exhaustive |= valuesUsed.size() == UINT8_MAX;
			break;

		case Type::UInt16:
		case Type::Int16:
			exhaustive |= valuesUsed.size() == UINT16_MAX;
			break;

		default:
			break;
		}

		if (!exhaustive)
			insnError(*in, "switch not exhaustive");

		break;
	}

	case InsnType::CompareIP: {
		auto& c = static_cast<const CompareIPInsn&>(insn);

		if (c.start() >= 16 || c.length() > 16 || c.start() + c.length() > 16)
			insnError(*in, "invalid block bounds for " + c.name());

		break;
	}

	case InsnType::Phi: {
		auto& p = static_cast<const PhiInsn&>(insn);

		if (p.sources().empty()) {
			insnError(*in, "empty phi source list");
		}

		for (auto& src : p.sources()) {
			if (!predecessors[in].count(src.first))
				insnError(*in, "invalid phi source " + src.first->name());
			if (src.second->type() != p.type())
				insnError(*in, "type mismatch");
		}

		if (p.sources().size() != predecessors[in].size())
			insnError(*in, "phi not exhaustive");

		break;
	}
	}
}

bool ModuleVerifier::verify()
{
	if (module.version() != 0)
		toplevelError("version must be 0");

	if (!checkPointerValidity())
		return false;
	if (!checkNameUniqueness())
		return false;
	if (!checkBlockFormat())
		return false;
	if (!buildBlockMaps())
		return false;
	if (!checkBlockInstructions())
		return false;
	if (!buildValueLivenessMap())
		return false;

	return !errorsDetected;
}

}



BasicBlock* ModuleBuilder::newBlock(std::string name)
{
	_blocks.push_back(std::unique_ptr<BasicBlock>(new BasicBlock(name)));
	return _blocks.back().get();
}

std::unique_ptr<Module> ModuleBuilder::finish(std::string* diags)
{
	std::stringstream diagStream;

	ModuleVerifier mv(*this, diags ? &diagStream : nullptr);

	if (!mv.verify()) {
		if (diags)
			*diags = diagStream.str();
		return nullptr;
	}

	return std::unique_ptr<Module>(
		new Module(
			std::move(_blocks),
			_version,
			_onInit,
			_onPacket,
			_onPeriodic,
			std::move(mv.successors),
			std::move(mv.predecessors),
			std::move(mv.lastValueUsesByBlock),
			std::move(mv.liveValuesByBlock)));
}

}
}
