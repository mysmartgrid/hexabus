#ifndef LIBHEXABUS_ENDPOINT__REGISTRY_HPP
#define LIBHEXABUS_ENDPOINT__REGISTRY_HPP 1

#include <stdint.h>
#include <string>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <libhexabus/hexabus_types.h>

namespace hexabus {

	class EndpointDescriptor {
		public:
			enum Access {
				read = 1 << 0,
				write = 1 << 1
			};

			enum Function {
				sensor,
				actor,
				infrastructure
			};

		private:
			uint32_t _eid;
			std::string _description;
			boost::optional<std::string> _unit;
			hxb_datatype _type;
			Access _access;
			Function _function;

		public:
			EndpointDescriptor(uint32_t eid, const std::string& description,
					const boost::optional<std::string>& unit, hxb_datatype type, Access access,
					Function function)
				: _eid(eid), _description(description), _unit(unit), _type(type), _access(access),
				  _function(function)
			{
			}

			uint32_t eid() const { return _eid; }

			const std::string& description() const { return _description; }

			const boost::optional<std::string>& unit() const { return _unit; }

			hxb_datatype type() const { return _type; }

			Access access() const { return _access; }

			bool can_read() const { return _access & read; }

			bool can_write() const { return _access & write; }

			Function function() const { return _function; }
	};



	class EndpointRegistry {
		private:
			std::map<uint32_t, EndpointDescriptor> _eids;
			boost::filesystem::path _path;

		public:
			static const char* default_path;

			typedef std::map<uint32_t, EndpointDescriptor>::const_iterator const_iterator;

			EndpointRegistry();
			EndpointRegistry(const boost::filesystem::path& path);

			const boost::filesystem::path& path() const { return _path; }

			const_iterator begin() const;
			const_iterator end() const;

			const_iterator find(uint32_t eid) const;
			const EndpointDescriptor& lookup(uint32_t eid) const;

			void reload();
	};



	inline EndpointDescriptor::Access operator|(EndpointDescriptor::Access a,
			EndpointDescriptor::Access b)
	{
		return EndpointDescriptor::Access(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}
	inline EndpointDescriptor::Access operator&(EndpointDescriptor::Access a,
			EndpointDescriptor::Access b)
	{
		return EndpointDescriptor::Access(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}
	inline EndpointDescriptor::Access operator^(EndpointDescriptor::Access a,
			EndpointDescriptor::Access b)
	{
		return EndpointDescriptor::Access(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
	}
	inline EndpointDescriptor::Access operator|=(EndpointDescriptor::Access& a,
			EndpointDescriptor::Access b)
	{
		return a = a | b;
	}
	inline EndpointDescriptor::Access operator&=(EndpointDescriptor::Access& a,
			EndpointDescriptor::Access b)
	{
		return a = a & b;
	}
	inline EndpointDescriptor::Access operator^=(EndpointDescriptor::Access& a,
			EndpointDescriptor::Access b)
	{
		return a = a ^ b;
	}

}

#endif
