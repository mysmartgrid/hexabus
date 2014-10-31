#ifndef INCLUDE_MC_PROGRAM_HPP_345A0781A78039CE
#define INCLUDE_MC_PROGRAM_HPP_345A0781A78039CE

#include "MC/instruction.hpp"
#include "Util/iterator.hpp"
#include "Util/range.hpp"

#include <vector>

namespace hbt {
namespace mc {

class InvalidProgram : public std::logic_error {
	private:
		std::string _extra;

	public:
		InvalidProgram(const std::string& what, const std::string& extra)
			: logic_error(what), _extra(extra)
		{}

		const std::string& extra() const { return _extra; }
};

class Program {
	friend class Builder;

	private:
		uint8_t _version;
		boost::optional<Label> _on_packet, _on_periodic, _on_init;
		std::vector<std::unique_ptr<Instruction>> _instructions;

	public:
		typedef util::const_uptr_value_iterator<decltype(_instructions)> iterator;

	public:
		template<typename It>
		Program(uint8_t version, boost::optional<Label> on_packet,
				boost::optional<Label> on_periodic, boost::optional<Label> on_init, It begin, It end)
			: _version(version), _on_packet(on_packet), _on_periodic(on_periodic), _on_init(on_init), _instructions(begin, end)
		{}

		uint8_t version() const { return _version; }
		const Label* onPacket() const { return _on_packet.get_ptr(); }
		const Label* onPeriodic() const { return _on_periodic.get_ptr(); }
		const Label* onInit() const { return _on_init.get_ptr(); }

		util::range<iterator> instructions() const { return _instructions; }
};

}
}

#endif
