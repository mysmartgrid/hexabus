#include "generator_flash.hpp"
#include <boost/foreach.hpp>
#include <string>
#include <sstream>
#include <map>
using namespace hexabus;


const char COND_TABLE_BEGIN_TOKEN='-';
const char COND_TABLE_END_TOKEN='.';
const char COND_TABLE_SEPARATOR='.';
const char STATE_TABLE_BEGIN_TOKEN='-';
const char STATE_TABLE_END_TOKEN='.';
const char STATE_TABLE_SEPARATOR='.';

typedef std::map<unsigned int, std::string> flash_format_map_t;
typedef std::map<unsigned int, std::string>::iterator flash_format_map_it_t;

// TODO: Zwei listen fuer conditions und states
// TODO: States am Ende so sortieren, dass der Startstate der erste ist.


struct hba_doc_visitor : boost::static_visitor<> {
  hba_doc_visitor(
      flash_format_map_t& states,
      flash_format_map_t& conditions,
      const graph_t_ptr g) 
    : _states(states),
      _conditions(conditions),
     _g(g)
  { }

  void operator()(if_clause_doc const& clause, unsigned int state_id) const {
    unsigned int cond_id = find_vertex_id(_g, clause.name);
    unsigned int goodstate_id = find_vertex_id(_g, clause.goodstate);
    unsigned int badstate_id = find_vertex_id(_g, clause.badstate);

    std::cout << state_id << ": clause " << clause.name 
      << " (id " << cond_id << ")" << std::endl;
    std::cout << " set eid " << clause.eid 
      << " := " << clause.value << std::endl;
    std::cout << " goodstate: " << goodstate_id << std::endl;
    std::cout << " badstate: " << badstate_id << std::endl;
    std::ostringstream oss;
    oss
      << state_id << STATE_TABLE_SEPARATOR
      << cond_id << STATE_TABLE_SEPARATOR
      << clause.eid << STATE_TABLE_SEPARATOR
      << "X" << STATE_TABLE_SEPARATOR
      << clause.value << STATE_TABLE_SEPARATOR
      << goodstate_id << STATE_TABLE_SEPARATOR
      << badstate_id << STATE_TABLE_SEPARATOR
      ;
    _states.insert(std::pair<unsigned int, std::string>(state_id, oss.str()));
  }

  void operator()(state_doc const& hba) const
  {
    BOOST_FOREACH(if_clause_doc const& if_clause, hba.if_clauses)
    {
      hba_doc_visitor p(_states, _conditions, _g);
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
    std::ostringstream oss;
    oss
      << condition.ipv6_address << COND_TABLE_SEPARATOR
      << condition.eid << COND_TABLE_SEPARATOR
      << condition.dtype << COND_TABLE_SEPARATOR
      << condition.op << COND_TABLE_SEPARATOR
      << condition.value << COND_TABLE_SEPARATOR
      ;
    _conditions.insert(std::pair<unsigned int, std::string>(condition.id, oss.str()));
  }

  flash_format_map_t& _states;
  flash_format_map_t& _conditions;
  graph_t_ptr _g;
};

void generator_flash::operator()(std::ostream& os) const {
  flash_format_map_t conditions;
  flash_format_map_t states;

  std::cout << "start state: " << _ast.start_state << std::endl;
  BOOST_FOREACH(hba_doc_block const& block, _ast.blocks)
  {
    boost::apply_visitor(hba_doc_visitor(states, conditions, _g), block);
  }

  std::cout << "Created condition table:" << std::endl;
  for( flash_format_map_it_t it = conditions.begin(); 
      it != conditions.end(); ++it) {
    std::cout << (*it).first << ": " << (*it).second << std::endl;
  }
 
  std::cout << "Created state table:" << std::endl;
  for( flash_format_map_it_t it = states.begin(); 
      it != states.end(); ++it) {
    std::cout << (*it).first << ": " << (*it).second << std::endl;
  }
 

}


