#ifndef RESYNCD_HPP_
#define RESYNCD_HPP_

#include <string>

void run_resyncd(const std::string& iface);
void run_resyncd_sync(const std::string& iface);
void kill_resyncd(const std::string& iface);

#endif
