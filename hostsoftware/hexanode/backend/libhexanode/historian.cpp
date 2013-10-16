#include "historian.hpp"
#include "../../../../shared/endpoints.h"
#include <numeric>
#include <sstream>

using namespace hexanode;

int add_to_totals(int total, const devices_t::value_type& data) {
  return total + data.second;
}

void Historian::send_power_balance() {
  boost::mutex::scoped_lock lock(_guard);
  uint32_t total_consumption = std::accumulate(
      _power_consumptions.begin(), _power_consumptions.end(), 
      0, add_to_totals);
  uint32_t total_production = std::accumulate(
      _power_productions.begin(), _power_productions.end(), 
      0, add_to_totals);
  int32_t power_balance =  total_consumption - total_production;
  std::cout << "Sending power balance: " << power_balance << std::endl;
  _send_socket->send(
      hexabus::InfoPacket<float>(EP_POWER_BALANCE, power_balance));
}

void Historian::add_production(
    const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t& eid,
    const uint32_t current_production) {
  boost::mutex::scoped_lock lock(_guard);
  std::cout << "Adding production of " << current_production
    << " Watts of device " << endpoint.address() << std::endl;
  std::string device=endpoint_eid2device(endpoint, eid);
  _power_productions[device] = current_production;
}

void Historian::add_consumption(
    const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t& eid,
    const uint32_t consumption) 
{
  boost::mutex::scoped_lock lock(_guard);
  std::cout << "Adding consumption of " << consumption
    << " Watts of device " << endpoint.address() << std::endl;
  std::string device=endpoint_eid2device(endpoint, eid);
  _power_consumptions[device] = consumption;
}

void Historian::remove_device(
    const boost::asio::ip::udp::endpoint& endpoint,
    const uint32_t& eid)
{
  boost::mutex::scoped_lock lock(_guard);
  std::string device=endpoint_eid2device(endpoint, eid);
  std::cout << "Clearing device " << device << std::endl;
  _power_consumptions.erase(device);
  _power_productions.erase(device);
}

void Historian::run() {
  _t = boost::thread(
      boost::bind(&Historian::loop, this)
      );
}

// Add consumption
//boost::mutex::scoped_lock lock(_comsumptions_guard);
//_power_consumptions.push_back(value);

void Historian::loop() {
  while (! _terminate_loop) {
    send_power_balance();
    boost::this_thread::sleep(boost::posix_time::seconds(_update_seconds));
  }
}


const std::string Historian::endpoint_eid2device(
    const boost::asio::ip::udp::endpoint& endpoint,
    const uint32_t& eid)
{
  std::ostringstream oss;
  oss << endpoint.address() << "%" << eid;
  return oss.str();
}

