#ifndef MODULE__HPP_EBE64C33EB42ADC1
#define MODULE__HPP_EBE64C33EB42ADC1

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "hbt/IR/basicblock.hpp"
#include "hbt/Util/iterator.hpp"
#include "hbt/Util/range.hpp"

namespace hbt {
namespace ir {

class ModuleBase {
protected:
	typedef std::vector<std::unique_ptr<BasicBlock>> block_list;
	typedef util::const_uptr_value_iterator<block_list> block_iterator;

	block_list _blocks;
	unsigned _version;
	const BasicBlock* _onInit;
	const BasicBlock* _onPacket;
	const BasicBlock* _onPeriodic;

protected:
	ModuleBase(unsigned version, const BasicBlock* onInit,
			const BasicBlock* onPacket, const BasicBlock* onPeriodic)
		: _version(version), _onInit(onInit), _onPacket(onPacket), _onPeriodic(onPeriodic)
	{}

	ModuleBase(std::vector<std::unique_ptr<BasicBlock>>&& blocks, unsigned version,
			const BasicBlock* onInit, const BasicBlock* onPacket, const BasicBlock* onPeriodic)
		: _blocks(std::move(blocks)), _version(version), _onInit(onInit), _onPacket(onPacket),
		  _onPeriodic(onPeriodic)
	{}

public:
	unsigned version() const { return _version; }
	const BasicBlock* onInit() const { return _onInit; }
	const BasicBlock* onPacket() const { return _onPacket; }
	const BasicBlock* onPeriodic() const { return _onPeriodic; }

	util::range<block_iterator> blocks() const { return _blocks; }
};

class Module : public ModuleBase {
	friend class ModuleBuilder;
private:
	std::map<const BasicBlock*, std::set<const BasicBlock*>> _successors;
	std::map<const BasicBlock*, std::set<const BasicBlock*>> _predecessors;

	std::map<const BasicBlock*, std::map<const Value*, const Instruction*>>
		_lastValueUsesByBlock;
	std::map<const BasicBlock*, std::set<const Value*>> _liveValuesByBlock;
	std::map<const Value*, std::set<const BasicBlock*>> _valuesLiveIn;

	Module(std::vector<std::unique_ptr<BasicBlock>>&& blocks, unsigned version,
			const BasicBlock* onInit, const BasicBlock* onPacket,
			const BasicBlock* onPeriodic,
			std::map<const BasicBlock*, std::set<const BasicBlock*>>&& successors,
			std::map<const BasicBlock*, std::set<const BasicBlock*>>&& predecessors,
			std::map<const BasicBlock*, std::map<const Value*, const Instruction*>>&&
				lastValueUsesByBlock,
			std::map<const BasicBlock*, std::set<const Value*>>&& liveValuesByBlock)
		: ModuleBase(std::move(blocks), version, onInit, onPacket, onPeriodic),
		  _successors(std::move(successors)), _predecessors(std::move(predecessors)),
		  _lastValueUsesByBlock(std::move(lastValueUsesByBlock)),
		  _liveValuesByBlock(std::move(liveValuesByBlock))
	{
		for (auto& valuesByBlock : _liveValuesByBlock) {
			for (auto* val : valuesByBlock.second)
				_valuesLiveIn[val].insert(valuesByBlock.first);
		}
	}

public:
	const std::set<const BasicBlock*>& successorsOf(const BasicBlock* b) const
	{
		return _successors.at(b);
	}

	const std::set<const BasicBlock*>& predeccorsOf(const BasicBlock* b) const
	{
		return _predecessors.at(b);
	}

	const std::map<const Value*, const Instruction*>& lastValueUsesIn(const BasicBlock* b) const
	{
		return _lastValueUsesByBlock.at(b);
	}

	const std::set<const Value*> liveValuesIn(const BasicBlock* b) const
	{
		return _liveValuesByBlock.at(b);
	}

	const std::set<const BasicBlock*>& valueLiveBlocks(const Value* v) const
	{
		return _valuesLiveIn.at(v);
	}
};

class ModuleBuilder : public ModuleBase {
public:
	ModuleBuilder(unsigned version)
		: ModuleBase(version, nullptr, nullptr, nullptr)
	{}

	explicit ModuleBuilder(Module&& mod)
		: ModuleBase(std::move(mod))
	{}

	using ModuleBase::blocks;
	using ModuleBase::onInit;
	using ModuleBase::onPacket;
	using ModuleBase::onPeriodic;

	block_list& blocks() { return _blocks; }
	void onInit(const BasicBlock* b) { _onInit = b; }
	void onPacket(const BasicBlock* b) { _onPacket = b; }
	void onPeriodic(const BasicBlock* b) { _onPeriodic = b; }

	BasicBlock* newBlock(std::string name);

	std::unique_ptr<Module> finish(std::string* diags = nullptr);
};

}
}

#endif
