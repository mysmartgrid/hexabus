#include "table_builder.hpp"

using namespace hexabus;

struct table_builder : boost::static_visitor<> {
  table_builder(endpoint_table_ptr ept) : _e(ept) { }

  void operator()(endpoint_doc& ep) const {
    // TODO check if already exists. this is an error, at least if it's a contradiction.
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

  void operator()(statemachine_doc& statemachine) const { }
  void operator()(include_doc& include) const { }
  void operator()(alias_doc alias) const { }
  void operator()(module_doc module) const { }

  endpoint_table_ptr _e;
};

void TableBuilder::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    // only state machines
    boost::apply_visitor(table_builder(_e), block);
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
    std::cout << " - Acces (rwb): " << ep.read << ep.write << ep.broadcast << std::endl;
  }
}
