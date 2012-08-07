#ifndef LIBHBC_GRAPH_BUILDER_HPP
#define LIBHBC_GRAPH_BUILDER_HPP

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>
#include <libhbc/ast_datatypes.hpp>

namespace hexabus {

  class GraphBuilder {
    public:
      typedef std::tr1::shared_ptr<GraphBuilder> Ptr;
      GraphBuilder() : _g(new graph_t()) {};
      virtual ~GraphBuilder() {};

      graph_t_ptr get_graph() const { return _g; };
      void write_graphviz(std::ostream& os);
      void operator()(hbc_doc& hbc);

    private:
      GraphBuilder& operator= (const GraphBuilder& rhs);

      graph_t_ptr _g;
  };
};

#endif // LIBHBC_GRAPH_BUILDER_HPP

