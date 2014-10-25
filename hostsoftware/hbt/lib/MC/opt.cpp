#include "MC/opt.hpp"

#include "MC/builder.hpp"

#include <map>
#include <set>
#include <vector>

#include <iostream>

namespace hbt {
namespace mc {

void threadTrivialJumps(Builder& b)
{
	std::map<unsigned, Label> forwardingLabels;

	for (auto* insn : b.instructions()) {
		if (insn->label() && insn->opcode() == Opcode::JUMP)
			forwardingLabels.insert({
				insn->label()->id(),
				dynamic_cast<const ImmediateInstruction<Label>&>(*insn).immed() });
	}

	auto forward = [&] (Label l) {
		while (forwardingLabels.count(l.id()))
			l = forwardingLabels.at(l.id());
		return l;
	};

	if (b.onPacket())
		b.onPacket(forward(*b.onPacket()));
	if (b.onPeriodic())
		b.onPeriodic(forward(*b.onPeriodic()));
	if (b.onInit())
		b.onInit(forward(*b.onInit()));

	for (auto it = b.instructions().begin(), end = b.instructions().end(); it != end; ++it) {
		boost::optional<Label> label;

		if ((*it)->label())
			label = *(*it)->label();

		if (auto* sw = dynamic_cast<const ImmediateInstruction<SwitchTable>*>(*it)) {
			b.insertBefore(it);

			std::vector<SwitchEntry> entries;
			for (auto& e : sw->immed())
				entries.push_back({ e.label, forward(e.target) });

			it = b.insert(label, sw->opcode(), SwitchTable(entries.begin(), entries.end()), sw->line());
			b.erase(std::next(it), std::next(std::next(it)));
			continue;
		}

		if (auto* j = dynamic_cast<const ImmediateInstruction<Label>*>(*it)) {
			b.insertBefore(it);

			it = b.insert(label, j->opcode(), forward(j->immed()), j->line());
			b.erase(std::next(it), std::next(std::next(it)));
			continue;
		}
	}
}

void pruneUnreachableInstructions(Builder& b)
{
	std::set<unsigned> reachableLabels;

	if (b.onPacket())
		reachableLabels.insert(b.onPacket()->id());
	if (b.onPeriodic())
		reachableLabels.insert(b.onPeriodic()->id());
	if (b.onInit())
		reachableLabels.insert(b.onInit()->id());

	auto it = b.instructions().begin(), end = b.instructions().end();
	while (it != end) {
		if (!(*it)->label() || !reachableLabels.count((*it)->label()->id())) {
			auto dead = it++;
			b.erase(dead, it);
			continue;
		}

		for (; it != end; ++it) {
			if (auto* sw = dynamic_cast<const ImmediateInstruction<SwitchTable>*>(*it)) {
				for (auto& e : sw->immed())
					reachableLabels.insert(e.target.id());
				continue;
			}

			if (auto* j = dynamic_cast<const ImmediateInstruction<Label>*>(*it)) {
				reachableLabels.insert(j->immed().id());
				if (j->opcode() == Opcode::JUMP) {
					++it;
					break;
				}
				continue;
			}

			if ((*it)->opcode() == Opcode::RET) {
				++it;
				break;
			}
		}
	}
}

void moveJumpTargets(Builder& b)
{
	threadTrivialJumps(b);

	bool somethingChanged = false;
	do {
		std::map<unsigned, Builder::insn_iterator> lastJumpTo, labelPositions;

		somethingChanged = false;

		for (auto it = b.instructions().begin(), end = b.instructions().end(); it != end; ++it) {
			if ((*it)->label())
				labelPositions.insert({ (*it)->label()->id(), it });

			if (auto* j = dynamic_cast<const ImmediateInstruction<Label>*>(*it)) {
				lastJumpTo[j->immed().id()] = it;
				continue;
			}

			if (auto* sw = dynamic_cast<const ImmediateInstruction<SwitchTable>*>(*it)) {
				for (auto& e : sw->immed())
					lastJumpTo[e.target.id()] = it;
				continue;
			}
		}

		auto isTerminator = [] (Builder::insn_iterator it) {
			return (*it)->opcode() == Opcode::JUMP || (*it)->opcode() == Opcode::RET;
		};

		for (auto it = b.instructions().begin(), end = b.instructions().end(); it != end; ++it) {
			if ((*it)->opcode() != Opcode::JUMP)
				continue;

			auto& j = dynamic_cast<const ImmediateInstruction<Label>&>(**it);
			if (lastJumpTo.at(j.immed().id()) == it) {
				auto target = labelPositions.at(j.immed().id());

				if (!isTerminator(std::prev(target)))
					continue;

				++it;
				b.erase(std::prev(it), it);
				somethingChanged = true;

				if (target == it)
					continue;

				auto targetEnd = target;
				while (targetEnd != end && !isTerminator(targetEnd))
					++targetEnd;
				if (targetEnd != end)
					++targetEnd;

				b.moveRange(it, target, targetEnd);
			}
		}
	} while (somethingChanged);
}

void collapseTrivialJumps(Builder& b)
{
	moveJumpTargets(b);
	pruneUnreachableInstructions(b);
}

}
}
