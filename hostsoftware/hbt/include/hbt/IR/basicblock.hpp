#ifndef BASICBLOCK__HPP_5325F315AAE973FF
#define BASICBLOCK__HPP_5325F315AAE973FF

#include <memory>
#include <string>
#include <utility>

#include <boost/container/list.hpp>

#include "hbt/IR/instruction.hpp"
#include "hbt/Util/iterator.hpp"
#include "hbt/Util/range.hpp"

namespace hbt {
namespace ir {

class Builder;



class BasicBlock {
	friend Builder;

private:
	typedef boost::container::list<std::unique_ptr<Instruction>> insn_list;
public:
	typedef util::const_uptr_value_iterator<insn_list, BasicBlock> insn_iterator;

private:
	std::string _name;
	insn_list _instructions;

public:
	BasicBlock(std::string name)
		: _name(std::move(name))
	{}

	const std::string& name() const { return _name; }
	util::range<insn_iterator> instructions() const { return _instructions; }

	insn_list& instructions() { return _instructions; }

	template<typename Insn>
	auto insert(insn_iterator pos, Insn insn)
		-> typename std::enable_if<!std::is_base_of<Value, Insn>::value, void>::type
	{
		_instructions.insert(
			pos.baseIterator(),
			std::unique_ptr<Insn>(new Insn(std::move(insn))));
	}

	template<typename Insn>
	auto insert(insn_iterator pos, Insn insn)
		-> typename std::enable_if<std::is_base_of<Value, Insn>::value, const Value*>::type
	{
		auto it = _instructions.insert(
			pos.baseIterator(),
			std::unique_ptr<Insn>(new Insn(std::move(insn))));
		return static_cast<Insn*>(it->get());
	}

	template<typename Insn>
	auto append(Insn insn)
		-> decltype(insert(_instructions.cend(), std::move(insn)))
	{
		return insert(_instructions.cend(), std::move(insn));
	}

	void erase(insn_iterator begin, insn_iterator end)
	{
		_instructions.erase(begin.baseIterator(), end.baseIterator());
	}
};

}
}

#endif
