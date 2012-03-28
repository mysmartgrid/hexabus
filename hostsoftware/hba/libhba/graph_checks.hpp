#ifndef LIBHBA_GRAPH_CHECKS_HPP
#define LIBHBA_GRAPH_CHECKS_HPP 1

#include <libhba/common.hpp>
#include <libhba/graph.hpp>

namespace hexabus {
  class GraphChecks {
    public:
      typedef std::tr1::shared_ptr<GraphChecks> Ptr;
      GraphChecks (graph_t_ptr g) : _g(g) {};
      virtual ~GraphChecks() {};
      void check_states_incoming() const;
      void check_states_outgoing() const;
      void check_conditions_incoming() const;
      void check_conditions_outgoing() const;

    private:
      GraphChecks (const GraphChecks& original);
      GraphChecks& operator= (const GraphChecks& rhs);
      graph_t_ptr _g;
      
  };
  
};


#endif /* LIBHBA_GRAPH_CHECKS_HPP */

