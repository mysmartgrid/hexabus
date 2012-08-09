#ifndef LIBHBC_LABEL_WRITER_HPP
#define LIBHBC_LABEL_WRITER_HPP

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>

namespace hexabus {

  template <class Name, class Type> class vertex_label_writer {
    public:
      vertex_label_writer(Name _name, Type _type)
        : name(_name), type(_type) { };

      template <class VertexOrEdge> void operator()(std::ostream& out, const VertexOrEdge& v) const {
        out << "[";
        switch(type[v]) {
          case v_cond:
            out << "shape=diamond";
            break;
          case v_state:
            out << "shape=circle";
            break;
          case v_command:
            out << "shape=rectangle";
            break;
        }
        out << ",label=\"" << name[v] << "\"]";
      }

    private:
      Name name;
      Type type;
  };

  template <class Name, class Type> vertex_label_writer<Name, Type> make_vertex_label_writer(Name n, Type type) {
    return vertex_label_writer<Name, Type>(n, type);
  }
};

#endif // LIBHBC_LABEL_WRITER_HPP

