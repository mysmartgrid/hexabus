#include "hba_output.hpp"

#include <libhbc/error.hpp>
#include <iomanip>
#include <libhbc/hashlib2plus/hashlibpp.h>

using namespace hexabus;

void HBAOutput::operator()(std::ostream& ostr) {
  // before running this, graph_transformation has done its work and has left us with a SINGLE state machine in the graph.

  // TODO This code is very ugly.
  //      It should be re-implemented, using a visitor pattern
  //      or at least different methods for printing things, not a long spaghetti.

  hexabus::VersionInfo vinf;
  ostr << "# Auto-generated by Hexabus Compiler version " << vinf.getVersion() << "." << std::endl << std::endl;

  // find out target IP address
  device_table::iterator dt_it = _d->find(_dev_name);
  if(dt_it == _d->end()) {
    std::ostringstream oss;
    oss << "Device name " << _dev_name << " not found in device table. Aborting." << std::endl;
    throw HBAConversionErrorException(oss.str());
  }

  ostr << "target ";
  print_ipv6address(dt_it->second.ipv6_address, ostr);
  ostr << ";" << std::endl << std::endl;

  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));

  // Find index of init state
  for(; vertexIt != vertexEnd; vertexIt++) {
    std::string v_name = (*_g)[*vertexIt].name;
    if(v_name.length() > 5) { // make sure it's long enough to contain the substring
      if(v_name.substr(v_name.length() - 5) == ".init") {
        unsigned int init_v_id = (*_g)[*vertexIt].vertex_id;
        unsigned int init_m_id = (*_g)[*vertexIt].machine_id;

        // Generate Machine ID
        md5wrapper md5;
        std::string v_machine_name = (*_g)[*vertexIt].name.substr((*_g)[*vertexIt].name.find_last_of(' ') + 1); // finding out the state machine name from the vertex name
        v_machine_name = v_machine_name.substr(2, v_machine_name.find_first_of('.') - 2); // name starts at 2, because it is lead by "\n"

        ostr << "machine " << md5.getHashFromString(v_machine_name) << ";" << std::endl << std::endl;
        ostr << "startstate state_" << init_m_id << "_" << init_v_id << ";" << std::endl << std::endl;
      }
    }
  } // there must be an init state. this is checked when constructing the graph

  // iterate over vertices, find condition vertices and create condition blocks for them.
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt) {
    vertex_id_t vertexID = *vertexIt;
    vertex_t& vertex = (*_g)[vertexID];

    if (vertex.type == v_cond) {
      try {
        condition_doc cond = boost::get<condition_doc>(vertex.contents);

        switch(cond.which()) {
          case 0: // unsigned int, "true" condition
            // we do not need a condition table entry for "true" -- it is given by index 255
            break;
          case 1: // atomic condition
            {
              atomic_condition_doc at_cond = boost::get<atomic_condition_doc>(cond);
              print_condition(at_cond, ostr, vertex);
            }
            break;
          case 2: // timeout condition
            {
              timeout_condition_doc to_cond = boost::get<timeout_condition_doc>(cond);
              print_condition(to_cond, ostr, vertex);
            }
            break;
          case 3: // timer condition
            {
              timer_condition_doc tm_cond = boost::get<timer_condition_doc>(cond);
              print_condition(tm_cond, ostr, vertex);
            }
            break;
          case 4: // compound_condition
            {
              std::ostringstream oss;
              oss << "Compound condition in HBA Conversion stage. Cannot convert to HBA code." << std::endl;
              HBAConversionErrorException(oss.str());
            }
        }
      } catch(boost::bad_get e) {
        std::ostringstream oss;
        oss << "Condition vertex does not contain condition data!" << std::endl;
        throw HBAConversionErrorException(oss.str());
      }
    }
  }

  // iterate over vertices again, find state vertices and create state blocks for them.
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt) {
    vertex_id_t vertexID = *vertexIt;
    vertex_t & vertex = (*_g)[vertexID];

    if(vertex.type == v_state) {
      // state name
      ostr << "state state_" << vertex.machine_id << "_" << vertex.vertex_id << " {" << std::endl;

      // if-blocks in the state
      graph_t::out_edge_iterator outEdgeIt, outEdgeEnd;
      boost::tie(outEdgeIt, outEdgeEnd) = out_edges(vertexID, (*_g));
      for(; outEdgeIt != outEdgeEnd; outEdgeIt++) {
        edge_id_t edgeID = *outEdgeIt;
        // edge_t& edge = (*_g)[edgeID];

        // "edge" now connects our state to an if-block.
        // get the condition and the command block from there
        vertex_id_t if_vertexID = edgeID.m_target;
        vertex_t& if_vertex = (*_g)[if_vertexID];

        if(if_vertex.type != v_cond)
          throw HBAConversionErrorException("Condition vertex expected. Other vertex type found (HBA output generation stage)");

        if(if_vertex.contents.which() == 0 && boost::get<condition_doc>(if_vertex.contents).which() == 0) // If it is a Condition AND it is unsigned int ("true condition")...
          ostr << "  if true {" << std::endl;
        else
          ostr << "  if " << "cond_" << if_vertex.machine_id << "_" << if_vertex.vertex_id << " {" << std::endl;

        // now find the command block connected to the if block
        // TODO make sure each if-block has exactly one command block, and the command block has exactly one command
        // (do this when/after slicing the program
        graph_t::out_edge_iterator commandEdgeIt, commandEdgeEnd;
        boost::tie(commandEdgeIt, commandEdgeEnd) = out_edges(if_vertexID, (*_g));

        edge_id_t commandID = *commandEdgeIt;
        // edge_t& command_edge = (*_g)[commandID];
        vertex_id_t command_vertexID = commandID.m_target;
        vertex_t& command_vertex = (*_g)[command_vertexID];

        if(command_vertex.type != v_command)
          throw HBAConversionErrorException("Command vertex expected. Other vertex type found (HBA output generation stage)");

        // find the write command
        try {
          command_block_doc command = boost::get<command_block_doc>(command_vertex.contents);
          size_t n_write = command.commands.size();
          unsigned int eid = 0;
          float value = 0;
          if(n_write == 0) { // empty command - add a "do nothing" set command
            // write to EID 0 is interpreted as "write nothing" command.
            eid = 0;
            value = 0;
          } else if(n_write == 1) { // just what we want
            write_command_doc& write_cmd = command.commands[0].write_command; // TODO make sure there is exactly one.

            // TODO
            // Here some checks could be done:
            // Is the endpoint we are writing to write-able?
            // Does the endpoint on whose data we rely in the if-clause broadcast?
            // Do the datatypes fit?
            // Are we creating non-leavable states (should be checked earlier though)?
            // ...

            // now find the eid
            try {
              std::string epname = boost::get<std::string>(write_cmd.geid.endpoint_name);
              endpoint_table::iterator ep_it = _e->find(epname);
              if(ep_it == _e->end()) {
                std::ostringstream oss;
                std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(command_vertex.machine_id);
                if(it != machine_filenames_per_id.end())
                  oss << "[" << it->second << ":" << write_cmd.lineno << "] ";
                oss << "Endpoint name not found: " << boost::get<std::string>(write_cmd.geid.endpoint_name) << std::endl;
                throw HBAConversionErrorException(oss.str());
              }

              // if endpoint is not writeable, throw an exception
              if(ep_it->second.write == false) {
                std::ostringstream oss;
                std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(command_vertex.machine_id);
                if(it != machine_filenames_per_id.end())
                  oss << "[" << it->second << ":" << write_cmd.lineno << "] ";
                oss << "Endpoint '" << boost::get<std::string>(write_cmd.geid.endpoint_name) << "' is not writeable." << std::endl;
                throw EndpointNotWriteableException(oss.str());
              }

              eid = ep_it->second.eid;
            } catch (boost::bad_get e) {
              std::ostringstream oss;
              std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(command_vertex.machine_id);
              if(it != machine_filenames_per_id.end())
                oss << "[" << it->second << ":" << write_cmd.lineno << "] ";
              oss << "Only literal endpoint names allowed in state machine definition." << std::endl;
              throw HBAConversionErrorException(oss.str());
            }

            // now find the value
            try {
              value = boost::get<float>(write_cmd.constant);
            } catch(boost::bad_get e) {
              std::ostringstream oss;
              std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(command_vertex.machine_id);
              if(it != machine_filenames_per_id.end())
                oss << "[" << it->second << ":" << write_cmd.lineno << "] ";
              oss << "Only literal constants allowed in state machine definition. (At the moment only float works)" << std::endl;
              throw HBAConversionErrorException(oss.str());
            }
          } else // too many... TODO throw something!
          { }

          // output the "set" line
          ostr << "    set " << eid << " := " << value << ";" << std::endl;
        } catch(boost::bad_get e) {
          // TODO this is a hexabus-compiler-has-a-bug error
          throw HBAConversionErrorException("Command vertex does not contain commamnd data!");
        }

        // now follow the edge to the target state vertex
        graph_t::out_edge_iterator targetEdgeIt, targetEdgeEnd;
        boost::tie(targetEdgeIt, targetEdgeEnd) = out_edges(command_vertexID, (*_g));

        // TODO make sure we have exactly one...
        edge_id_t targetID = *targetEdgeIt;
        vertex_id_t target_state_vertexID = targetID.m_target;
        vertex_t& target_state_vertex = (*_g)[target_state_vertexID];

        if(target_state_vertex.type != v_state) // TODO this is probably a bug in hexbaus compiler
          throw HBAConversionErrorException("State vertex expected. Got other vertex type.");

        // print goodstate / badstate line
        ostr << "    goodstate " << "state_" << target_state_vertex.machine_id << "_" << target_state_vertex.vertex_id << ";" << std::endl;
        // TODO for now, badstate = goodstate
        ostr << "    badstate " << "state_" << target_state_vertex.machine_id << "_" << target_state_vertex.vertex_id << ";" << std::endl;

        // closing bracket
        ostr << "  }" << std::endl;
      }

      // closing bracket
      ostr << "}" << std::endl << std::endl;
    }
  }
}

