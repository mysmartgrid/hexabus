#include "table_builder.hpp"

using namespace hexabus;

struct table_builder : boost::static_visitor<> {
  table_builder(endpoint_table_ptr ept, device_table_ptr dt) : _e(ept), _d(dt) { }

  void operator()(endpoint_doc& ep) const {
    // TODO check if something is NOT specified! like an endpoint without a name!
    // check if EID already exists in table
    bool exists = false;
    BOOST_FOREACH(endpoint tep, *_e) {
      if(tep.eid == ep.eid)
        exists = true;
    }
    if(exists) {
      std::cout << "Duplicate endpoint ID in line " << ep.lineno << std::endl;
      // TODO throw something
    } else {
      endpoint e;
      e.eid = ep.eid;

      e.name = "";
      e.dtype = DT_UNDEFINED;
      e.read = e.write = e.broadcast = false;
      BOOST_FOREACH(endpoint_cmd_doc ep_cmd, ep.cmds) {
        switch(ep_cmd.which()) {
          case 0: // name
            if(e.name == "")
              e.name = boost::get<ep_name_doc>(ep_cmd).name;
            else {
              std::cout << "Duplicate endpoint name in line " << ep.lineno << std::endl;
              // TODO throw something
            }
            break;
          case 1: // datatype
            if(e.dtype == DT_UNDEFINED)
              e.dtype = boost::get<ep_datatype_doc>(ep_cmd).dtype;
            else {
              std::cout << "Duplicate datatype in line " << ep.lineno << std::endl;
              // TODO throw something
            }
            break;
          case 2: // access
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
                  std::cout << "not implemented!??" << std::endl;
              }
            }
            break;

          default:
            std::cout << "not implemented?!" << std::endl;
        }
      }

      _e->push_back(e);
    }
  }

  void operator()(alias_doc& alias) const {
    // TODO check if alias name already exists
    bool exists = false;
    BOOST_FOREACH(device_alias da, *_d) {
      if(da.name == alias.device_name)
        exists = true;
    }

    if(exists) {
      std::cout << "Duplicate device alias in line " << alias.lineno << std::endl;
      // TODO throw something
    } else {
      device_alias dev;
      dev.name = alias.device_name;
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
              std::cout << "Duplicate IP address in line " << alias.lineno << std::endl;
              // TODO throw something
            }
            break;
          case 1: // eid list
            if(!eid_list_read)
            {
              dev.eids = boost::get<alias_eids_doc>(cmd).eid_list.eids;
              eid_list_read = true;
            } else {
              std::cout << "Duplicate EID list entry in line " << alias.lineno << std::endl;
              // TODO throw something
            }
            break;

          default:
            std::cout << "unknown which()" << std::endl;
        }
      }
      // check if an IP address is present
      if(!ip_adr_read) {
        std::cout << "Missing IP address in line " << alias.lineno << std::endl;
        // TODO throw something
      }
      // check for duplicate EIDs
      BOOST_FOREACH(unsigned int eid, dev.eids) {
        unsigned int occurances = 0;
        BOOST_FOREACH(unsigned int eid2, dev.eids) {
          if(eid == eid2)
            occurances++;
        }
        if(occurances > 1) {
          std::cout << "Duplicate EID entry in alias definition in line " << alias.lineno << std::endl;
          // TODO throw something
        }
      }

      _d->push_back(dev);
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

  BOOST_FOREACH(endpoint ep, *_e) {
    std::cout << "EID: " << ep.eid << " - Name: " << ep.name << " - Datatype: ";
    switch(ep.dtype) {
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
    std::cout << " - Access (rwb): " << ep.read << ep.write << ep.broadcast << std::endl;
  }

  std::cout << std::endl << "Device alias table:" << std::endl;
  BOOST_FOREACH(device_alias dev, *_d) {
    std::cout << "Name: " << dev.name << " - IP Addr: " << dev.ipv6_address.to_string() << " - EIDs: ";
    BOOST_FOREACH(unsigned int eid, dev.eids) {
      std::cout << eid << " ";
    }
    std::cout << std::endl;
  }
}
