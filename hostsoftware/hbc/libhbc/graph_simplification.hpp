#ifndef LIBHBC_GRAPH_SIMPLIFICATION
#define LIBHBC_GRAPH_SIMPLIFICATION

#include <libhbc/graph.hpp>
#include <libhbc/ast_datatypes.hpp>

namespace hexabus {
  class GraphSimplification {
    public:
      GraphSimplification(graph_t_ptr g) : _g(g) { }

    private:
      void addTransition(vertex_id_t from, vertex_id_t to, command_block_doc& commands);
      command_block_doc commandBlockTail(command_block_doc& commands);

      graph_t_ptr _g;
  };
};

#endif // LIBHBC_GRAPH_SIMPLIFICATION
