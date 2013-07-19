#ifndef LIBHEXABUS_EID__REGISTRY_HPP
#define LIBHEXABUS_EID__REGISTRY_HPP 1

#include <stdint.h>
#include <string>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "../../../shared/hexabus_types.h"

namespace hexabus {

	class EndpointDescriptor {
		public:
			static const uint32_t read = 1;
			static const uint32_t write = 2;

		private:
			uint32_t _eid;
			std::string _description;
			boost::optional<std::string> _unit;
			hxb_datatype _type;
			uint32_t _access;

		public:
			EndpointDescriptor(uint32_t eid, const std::string& description,
					const boost::optional<std::string>& unit, hxb_datatype type, uint32_t access)
				: _eid(eid), _description(description), _unit(unit), _type(type), _access(access)
			{
			}

			uint32_t eid() const { return _eid; }

			const std::string& description() const { return _description; }

			const boost::optional<std::string>& unit() const { return _unit; }

			hxb_datatype type() const { return _type; }

			uint32_t access() const { return _access; }

			bool can_read() const { return _access & read; }

			bool can_write() const { return _access & write; }
	};



	class EndpointRegistry {
		private:
			std::map<uint32_t, EndpointDescriptor> _eids;
			boost::filesystem::path _path;

		public:
			static const char* default_path;

			EndpointRegistry();
			EndpointRegistry(const boost::filesystem::path& path);

			const boost::filesystem::path& path() const { return _path; }


			void reload();
	};

}

#endif
