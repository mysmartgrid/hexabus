#ifndef LIBHBA_LABEL_WRITER_HPP
#define LIBHBA_LABEL_WRITER_HPP 1

#include <libhba/common.hpp>
#include <libhba/graph_builder.hpp>

namespace hexabus {

  template <class Name, class ID, class Type>
    class state_cond_label_writer {
      public:
        state_cond_label_writer(Name _name, ID _id, Type _type) 
          : name(_name), id(_id), type(_type) {};
        template <class VertexOrEdge>
          void operator()(std::ostream& out, const VertexOrEdge& v) const {
            if (type[v] == STATE) {
              out << "[shape=oval,rank=source,label=\"" << id[v] << ": " << name[v] << "\"]";
            } else if (type[v] == CONDITION) {
              out << "[shape=box,rank=min,label=\"" << id[v] << ": " << name[v] << "\"]";
            } else if (type[v] == STARTSTATE) {
              out << "[shape=hexagon,rank=max,label=\"" << id[v] << ": " << name[v] << "\"]";
            } else {
              std::ostringstream oss;
              oss << "Unknown vertex type for vertex " << name[v]; 
              throw GenericException(oss.str());
            }	
          }
      private:
        Name name;
        ID id;
        Type type;
    };

  template < class Name, class ID, class Type >
    state_cond_label_writer<Name, ID, Type>
    make_state_cond_label_writer(Name n, ID id, Type t) {
      return state_cond_label_writer<Name, ID, Type>(n, id, t);
    }


  template <class Name, class Type>
    class edge_label_writer {
      public:
        edge_label_writer(Name _name, Type _type) 
          : name(_name), type(_type) {};
        template <class VertexOrEdge>
          void operator()(std::ostream& out, const VertexOrEdge& v) const {
            if (type[v] == FROM_STATE) {
              out << "[color=\"black\"]";
            } else if (type[v] == BAD_STATE) {
              out << "[color=\"red\"]";
            } else if (type[v] == GOOD_STATE) {
              out << "[color=\"green\"]";
            } else {
              std::ostringstream oss;
              oss << "Unknown edge type for edge " << name[v]; 
              throw GenericException(oss.str());
            }	
          }
      private:
        Name name;
        Type type;
    };

  template < class Name, class Type >
    edge_label_writer<Name, Type>
    make_edge_label_writer(Name n, Type t) {
      return edge_label_writer<Name, Type>(n, t);
    }


}


#endif /* LIBHBA_LABEL_WRITER_HPP */
