#include "types_io.hpp"

#include <sstream>

#include <boost/format.hpp>

using boost::format;

std::string format_mac(uint64_t addr)
{
	uint8_t mac[8];

	memcpy(mac, &addr, 8);
	return (format("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x")
		% int(mac[0]) % int(mac[1]) % int(mac[2]) % int(mac[3])
		% int(mac[4]) % int(mac[5]) % int(mac[6]) % int(mac[7])).str();
}

std::string bitlist(uint32_t bits)
{
	std::stringstream ss;

	for (int i = 0; bits != 0; i++, bits >>= 1) {
		if (ss.str().size())
			ss << " ";
		if (bits & 1)
			ss << i;
	}

	return ss.str();
}



std::string print_tabular(const KeyLookupDescriptor& key, int indent)
{
	std::stringstream os;

	std::string prefix(indent, '\t');

	os << format(prefix + "Mode %1%") % int(key.mode());

	if (key.mode() != 0) {
		os << std::endl;

		if (key.mode() == 2) {
			uint32_t source = boost::get<uint32_t>(key.source());
			uint16_t pan = source >> 16;
			uint16_t saddr = source & 0xFFFF;

			os << format(prefix + "Source %|04x|:%|04x|") % pan % saddr << std::endl;
		} else if (key.mode() == 3) {
			uint64_t source = boost::get<uint64_t>(key.source());

			os << format(prefix + "Source %1%") % format_mac(source) << std::endl;
		}

		os << format(prefix + "Index %1%") % int(key.id());
	}

	return os.str();
}

std::string print_tabular(const Key& key)
{
	std::stringstream os;

	os << "Key" << std::endl
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

std::string print_tabular(const NetDevice& dev)
{
	std::stringstream os;

	os << format("Interface %1%") % dev.name() << std::endl
		<< format("	Phy %1%") % dev.phy() << std::endl;

	if (dev.has_addrs()) {
		os << format("	PAN ID %|04x|") % dev.pan_id() << std::endl
			<< format("	Short address %|04x|") % dev.short_addr() << std::endl
			<< format("	Extended address %1%") % format_mac(dev.addr()) << std::endl;
	}

	if (dev.has_macparams()) {
		os << format("	Transmit power %1%dBm") % dev.txpower() << std::endl
			<< format("	LBT %1%") % (dev.lbt() ? "on" : "off") << std::endl
			<< format("	CCA mode %|i|") % int(dev.cca_mode()) << std::endl
			<< format("	CCA ED level %1%dBm") % dev.ed_level() << std::endl;
	}

	return os.str();
}

std::string print_tabular(const Device& dev)
{
	std::stringstream os;

	os << "Device" << std::endl
		<< format("	PAN ID %|04x|") % dev.pan_id() << std::endl
		<< format("	Short address %|04x|") % dev.short_addr() << std::endl
		<< format("	Extended address %1%") % format_mac(dev.hwaddr()) << std::endl
		<< format("	Frame counter %1%") % dev.frame_ctr() << std::endl
		<< format("	Security override %1%") % (dev.sec_override() ? "on" : "off") << std::endl
		<< format("	Keying mode %1%") % int(dev.key_mode()) << std::endl;

	return os.str();
}

std::string print_tabular(const Phy& phy)
{
	std::stringstream os;

	os << format("Phy %1%") % phy.name() << std::endl
		<< format("	Channel %1%") % phy.channel() << std::endl
		<< format("	Page %1%") % phy.page() << std::endl;

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
	std::stringstream os;

	os << "Security level" << std::endl
		<< format("	Frame type %|i|") % sl.frame_type() << std::endl
		<< format("	Levels %1%") % bitlist(sl.levels()) << std::endl;

	return os.str();
}

std::string print_tabular(const SecurityParameters& params)
{
	std::stringstream os;

	os << "Security parameters" << std::endl;
	if (params.enabled()) {
		os << format("	Outgoing level %1%") % int(params.out_level()) << std::endl
			<< format("	Outgoing key") << std::endl
			<< print_tabular(params.out_key(), 2) << std::endl
			<< format("	Frame counter %1%") % params.frame_counter() << std::endl;
	} else {
		os << "	Disabled" << std::endl;
	}

	return os.str();
}
