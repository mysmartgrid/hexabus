#ifndef LIBHBA_GRAPH_BUILDER_HPP
#define LIBHBA_GRAPH_BUILDER_HPP 1

#include <libhba/common.hpp>
#include <libhba/ast_datatypes.hpp>
#include <libhba/graph.hpp>
#include <iostream>
#include <string>

namespace hexabus {
    class GraphBuilder {
    public:
      typedef std::tr1::shared_ptr<GraphBuilder> Ptr;
      GraphBuilder () : 
        _g(new graph_t()) {};
      virtual ~GraphBuilder() {};

      graph_t_ptr get_graph() const { return _g; };
      void write_graphviz(std::ostream& os);
      void operator()(hba_doc& hba);

    private:
      GraphBuilder& operator= (const GraphBuilder& rhs);

      void mark_start_state(const std::string& name);
      graph_t_ptr _g;



  };

};


#endif /* LIBHBA_GRAPH_BUILDER_HPP */
