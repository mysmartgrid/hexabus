#include "generator_flash.hpp"
#include <boost/foreach.hpp>
#include <string>
#include <sstream>
#include <map>
#include <iomanip> // for hexadecimal output TODO probably not needed later!
#include <libhba/hba_datatypes.hpp>
#include "../../../shared/hexabus_statemachine_structs.h"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

using namespace hexabus;

const char COND_TABLE_BEGIN_TOKEN='-';
const char COND_TABLE_SEPARATOR='.';
const char COND_TABLE_END_TOKEN = '.';
const char STATE_TABLE_BEGIN_TOKEN='-';
const char STATE_TABLE_SEPARATOR='.';
const char STATE_TABLE_END_TOKEN = '.';

typedef std::map<unsigned int, std::string> flash_format_map_t;
typedef std::map<unsigned int, std::string>::iterator flash_format_map_it_t;
typedef std::map<unsigned int, struct transition> flash_format_state_map_t;
typedef std::map<unsigned int, struct transition>::iterator flash_format_state_map_it_t;
typedef std::map<unsigned int, struct condition> flash_format_cond_map_t;
typedef std::map<unsigned int, struct condition>::iterator flash_format_cond_map_it_t;

// TODO: States am Ende so sortieren, dass der Startstate der erste ist.

unsigned int state_index = 0; // TODO make this a little more elegant

struct hba_doc_visitor : boost::static_visitor<>
{
  hba_doc_visitor(
      flash_format_map_t& states,
      flash_format_state_map_t& states_bin,
      flash_format_map_t& conditions,
      flash_format_cond_map_t& conditions_bin,
      const graph_t_ptr g,
      Datatypes* datatypes)
    : _states(states),
      _states_bin(states_bin),
      _conditions(conditions),
      _conditions_bin(conditions_bin),
      _g(g),
      _datatypes(datatypes)
  { }

  void operator()(if_clause_doc const& clause, unsigned int state_id, unsigned int state_index) const
  {
    unsigned int cond_id = find_vertex_id(_g, clause.name);
    unsigned int goodstate_id = find_vertex_id(_g, clause.goodstate);
    unsigned int badstate_id = find_vertex_id(_g, clause.badstate);

    std::cout << state_id << ": clause " << clause.name
      << " (id " << cond_id << ")" << std::endl;
    std::cout << " set eid " << clause.eid
      << " := " << clause.value << " datatype " << (unsigned int)_datatypes->getDatatype(clause.eid) << std::endl;
    std::cout << " goodstate: " << goodstate_id << std::endl;
    std::cout << " badstate: " << badstate_id << std::endl;

    // construct webinterface representation (string)
    std::ostringstream oss;
    oss
      << state_id << STATE_TABLE_SEPARATOR
      << cond_id << STATE_TABLE_SEPARATOR
      << clause.eid << STATE_TABLE_SEPARATOR
      << (unsigned int)_datatypes->getDatatype(clause.eid) << STATE_TABLE_SEPARATOR
      << clause.value << STATE_TABLE_SEPARATOR // TODO find out what is correct in the webinterface "spec"!
      << goodstate_id << STATE_TABLE_SEPARATOR
      << badstate_id << STATE_TABLE_SEPARATOR
      ;
    // _states.insert(std::pair<unsigned int, std::string>(state_id, oss.str()));
    _states.insert(std::pair<unsigned int, std::string>(state_index, oss.str()));

    // construct binary representation
    struct transition t;
    t.fromState = state_id; // TODO need to cast here? // beter: Make sure only valid IDs are generated
    t.cond = cond_id;
    t.eid = clause.eid;
    t.goodState = goodstate_id;
    t.badState = badstate_id;
    t.value.datatype = _datatypes->getDatatype(clause.eid);
    memset(t.value.data, 0, sizeof(t.value.data)); // most of the time only four bytes needes, so keep the rest at 0
    std::stringstream ss;
    ss << std::hex << clause.value;
    if(_datatypes->getDatatype(clause.eid) == HXB_DTYPE_FLOAT) // TODO implement ALL the datatypes
    {
      ss >> *(float*)t.value.data;
    } else { // TODO for now, act as if everything were uint32
      ss >> std::dec >> *(uint32_t*)t.value.data;
    }
    // _states_bin.insert(std::pair<unsigned int, struct transition>(state_id, t));
    _states_bin.insert(std::pair<unsigned int, struct transition>(state_index, t));
  }

  void operator()(state_doc const& hba) const
  {
    BOOST_FOREACH(if_clause_doc const& if_clause, hba.if_clauses)
    {
      Datatypes* dt = new Datatypes(); // TODO where is this gonna live? Make it a shared ptr?
      hba_doc_visitor p(_states, _states_bin, _conditions, _conditions_bin, _g, dt);
      p(if_clause, hba.id, state_index++);
    }
  }

