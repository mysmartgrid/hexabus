#!/usr/bin/env python3

import ipaddress
import subprocess
import io
import re
import random

class DiscoveryFailed(Exception):
	"""Discovery of networks failed"""

def discover_present_prefixes(iface):
	"""List all prefixes seen by the machine.

	Calls rdisc6 to aquire a list a prefixes and routes announced on the network.

	Args:
		iface: name of the interface to ask for routes

	Returns:
		A tuple (prefixes, routes) of IPv6 networks reachable by this node
	"""
	prefixes = []
	routes = []
	with subprocess.Popen(["rdisc6", iface], stdout=subprocess.PIPE) as proc:
		tio = io.TextIOWrapper(proc.stdout)
		line = tio.readline()
		while len(line) > 0:
			parts = re.match("^ (Prefix|Route)\s*:\s*([^\s]*)", line)
			if parts != None:
				if parts.group(1) == "Prefix":
					prefixes.append(ipaddress.ip_network(parts.group(2)))
				else:
					routes.append(ipaddress.ip_network(parts.group(2)))
			line = tio.readline()
			proc.wait()
			if proc.returncode != 0:
				raise DiscoveryFailed
	return (prefixes, routes)

def net_contains(net, cand):
	"""Checks whether a network fully contains another network

	Args:
		net: candidate supernet
		cand: candidate subnet

	Returns:
		True if and only if net is a strict supernet of cand
	"""
	return cand.network_address in net and cand.broadcast_address in net

def exclude_all(netlist):
	"""Calculates a list of private prefixes that are not subnets of given networks

	Args:
		netlist: a list of networks

	Returns:
		A list of networks in fd00::/8 that are not subnets of nets in netlist
	"""
	result = [ ipaddress.ip_network("fd00::/8") ]
	for i in netlist:
		next = []
		for net in result:
			if net_contains(net, i):
				next.extend(net.address_exclude(i))
			elif net_contains(i, net):
				next.extend(i.address_exclude(net))
		result = next
	return result

def select_networks(prefixes, unusedPrefixes):
	"""Select a suitable pair of prefixes for a HxB router node

	Args:
		prefixes: list of prefixes available for SLAAC
		unusedPrefixes: list if viable prefixes for the HxB network

	Returns:
		A tuple (eth, hxb) of prefixes for the ethernet side and the hexabus side
		of the network. eth may be None if 
	"""
	hxb = random.choice(unusedPrefixes)
	while hxb.prefixlen < 63:
		hxb = random.choice(list(hxb.subnets(1)))
	if len(prefixes) > 0:
		return (prefixes[0], list(hxb.subnets(1))[1])
	else:
		nets = list(hxb.subnets(1))
		return (nets[0], nets[1])

if __name__ == '__main__':
	try:
		(prefixes, routes) = discover_present_prefixes("wlan0")
		viable = exclude_all(prefixes + routes)
		nets = select_networks(prefixes, viable)
		print(nets)
	except DiscoveryFailed:
		exit(1)
	except:
		exit(2)
#	for p in prefixes:
#		print(p)
