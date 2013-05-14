#include "historian.hpp"
#include <numeric>


using namespace hexanode;

int add_to_totals(int total, const devices_t::value_type& data) {
  return total + data.second;
}


void Historian::send_power_balance() {
  boost::mutex::scoped_lock lock(_guard);
  uint32_t total_consumption = std::accumulate(
      _power_consumptions.begin(), _power_consumptions.end(), 
      0, add_to_totals);
  int32_t power_balance =  total_consumption - _power_production;
  std::cout << "Sending power balance: " << power_balance << std::endl;
  // TODO: Really send power balance!
}

void Historian::set_production(uint32_t current_production) {
  boost::mutex::scoped_lock lock(_guard);
  _power_production = current_production;
}

void Historian::add_consumption(
    const boost::asio::ip::udp::endpoint& endpoint,
    const uint32_t consumption) 
{
  boost::mutex::scoped_lock lock(_guard);
  std::cout << "Adding consumption of " << consumption
    << " Watts of device " << endpoint.address() << std::endl;
  _power_consumptions[endpoint.address()] = consumption;
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
  while (! _terminate_midievent_loop) {
    send_power_balance();
    boost::this_thread::sleep(boost::posix_time::seconds(_update_seconds));
  }
}



