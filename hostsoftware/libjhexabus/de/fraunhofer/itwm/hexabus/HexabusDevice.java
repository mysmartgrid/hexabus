package de.fraunhofer.itwm.hexabus;

import java.net.InetAddress;
import java.util.Collection;
import java.util.ArrayList;

public class HexabusDevice {
	private InetAddress address;
	private Collection<HexabusEndpoint> endpoints;

	public HexabusDevice(InetAddress address) {
		this.address = address;
		endpoints = new ArrayList<HexabusEndpoint>();
	}

	public void addEndpoint(int eid, Hexabus.DataType datatype, String description) {
		endpoints.add(new HexabusEndpoint(this, eid, datatype, description));
	}

	public void addEndpoint(int eid, Hexabus.DataType datatype) {
		endpoints.add(new HexabusEndpoint(this, eid, datatype));
	}

	public void addEndpoint(HexabusEndpoint endpoint) {
		endpoints.add(endpoint);
	}

	public void addEndpoints(Collection<HexabusEndpoint> endpoints) {
		this.endpoints.addAll(endpoints);
	}

	public InetAddress getIP() {
		return address;
	}

	public Collection<HexabusEndpoint> getEndpoints() {
		return endpoints;
	}
}
