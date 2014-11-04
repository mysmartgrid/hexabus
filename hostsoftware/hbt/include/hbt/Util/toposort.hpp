#ifndef TOPOSORT__HPP_96CA750AD6BE30F9
#define TOPOSORT__HPP_96CA750AD6BE30F9

#include <list>
#include <map>
#include <set>

namespace hbt {
namespace util {

template<typename Range, typename Extract, typename Preds>
inline auto topoSort(Range&& r, Extract e, Preds predecessorsOf)
	-> std::list<decltype(e(*r.begin()))>
{
	typedef decltype(e(*r.begin())) T;

	std::map<T, std::set<T>> predecessors, successors;
	std::list<T> queue, result;

	for (const auto& item : r) {
		auto key = e(item);
		auto preds = predecessorsOf(key);
		for (auto& pred : preds)
			successors[pred].insert(key);
		if (predecessors.emplace(std::move(key), std::move(preds)).first->second.empty())
			queue.push_back(key);
	}

	while (!queue.empty()) {
		auto& item = queue.front();

		result.push_back(item);
		for (auto& suc : successors[item]) {
			auto& node = predecessors[suc];

			if (node.empty())
				continue;

			node.erase(item);
			if (node.empty())
				queue.push_back(suc);
		}

		queue.pop_front();
	}

	return std::move(result);
}


}
}

#endif
