#include "types_io.hpp"

#include <sstream>

#include <boost/format.hpp>

std::string format_mac(uint64_t addr)
{
	uint8_t mac[8];

	memcpy(mac, &addr, 8);
	return (boost::format("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x")
		% mac[0] % mac[1] % mac[2] % mac[3]
		% mac[4] % mac[5] % mac[6] % mac[7]).str();
}

std::string bitlist(uint32_t bits)
{
	std::stringstream ss;

	for (int i = 0; bits != 0; i++, bits >>= 1) {
		if (i)
			ss << " ";
		if (bits & 1)
			ss << i;
	}

	return ss.str();
}



std::string print_tabular(const KeyLookupDescriptor& key, int indent)
{
	using boost::format;
	std::stringstream os;

	std::string prefix(indent, '\t');

	os << format(prefix + "Mode %1%") % int(key.mode()) << std::endl;

	if (key.mode() != 0) {
		if (key.mode() == 2) {
			uint32_t source = boost::get<uint32_t>(key.source());
			uint16_t pan = source >> 16;
			uint16_t saddr = source & 0xFFFF;

			os << format(prefix + "Source %|04x|:%|04x|") % pan % saddr << std::endl;
		} else if (key.mode() == 3) {
			uint64_t source = boost::get<uint64_t>(key.source());

			os << format(prefix + "Source %1%") % format_mac(source) << std::endl;
		}

		os << format(prefix + "Index %1%") % int(key.id()) << std::endl;
	}

	return os.str();
}

std::string print_tabular(const Key& key)
{
	using boost::format;
	std::stringstream os;

	os << format("Key on %1%") % key.iface() << std::endl
		<< print_tabular(key.lookup_desc(), 1) << std::endl;

	os << format("	Frame types %1%") % bitlist(key.frame_types()) << std::endl;
	if (key.frame_types() & (1 << 3))
		os << format("	Command frames %1%") % bitlist(key.cmd_frame_ids()) << std::endl;

	os << "	Key";
	for (int i = 0; i < 16; i++) {
		os << format(" %|02x|") % int(key.key()[i]);
	}
	os << std::endl;

	return os.str();
}

std::string print_tabular(const Device& dev)
{
	using boost::format;
	std::stringstream os;

	os << format("Device on %1%") % dev.iface() << std::endl
		<< format("	PAN ID %|04x|") % dev.pan_id() << std::endl
		<< format("	Short address %|04x|") % dev.short_addr() << std::endl
		<< format("	Extended address %1%") % format_mac(dev.hwaddr()) << std::endl
		<< format("	Frame counter %1%") % dev.frame_ctr() << std::endl
		<< "	Key:" << std::endl
		<< print_tabular(dev.key(), 2) << std::endl;

	return os.str();
}

std::string print_tabular(const Phy& phy)
{
	using boost::format;
	std::stringstream os;

	os << format("PHY %1%") % phy.name() << std::endl
		<< format("	Channel %1%") % phy.channel() << std::endl
		<< format("	Page %1%") % phy.page() << std::endl
		<< format("	Transmit power %1%") % phy.txpower() << std::endl
		<< format("	LBT %1%") % (phy.lbt() ? "on" : "off") << std::endl
		<< format("	CCA mode %|i|") % phy.cca_mode() << std::endl
		<< format("	CCA ED level %1%") % phy.ed_level() << std::endl;

	typedef std::vector<uint32_t>::const_iterator iter;
	int i = 0;
	for (iter it = phy.supported_pages().begin(),
		end = phy.supported_pages().end();
	     it != end;
	     it++, i++) {
		int page = *it;
		if (page != 0) {
			os << format("	Channels on page %1%: %2%") % i % bitlist(page) << std::endl;
		}
	}

	return os.str();
}

std::string print_tabular(const Seclevel& sl)
{
	using boost::format;
	std::stringstream os;

	os << format("Security lveel on %1%") % sl.iface() << std::endl
		<< format("	Frame type %|i|") % sl.frame_type() << std::endl
		<< format("	Levels %1%") % bitlist(sl.levels()) << std::endl;

	return os.str();
}