void HBAOutput::print_condition(atomic_condition_doc at_cond, std::ostream& ostr, vertex_t& vertex) {
  // give the condition a name
  ostr << "condition cond_" << vertex.machine_id << "_" << vertex.vertex_id << " {" << std::endl;

  ostr << "  ip := ";

  // find the device alias, print its IP address
  if (_dev_name == boost::get<std::string>(at_cond.geid.device_alias)) { // If we are ON the device that broadcasts the message, we should use "localhost"
    ostr << "0000:0000:0000:0000:0000:0000:0000:0001"; // ::1 = localhost
  } else { // otherwise we should use the IP address of the device which broadcasts the message
    try {
      device_table::iterator d_it = _d->find(boost::get<std::string>(at_cond.geid.device_alias));
      if(d_it == _d->end()) {
        std::ostringstream oss;
        std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(vertex.machine_id);
        if(it != machine_filenames_per_id.end())
          oss << "[" << it->second << ":" << at_cond.geid.lineno << "] ";
        oss << "Device name not found: " << boost::get<std::string>(at_cond.geid.device_alias);
        throw HBAConversionErrorException(oss.str());
      }

      print_ipv6address(d_it->second.ipv6_address, ostr);

    } catch(boost::bad_get e) {
      std::ostringstream oss;
      std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(vertex.machine_id);
      if(it != machine_filenames_per_id.end())
        oss << "[" << it->second << ":" << at_cond.geid.lineno << "] ";
      oss << "Only literal device names (no placeholders) allowed in state machine definition!" << std::endl;
      throw HBAConversionErrorException(oss.str());
    }
  }

  ostr << ";" << std::dec << std::endl;

  // find the endpoint name, print its IP address
  try {
    endpoint_table::iterator e_it = _e->find(boost::get<std::string>(at_cond.geid.endpoint_name));
    if(e_it == _e->end()) {
      std::ostringstream oss;
      std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(vertex.machine_id);
      if(it != machine_filenames_per_id.end())
        oss << "[" << it->second << ":" << at_cond.geid.lineno << "] ";
      oss << "Endpoint name not found: " << boost::get<std::string>(at_cond.geid.endpoint_name) << std::endl;
      throw HBAConversionErrorException(oss.str());
    }


    // throw an exception if an endpoint is not broadcast!
    if(e_it->second.broadcast == false) {
      std::ostringstream oss;
      std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(vertex.machine_id);
      if(it != machine_filenames_per_id.end())
        oss << "[" << it->second << ":" << at_cond.geid.lineno << "] ";
      oss << "Endpoint '" << boost::get<std::string>(at_cond.geid.endpoint_name) << "' is not broadcast." << std::endl;
      throw EndpointNotBroadcastException(oss.str());
    }

    ostr << "  eid := " << e_it->second.eid << ";" << std::endl;

  } catch(boost::bad_get e) {
    std::ostringstream oss;
    std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find(vertex.machine_id);
    if(it != machine_filenames_per_id.end())
      oss << "[" << it->second << ":" << at_cond.geid.lineno << "] ";
    oss << "Only literal endpoint names (no placeholders) allowed in state machine definition!" << std::endl;
    throw HBAConversionErrorException(oss.str());
  }

  // value comparison...
  ostr << "  value ";

  // comparison operator
  switch(at_cond.comp_op) {
    case STM_COMP_EQ:
      ostr << "==";
      break;
    case STM_COMP_LEQ:
      ostr << "<=";
      break;
    case STM_COMP_GEQ:
      ostr << ">=";
      break;
    case STM_COMP_LT:
      ostr << "<";
      break;
    case STM_COMP_GT:
      ostr << ">";
      break;
    case STM_COMP_NEQ:
      ostr << "!=";
      break;
    default:
      throw HBAConversionErrorException("operator not implemeted");
      break;
  }

  // constant
  try {
    ostr << " " << boost::get<float>(at_cond.constant) << ";" << std::endl;;
  } catch(boost::bad_get e) {
    // TODO this is an error in the input file
    std::ostringstream oss;
    oss << "Only literal constants (no placeholders) allowed in state machine definition!" << std::endl;
    oss << "(At the moment, only float is allowed!)" << std::endl; // TODO !!!
    throw HBAConversionErrorException(oss.str());
  }

  // closing bracket
  ostr << "}" << std::endl << std::endl;
}


