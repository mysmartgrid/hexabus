#ifndef LIBHEXANODE_INFO_QUERY_HPP
#define LIBHEXANODE_INFO_QUERY_HPP 1

#include <libhexanode/common.hpp>
#include <libhexabus/socket.hpp>

namespace hexanode {
  class InfoQuery {
    public:
      typedef boost::shared_ptr<InfoQuery> Ptr;
      InfoQuery (hexabus::Socket* network) 
        : _network(network) {};
      virtual ~InfoQuery() {};

      std::string get_device_name(
          const boost::asio::ip::address_v6& addr);

    private:
      InfoQuery (const InfoQuery& original);
      InfoQuery& operator= (const InfoQuery& rhs);
      hexabus::Socket* _network;
      
  };
  
};


#endif /* LIBHEXANODE_INFO_QUERY_HPP */

