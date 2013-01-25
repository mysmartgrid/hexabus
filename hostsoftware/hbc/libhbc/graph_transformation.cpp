#include "graph_transformation.hpp"

#include <libhbc/error.hpp>
#include <libhbc/hbc_printer.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include <boost/graph/graphviz.hpp>
#include <libhbc/label_writer.hpp>

using namespace hexabus;

machine_map_t GraphTransformation::generateMachineMap(graph_t_ptr g) {
  machine_map_t machine_map;
  // iterate over all vertices in graph
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(*g);
  for(; vertexIt != vertexEnd; vertexIt++) {
    vertex_id_t vertexID = *vertexIt;
    // look for machine ID in map
    machine_map_t::iterator node_it;
    if((node_it = machine_map.find((*g)[vertexID].machine_id)) != machine_map.end()) {
      // if there, append found vertex to vector in map
      node_it->second.push_back(vertexID);
    } else {
      std::vector<vertex_id_t> vertex_vect;
      vertex_vect.push_back(vertexID);
      // if not there, make new entry in map
      machine_map.insert(std::pair<unsigned int, std::vector<vertex_id_t> >((*g)[vertexID].machine_id, vertex_vect));
    }
  }
  // okay. now we have a map mapping from each state machine ID to the vertices belonging to this state machine. (1)

  return machine_map;
}

std::vector<std::string> GraphTransformation::findDevices(std::vector<vertex_id_t> stm_vertices, graph_t_ptr g) {
  std::vector<std::string> devices;

  // iterate over all vertices in the state machine
  BOOST_FOREACH(vertex_id_t v_id, stm_vertices) {
    if((*g)[v_id].type == v_command) { // only look at command block vertices
      std::string device_name = "";
      try {
        command_block_doc cmdblck = boost::get<command_block_doc>((*g)[v_id].contents);
        // iterate over all commands in the command block
        BOOST_FOREACH(command_doc cmd, cmdblck.commands) {
          try {
          std::string device_name = boost::get<std::string>(cmd.write_command.geid.device_alias);
          if(!exists(device_name, devices))
            devices.push_back(device_name);
          } catch(boost::bad_get b) {
            std::ostringstream oss;
            std::map<unsigned int, std::string>::iterator it = machine_filenames_per_id.find((*g)[v_id].machine_id);
            if(it != machine_filenames_per_id.end())
              oss << "[" << it->second << ":" << cmd.write_command.lineno << "] ";
            oss << "Placeholder found in state machine instance (can not be resolved)" << std::endl;
            throw GraphTransformationErrorException(oss.str());
          }
        }
      } catch(boost::bad_get b) {
        throw GraphTransformationErrorException("Contents of command vertex are not a command block (Graph transformation stage)");
      }
    }
  }

  return devices;
}

template <typename T> bool GraphTransformation::exists(T elem, std::vector<T> vect) {
  bool exists = false;
  BOOST_FOREACH(T the_elem, vect)
    if(the_elem == elem)
      exists = true;
  return exists;
}

graph_t_ptr GraphTransformation::reconstructGraph(std::vector<vertex_id_t> vertices, graph_t_ptr in_g) {
  graph_t_ptr out_g(new graph_t());
  std::map<vertex_id_t, vertex_id_t> vertex_mapping; // keep track which vertices in the constructed graph correspond to vertices in the input graph
  BOOST_FOREACH(vertex_id_t in_v_id, vertices) {
    // first: add all the vertices
    vertex_t in_v = (*in_g)[in_v_id];
    vertex_id_t out_v_id = add_vertex(out_g, in_v.name, in_v.machine_id, in_v.vertex_id, in_v.type, in_v.contents);
    vertex_mapping.insert(std::pair<vertex_id_t, vertex_id_t>(in_v_id, out_v_id));
  }

  BOOST_FOREACH(vertex_id_t in_v_id, vertices) {
    // second pass: add the vertex's edges
    graph_t::adjacency_iterator adj_vert_it, adj_vert_end;
    vertex_id_t out_from_v_id = vertex_mapping.find(in_v_id)->second; // vertex in out_graph FROM which the edges should be drawn
    for(boost::tie(adj_vert_it, adj_vert_end) = adjacent_vertices(in_v_id, *in_g); adj_vert_it != adj_vert_end; adj_vert_it++) {
      vertex_id_t out_to_v_id = vertex_mapping.find(*adj_vert_it)->second;
      add_edge(out_g, out_from_v_id, out_to_v_id, e_transformed);
    }
  }

  return out_g;
}

void GraphTransformation::operator()(graph_t_ptr in_g) {
  // * iterate over (list of) state machines              |
  // * for each state machine:                            |
  //   * make list of devices  (1)                        |
  //   * slice each state machine for each device         | * list of state machine slices for devices
  // * if a device appears in multiple state machines:    |   (at this point one dev. can have several state machines)
  //   * don't continue. parallel composition woule lead to too many states

  machine_map_t machines_per_stmid = generateMachineMap(in_g); // list of nodes for each state machine ID

  device_map_t machines_per_device; // Multimap, maps from Device name -> several Vectors of vertex_id_t's (each vector representing one state machine)

  BOOST_FOREACH(machine_map_elem mm_el, machines_per_stmid) {
    std::vector<std::string> device_names = findDevices(mm_el.second, in_g); // list of devices for this state machine

    // now we have a list off all device names used in state machine #mm_el.first
    // -- insert the machine into the list of machines for each device it contains.
    BOOST_FOREACH(std::string device_name, device_names) {
      // now we can construct the graph from the list of nodes in the mm_el.
      graph_t_ptr g = reconstructGraph(mm_el.second, in_g);


      // check if device already has a graph assigned to it.
      if(device_graphs.find(device_name) != device_graphs.end())
      { // if the device already has a graph, throw an error
        std::ostringstream oss;
        oss << "Error: Device named '" << device_name << "' has multiple state machines writing to it. This is not supported." << std::endl;
        throw GraphTransformationErrorException(oss.str());
      }

      // then we can add it to a list of graphs (state machines) for each device
      device_graphs.insert(std::pair<std::string, graph_t_ptr>(device_name, g));

    }
  }
}

void GraphTransformation::writeGraphviz(std::string filename_prefix) {
  std::map<std::string, graph_t_ptr>::iterator graph_it;
  for(graph_it = device_graphs.begin(); graph_it != device_graphs.end(); graph_it++) {
    std::ostringstream fnoss;
    fnoss << filename_prefix << graph_it->first << ".dot";

    std::map<std::string, std::string> graph_attr, vertex_attr, edge_attr;
    graph_attr["ratio"] = "fill";
    vertex_attr["shape"] = "rectangle";

    std::ofstream os;
    os.open(fnoss.str().c_str());

    if(!os) {
      std::ostringstream oss;
      oss << "Could not open file " << fnoss.str() << " for graph output." << std::endl;
      throw GraphOutputException(oss.str());
    }

    graph_t_ptr g = graph_it->second;

    boost::write_graphviz(os, (*g),
        make_vertex_label_writer(
          boost::get(&vertex_t::name, (*g)), boost::get(&vertex_t::type, (*g))),
        boost::make_label_writer(boost::get(&edge_t::name, (*g))),
        boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr)
        );
    os.close();
  }
}

std::map<std::string, graph_t_ptr> GraphTransformation::getDeviceGraphs() {
  return device_graphs;
}
