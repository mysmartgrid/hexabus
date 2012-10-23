#ifndef LIBHBC_GRAPH_SIMPLIFICATION
#define LIBHBC_GRAPH_SIMPLIFICATION

#include <libhbc/graph.hpp>
#include <libhbc/ast_datatypes.hpp>

namespace hexabus {
  class GraphSimplification {
    public:
      GraphSimplification(std::map<std::string, graph_t_ptr> state_machines) : _in_state_machines(state_machines) { }

      void operator()();

    private:
      vertex_id_t addTransition(vertex_id_t from, vertex_id_t to, command_block_doc& commands, graph_t_ptr g, unsigned int& max_vertex_id);
      void deleteOthersWrites();
      void expandMultipleWrites();
      void expandMultipleWriteNode(vertex_id_t vertex_id, graph_t_ptr g, unsigned int& max_vertex_id);
      command_block_doc commandBlockTail(command_block_doc& commands);
      command_block_doc commandBlockHead(command_block_doc& commands);

      std::map<std::string, graph_t_ptr> _in_state_machines;
  };
};

#endif // LIBHBC_GRAPH_SIMPLIFICATION
