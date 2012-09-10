#ifndef LIBHBC_HBA_OUTPUT_HPP
#define LIBHBC_HBA_OUTPUT_HPP

namespace hexabus {
  class HBAOutput {
    public:
      typedef std::tr1::shared_ptr<HBAOutput> Ptr;
      HBAOutput (graph_t_ptr g) : _g(g) {};
      virtual ~HBAOutput() {};

      operator()(std::ostream& ostr);

    private:
      graph_t_ptr _g;
  };
}

#endif // LIBHBC_HBA_OUTPUT_HPP