  void operator()(condition_doc const& condition) const
  {
    std::cout << "condition " << condition.id << ":" << std::endl;
    if(condition.cond.which() == 0) // eidvalue
    {
      cond_eidvalue_doc cond = boost::get<cond_eidvalue_doc>(condition.cond);
      std::cout << "IPv6 addr: " << cond.ipv6_address << std::endl;
      std::cout << "EID: " << cond.eid << std::endl;
      std::cout << "Operator: " << cond.op << std::endl;
      std::cout << "Value: " << cond.value << std::endl;
      std::cout << "Datatype: " << (unsigned int)_datatypes->getDatatype(cond.eid) << std::endl;
      std::ostringstream oss;
      oss
        << cond.ipv6_address << COND_TABLE_SEPARATOR
        << cond.eid << COND_TABLE_SEPARATOR
        << (unsigned int)_datatypes->getDatatype(cond.eid) << COND_TABLE_SEPARATOR
        << cond.op << COND_TABLE_SEPARATOR
        << cond.value << COND_TABLE_SEPARATOR // TODO find out what is correct in the webinterface "spec"!
        ;
      _conditions.insert(std::pair<unsigned int, std::string>(condition.id, oss.str()));

      //construct binary representation
      struct condition c;
      // convert IP adress string to binary
      for(unsigned int i = 0; i < sizeof(c.sourceIP); i++)
      {
        std::stringstream ss;
        unsigned int ipbyte;
        ss << std::hex << cond.ipv6_address.substr(2 * i, 2); // read two bytes from the string, since two hex digits correspond to one byte in binary
        ss >> ipbyte;
        c.sourceIP[i] = ipbyte;
      }
      c.sourceEID = cond.eid;
      c.op = cond.op;
      c.datatype = _datatypes->getDatatype(cond.eid);
      std::stringstream ss;
      ss << std::hex << cond.value;
      if(_datatypes->getDatatype(cond.eid) == HXB_DTYPE_FLOAT) // TODO implement ALL the datatypes
      {
        ss >> *(float*)c.data;
      } else { // TODO for now just treat everything as uint32
        ss >> std::dec >> *(uint32_t*)c.data;
      }
      _conditions_bin.insert(std::pair<unsigned int, struct condition>(condition.id, c));
    } else { // timeout
      std::cout << "COND_TIMEOUT" << std::endl;
    }
  }

  flash_format_map_t& _states;
  flash_format_state_map_t& _states_bin;
  flash_format_map_t& _conditions;
  flash_format_cond_map_t& _conditions_bin;
  graph_t_ptr _g;
  Datatypes* _datatypes;
};

void generator_flash::operator()(std::ostream& os) const
{
  flash_format_map_t conditions;
  flash_format_cond_map_t conditions_bin;
  flash_format_map_t states;
  flash_format_state_map_t states_bin;

  std::cout << "start state: " << _ast.start_state << std::endl;
  BOOST_FOREACH(hba_doc_block const& block, _ast.blocks)
  {
    Datatypes* dt = new Datatypes(); // TODO one object is enough.
    boost::apply_visitor(hba_doc_visitor(states, states_bin, conditions, conditions_bin, _g, dt), block);
  }

  std::cout << "Created condition table:" << std::endl;
  for(flash_format_map_it_t it = conditions.begin(); it != conditions.end(); ++it)
  {
    std::cout << it->first << ": " << it->second << std::endl;
  }

  std::cout << "Created state table:" << std::endl;
  for(flash_format_map_it_t it = states.begin(); it != states.end(); ++it)
  {
    std::cout << it->first << ": " << it->second << std::endl;
  }

  std::cout << "Binary condition table:" << std::endl;
  for(flash_format_cond_map_it_t it = conditions_bin.begin(); it != conditions_bin.end(); it++)
  {
    std::cout << it->first << ": ";
    struct condition cond = it->second;
    char* c = (char*)&cond;
    for(unsigned int i = 0; i < sizeof(condition); i++)
    {
      std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)c[i] << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "Binary transition table:" << std::endl;
  for(flash_format_state_map_it_t it = states_bin.begin(); it != states_bin.end(); it++)
  {
    std::cout << it->first << ": ";
    struct transition t = it->second;
    char* c = (char*)&t;
    for(unsigned int i = 0; i < sizeof(transition); i++)
    {
      std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short int)c[i] << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "Writing output file..." << std::endl;
  // preamble
  os << COND_TABLE_BEGIN_TOKEN;
  // write conditions
  for(flash_format_map_it_t it = conditions.begin(); it != conditions.end(); it++)
  {
    os << it->second;
  }
  // table separator
  os << COND_TABLE_END_TOKEN << STATE_TABLE_BEGIN_TOKEN;
  // write transitions
  for(flash_format_map_it_t it = states.begin(); it != states.end(); ++it)
  {
    os << it->second;
  }
  os << STATE_TABLE_END_TOKEN;

  // output to file
  std::cout << "done." << std::endl;
}

