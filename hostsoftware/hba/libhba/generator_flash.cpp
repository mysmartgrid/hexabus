#include "generator_flash.hpp"
#include <boost/foreach.hpp>
#include <string>
#include <sstream>
#include <map>
#include <iomanip> // for hexadecimal output TODO probably not needed later!
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

struct hba_doc_visitor : boost::static_visitor<>
{
  hba_doc_visitor(
      flash_format_map_t& states,
      flash_format_state_map_t& states_bin,
      flash_format_map_t& conditions,
      flash_format_cond_map_t& conditions_bin,
      const graph_t_ptr g)
    : _states(states),
      _states_bin(states_bin),
      _conditions(conditions),
      _conditions_bin(conditions_bin),
      _g(g)
  { }

  void operator()(if_clause_doc const& clause, unsigned int state_id) const
  {
    unsigned int cond_id = find_vertex_id(_g, clause.name);
    unsigned int goodstate_id = find_vertex_id(_g, clause.goodstate);
    unsigned int badstate_id = find_vertex_id(_g, clause.badstate);

    std::cout << state_id << ": clause " << clause.name
      << " (id " << cond_id << ")" << std::endl;
    std::cout << " set eid " << clause.eid
      << " := " << clause.value << " datatype " << clause.dtype << std::endl;
    std::cout << " goodstate: " << goodstate_id << std::endl;
    std::cout << " badstate: " << badstate_id << std::endl;

    // construct webinterface representation (string)
    std::ostringstream oss;
    oss
      << state_id << STATE_TABLE_SEPARATOR
      << cond_id << STATE_TABLE_SEPARATOR
      << clause.eid << STATE_TABLE_SEPARATOR
      << clause.dtype << STATE_TABLE_SEPARATOR
      << clause.value << STATE_TABLE_SEPARATOR
      << goodstate_id << STATE_TABLE_SEPARATOR
      << badstate_id << STATE_TABLE_SEPARATOR
      ;
    _states.insert(std::pair<unsigned int, std::string>(state_id, oss.str()));


    // construct binary representation
    struct transition t;
    t.fromState = state_id; // TODO need to cast here?
    t.cond = cond_id;
    t.eid = clause.eid;
    t.goodState = goodstate_id;
    t.badState = badstate_id;
    t.value.datatype = clause.dtype;
    memset(t.value.data, 0, sizeof(t.value.data)); // most of the time only four bytes needes, so keep the rest at 0
    std::stringstream ss;
    ss << std::hex << clause.value;
    ss >> *(uint32_t*)t.value.data;
    _states_bin.insert(std::pair<unsigned int, struct transition>(state_id, t));
  }

  void operator()(state_doc const& hba) const
  {
    BOOST_FOREACH(if_clause_doc const& if_clause, hba.if_clauses)
    {
      hba_doc_visitor p(_states, _states_bin, _conditions, _conditions_bin, _g);
      p(if_clause, hba.id);
    }
  }

  void operator()(condition_doc const& condition) const
  {
    std::cout << "condition " << condition.id << ":" << std::endl;
    std::cout << "IPv6 addr: " << condition.ipv6_address << std::endl;
    // for( unsigned int i = 0; i < condition.ipv6_address.size(); i += 1) {
    //   std::cout << std::setw(2) << std::setfill('0') 
    //     << std::hex << (int)(condition.ipv6_address[i] & 0xFF);
    // }
    std::cout << "EID: " << condition.eid << std::endl;
    std::cout << "Operator: " << condition.op << std::endl;
    std::cout << "Value: " << condition.value << std::endl;
    std::cout << "Datatype: " << condition.dtype << std::endl;
    std::ostringstream oss;
    oss
      << condition.ipv6_address << COND_TABLE_SEPARATOR
      << condition.eid << COND_TABLE_SEPARATOR
      << condition.dtype << COND_TABLE_SEPARATOR
      << condition.op << COND_TABLE_SEPARATOR
      << condition.value << COND_TABLE_SEPARATOR
      ;
    _conditions.insert(std::pair<unsigned int, std::string>(condition.id, oss.str()));

    //construct binary representation
    struct condition c;
    // convert IP adress string to binary
    for(unsigned int i = 0; i < sizeof(c.sourceIP); i++)
    {
      std::stringstream ss;
      unsigned int ipbyte;
      ss << std::hex << condition.ipv6_address.substr(2 * i, 2); // read two bytes from the string, since two hex digits correspond to one byte in binary
      ss >> ipbyte;
      c.sourceIP[i] = ipbyte;
    }
    c.sourceEID = condition.eid;
    c.op = condition.op;
    c.datatype = condition.dtype;
    std::stringstream ss;
    ss << std::hex << condition.value;
    ss >> *(uint32_t*)c.data;
    _conditions_bin.insert(std::pair<unsigned int, struct condition>(condition.id, c));
  }

  flash_format_map_t& _states;
  flash_format_state_map_t& _states_bin;
  flash_format_map_t& _conditions;
  flash_format_cond_map_t& _conditions_bin;
  graph_t_ptr _g;
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
    boost::apply_visitor(hba_doc_visitor(states, states_bin, conditions, conditions_bin, _g), block);
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

