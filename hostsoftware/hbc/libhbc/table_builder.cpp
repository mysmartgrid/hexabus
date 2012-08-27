#include "table_builder.hpp"

#include <libhbc/error.hpp>

using namespace hexabus;

struct table_builder : boost::static_visitor<> {
  table_builder(endpoint_table_ptr ept, device_table_ptr dt) : _e(ept), _d(dt) { }

  void operator()(endpoint_doc& ep) const {
    // TODO check if something is NOT specified! like an endpoint without a name!
    endpoint e;
    std::string name = ep.name;

    e.dtype = DT_UNDEFINED;
    e.read = e.write = e.broadcast = false;
    bool eid_read = false, dt_read = false, access_read = false;
    BOOST_FOREACH(endpoint_cmd_doc ep_cmd, ep.cmds) {
      switch(ep_cmd.which()) {
        case 0: // eid
          if(!eid_read) {
            e.eid = boost::get<ep_eid_doc>(ep_cmd).eid;
            eid_read = true;
          } else {
            std::ostringstream oss;
            oss << "[" << ep.lineno << "] Endpoint has multiple EID entries." << std::endl;
            throw DuplicateEntryException(oss.str());
          }
          break;
        case 1: // datatype
          if(!dt_read) {
            e.dtype = boost::get<ep_datatype_doc>(ep_cmd).dtype;
            dt_read = true;
          } else {
            std::ostringstream oss;
            oss << "[" << ep.lineno << "] Endpoint has multiple datatype entries." << std::endl;
            throw DuplicateEntryException(oss.str());
          }
          break;
        case 2: // access
          if(!access_read) {
            BOOST_FOREACH(access_level al, boost::get<ep_access_doc>(ep_cmd).access_levels) {
              switch(al) {
                case AC_READ:
                  e.read = true;
                  break;
                case AC_WRITE:
                  e.write = true;
                  break;
                case AC_BROADCAST:
                  e.broadcast = true;
                  break;

                default:
                  throw CaseNotImplementedException("not implemented");
              }
            }
            access_read = true; // do this here, so that it gets set to true only when an access level definition is read
          } else {
            std::ostringstream oss;
            oss << "[" << ep.lineno << "] Endpoint has multiple access level entries." << std::endl;
            throw DuplicateEntryException(oss.str());
          }
          break;

        default:
          throw CaseNotImplementedException("not implemented");
      }
    }

    if(!eid_read) {
      std::ostringstream oss;
      oss << "[" << ep.lineno << "] Endpoint definition has no endpoint ID." << std::endl;
      throw MissingEntryException(oss.str());
    }
    if(!dt_read) {
      std::ostringstream oss;
      oss << "[" << ep.lineno << "] Endpoint definition has no datatype." << std::endl;
      throw MissingEntryException(oss.str());
    }
    if(!access_read) {
      std::ostringstream oss;
      oss << "[" << ep.lineno << "] Endpoint definition has no access level." << std::endl;
      throw MissingEntryException(oss.str());
    }

    // check if it's already in there
    endpoint_table::iterator it = _e->find(name);
    if(it == _e->end()) // not already in there
      _e->insert(std::pair<std::string, endpoint>(name, e));
    else { // already in there
      std::ostringstream oss;
      oss << "[" << ep.lineno << "] Duplicate endpoint name in endpoint definition." << std::endl;
      throw DuplicateEntryException(oss.str());
      // TODO uh, do we also want to check for multiple instances of the same EID?
    }
  }

  void operator()(alias_doc& alias) const {
    device_alias dev;
    std::string name = alias.device_name;
    bool eid_list_read = false, ip_adr_read = false;
    BOOST_FOREACH(alias_cmd_doc cmd, alias.cmds) {
      switch(cmd.which()) {
        case 0: // ip address
          if(!ip_adr_read) {
            dev.ipv6_address = boost::asio::ip::address::from_string(boost::get<alias_ip_doc>(cmd).ipv6_address);
            // TODO catch exceptions
            ip_adr_read = true; // TODO only set to true if something useful was read
          }
          else {
            std::ostringstream oss;
            oss << "[" << alias.lineno << "] Duplicate IP address in alias definition." << std::endl;
            throw DuplicateEntryException(oss.str());
          }
          break;
        case 1: // eid list
          if(!eid_list_read)
          {
            dev.eids = boost::get<alias_eids_doc>(cmd).eid_list.eids;
            eid_list_read = true;
          } else {
            std::ostringstream oss;
            oss << "[" << alias.lineno << "] Duplicate endpoint list in alias definition." << std::endl;
            throw DuplicateEntryException(oss.str());
          }
          break;

        default:
          throw CaseNotImplementedException("unknown which (alias definition)");
      }
    }
    // check if an IP address is present
    if(!ip_adr_read) {
      std::ostringstream oss;
      oss << "[" << alias.lineno << "] Missing IP address in alias definition." << std::endl;
      throw MissingEntryException(oss.str());
    }
    // check if an EP list is present
    if(!eid_list_read) {
      std::ostringstream oss;
      oss << "[" << alias.lineno << "] Missing endpoint list in alias definition." << std::endl;
      throw MissingEntryException(oss.str());
    }
    // check for duplicate EIDs
    BOOST_FOREACH(unsigned int eid, dev.eids) {
      unsigned int occurances = 0;
      BOOST_FOREACH(unsigned int eid2, dev.eids) {
        if(eid == eid2)
          occurances++;
      }
      if(occurances > 1) {
        std::ostringstream oss;
        oss << "[" << alias.lineno << "] Duplicate entry in endpoint list in alias definition." << std::endl;
        throw DuplicateEntryException(oss.str());
      }
    }

    // check for duplicates
    device_table::iterator it = _d->find(name);
    if(it == _d->end()) // not already in the table
      _d->insert(std::pair<std::string, device_alias>(name, dev));
    else {
      std::ostringstream oss;
      oss << "[" << alias.lineno << "] Duplicate alias name in alias definition." << std::endl;
      throw DuplicateEntryException(oss.str());
    }
  }

  void operator()(statemachine_doc& statemachine) const { }
  void operator()(include_doc& include) const { }
  void operator()(module_doc& module) const { }

  endpoint_table_ptr _e;
  device_table_ptr _d;
};

void TableBuilder::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    // only state machines
    boost::apply_visitor(table_builder(_e, _d), block);
  }
}

void TableBuilder::print() {
  std::cout << "Endpoint table:" << std::endl;

  for(endpoint_table::iterator it = _e->begin(); it != _e->end(); it++) {
    std::cout << "Endpoint name: " << it->first << " - EID: " << it->second.eid << " - Datatype: ";
    switch(it->second.dtype) {
      case DT_UNDEFINED:
        std::cout << "undef.";
        break;
      case DT_BOOL:
        std::cout << "bool";
        break;
      case DT_UINT8:
        std::cout << "uint8";
        break;
      case DT_UINT32:
        std::cout << "uint32";
        break;
      case DT_FLOAT:
        std::cout << "float";
        break;
      default:
        std::cout << "not implemented...?";
    }
    std::cout << " - Access (rwb): " << it->second.read << it->second.write << it->second.broadcast << std::endl;
  }

  std::cout << std::endl << "Device alias table:" << std::endl;
  for(device_table::iterator it = _d->begin(); it != _d->end(); it++) {
    std::cout << "Name: " << it->first << " - IP Addr: " << it->second.ipv6_address.to_string() << " - EIDs: ";
    BOOST_FOREACH(unsigned int eid, it->second.eids) {
      std::cout << eid << " ";
    }
    std::cout << std::endl;
  }
}
