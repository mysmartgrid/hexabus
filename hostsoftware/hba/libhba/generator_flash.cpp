#include "generator_flash.hpp"
#include <boost/foreach.hpp>

using namespace hexabus;

// TODO: Zwei listen fuer conditions und states
// TODO: States am Ende so sortieren, dass der Startstate der erste ist.


struct hba_doc_visitor : boost::static_visitor<> {
  hba_doc_visitor(std::ostream& os, const graph_t_ptr g) 
    : _os(os),
     _g(g)
  { }

  void operator()(if_clause_doc const& clause, unsigned int state_id) const {
    unsigned int cond_id = find_vertex_id(_g, clause.name);
    unsigned int goodstate_id = find_vertex_id(_g, clause.goodstate);
    unsigned int badstate_id = find_vertex_id(_g, clause.badstate);

    _os << state_id << ": clause " << clause.name 
      << " (id " << cond_id << ")" << std::endl;
    _os << " set eid " << clause.eid 
      << " := " << clause.value << std::endl;
    _os << " goodstate: " << goodstate_id << std::endl;
    _os << " badstate: " << badstate_id << std::endl;
  }

  void operator()(state_doc const& hba) const
  {
    BOOST_FOREACH(if_clause_doc const& if_clause, hba.if_clauses)
    {
      hba_doc_visitor p(_os, _g);
      p(if_clause, hba.id);
    }
  }

  void operator()(condition_doc const& hba) const
  {
    _os << "condition " << hba.id << ":" << std::endl;
    _os << "IPv6 addr: " << hba.ipv6_address << std::endl; 
    // for( unsigned int i = 0; i < hba.ipv6_address.size(); i += 1) {
    //   _os << std::setw(2) << std::setfill('0') 
    //     << std::hex << (int)(hba.ipv6_address[i] & 0xFF);
    // }
    _os << "EID: " << hba.eid << std::endl;
    _os << "Operator: " << hba.op << std::endl;
    _os << "Value: " << hba.value << std::endl;
  }

  std::ostream& _os;
  graph_t_ptr _g;
};

void generator_flash::operator()(std::ostream& os) const {
  std::cout << "start state: " << _ast.start_state << std::endl;

  BOOST_FOREACH(hba_doc_block const& block, _ast.blocks)
  {
    boost::apply_visitor(hba_doc_visitor(os, _g), block);
  }

}


