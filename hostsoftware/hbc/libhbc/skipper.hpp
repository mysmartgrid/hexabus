#ifndef LIBHBC_SKIPPER_HPP
#define LIBHBC_SKIPPER_HPP

namespace hexabus {
  //  Our comment parser definition

  template<class Iterator>
    class skipper : public boost::spirit::qi::grammar<Iterator>
  {
    public:
      skipper():skipper::base_type(start)
      {
        using namespace boost::spirit::qi;
        start = space | char_('#') >> *(char_ - eol) >> eol;
      }

      boost::spirit::qi::rule<Iterator> start;
  };
};

#endif /* LIBHBC_SKIPPER_HPP */