void HBAOutput::print_condition(timeout_condition_doc to_cond, std::ostream& ostr, vertex_t& vertex) {
  // condition name
  ostr << "condition cond_" << vertex.machine_id << "_" << vertex.vertex_id << " {" << std::endl;

  // timeout
  try {
    ostr << "  timeout := " << boost::get<unsigned int>(to_cond.seconds) << ";" << std::endl;
  } catch(boost::bad_get e) {
    std::ostringstream oss;
    oss << "Only integer constants allowed for timeout values - must be Uint format." << std::endl;
    throw HBAConversionErrorException(oss.str());
  }

  // closing bracket
  ostr << "}" << std::endl << std::endl;

}

void HBAOutput::print_condition(timer_condition_doc tm_cond, std::ostream& ostr, vertex_t& vertex) {
  // name
  ostr << "condition cond_" << vertex.machine_id << "_" << vertex.vertex_id << " {" << std::endl;

  // field of date-time-structure we want to check
  switch(tm_cond.fields) {
    case TF_HOUR:    ostr << "  hour";    break;
    case TF_MINUTE:  ostr << "  minute";  break;
    case TF_SECOND:  ostr << "  second";  break;
    case TF_DAY:     ostr << "  day";     break;
    case TF_MONTH:   ostr << "  month";   break;
    case TF_YEAR:    ostr << "  year";    break;
    case TF_WEEKDAY: ostr << "  weekday"; break;
  }
  // operator
  switch(tm_cond.op) {
    case STM_COMP_LT: ostr << " before "; break;
    case STM_COMP_GT: ostr << " after ";  break;
    default: break;
  }

  // value to compare to
  try {
    ostr << boost::get<unsigned int>(tm_cond.value) << ";" << std::endl;
  } catch(boost::bad_get e) {
    std::ostringstream oss;
    oss << "Only integer constants allowed for timer values - must be Uint format." << std::endl;
    throw HBAConversionErrorException(oss.str());
  }

  ostr << "}" << std::endl << std::endl;
}

void HBAOutput::print_ipv6address(boost::asio::ip::address_v6 addr, std::ostream& ostr) {
  boost::array<unsigned char, 16> addr_bytes = addr.to_bytes();
  for(unsigned int i = 0; i < 16; i++) {
    if(!(i % 2) && i)
      ostr<<":";
    ostr << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)addr_bytes[i];
  }
}
