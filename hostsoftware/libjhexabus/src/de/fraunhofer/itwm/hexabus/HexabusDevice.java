package de.fraunhofer.itwm.hexabus;

import java.net.InetAddress;
import java.util.HashMap;
import java.util.Map;
import java.io.IOException;
import java.net.UnknownHostException;

public class HexabusDevice {
	private InetAddress address;
	private HashMap<Integer,HexabusEndpoint> endpoints;

	/**
	 * @param address The InetAddress of the device
	 */
	public HexabusDevice(InetAddress address) {
		this.address = address;
		endpoints = new HashMap<Integer,HexabusEndpoint>();
		try{
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		}catch(Hexabus.HexabusException e){
			//TODO
		}
	}

	public HexabusDevice(String address) {
		try{
		this.address = InetAddress.getByName(address);
		}catch(UnknownHostException e) {
			//TODO
		}
		endpoints = new HashMap<Integer,HexabusEndpoint>();
		try{
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		}catch(Hexabus.HexabusException e){
			//TODO
		}
	}

	/**
	 * Creates an Endpoint with specified EID, data type and description
	 * and adds it to the endpoints associated with the device.
	 *
	 * @param eid The EID of the new endpoint
	 * @param datatype The data type of the new endpoint
	 * @param description A description of the new endpoint
	 * @return The newly created endpoint
	 */
	public HexabusEndpoint addEndpoint(int eid, Hexabus.DataType datatype, String description) throws Hexabus.HexabusException {
		if(endpoints.containsKey(new Integer(eid))) {
			throw new Hexabus.HexabusException("Endpoint exists");
		}
		HexabusEndpoint endpoint = new HexabusEndpoint(this, eid, datatype, description);
		endpoints.put(eid, endpoint);
		return endpoint;
	}

	/**
	 * Creates an Endpoint with specified EID, data type and empty description
	 * and adds it to the endpoints associated with the device.
	 *
	 * @param eid The EID of the new endpoint
	 * @param datatype The data type of the new endpoint
	 * @return The newly created endpoint
	 */
	public HexabusEndpoint addEndpoint(int eid, Hexabus.DataType datatype) {
		HexabusEndpoint endpoint = new HexabusEndpoint(this, eid, datatype);
		endpoints.put(eid, endpoint);
		return endpoint;
	}

	/**
	 * @return The InetAddress of the device
	 */
	public InetAddress getInetAddress() {
		return address;
	}

	/**
	 * @return The endpoints that are associated with the device
	 */
	public HashMap<Integer, HexabusEndpoint> getEndpoints() {
		return endpoints;
	}

	public HexabusEndpoint getByEid(int eid) throws Hexabus.HexabusException {
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		
		for(Map.Entry<Integer, HexabusEndpoint> entry : endpoints.entrySet()) {
			if(entry.getKey().intValue() == eid) {
				return entry.getValue();
			}
		}

		throw new Hexabus.HexabusException("EID not found");
	}

	/**
	 * Sends an endpoint query to the device and its endpoints.
	 * Replaces the currently asscociated endpoints with the result.
	 */
	public HashMap<Integer, HexabusEndpoint> fetchEndpoints() throws Hexabus.HexabusException, IOException {
		HashMap<Integer, HexabusEndpoint> oldEndpoints = endpoints;
		endpoints = new HashMap<Integer, HexabusEndpoint>();
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		long reply = 0;
		try {
			reply = getByEid(0).queryUint32Endpoint(); // Device Descriptor
		} catch(Hexabus.HexabusException e) {
			if(e.getMessage().substring(1,21).equals("Error packet received")) {
				endpoints = oldEndpoints;
				//TODO rethrow?
				return null;
			}
		}
		boolean moreEids = true;
		int eidOffset = 0;
		while(moreEids) {
			for(int i = 1; i<32;i++) {
				reply = reply >> 1;
				if((reply & 1) > 0) {
					HexabusEndpointQueryPacket epquery = new HexabusEndpointQueryPacket(i+eidOffset);
					int port = epquery.sendPacket(address);
					HexabusPacket packet = Hexabus.receivePacket(port);

					switch(packet.getPacketType()) {
						case ERROR:
							throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
						case EPINFO:
							HexabusEndpointInfoPacket epinfo = (HexabusEndpointInfoPacket) packet;
							Hexabus.DataType dataType = epinfo.getDataType();
							String description = "";
							description = epinfo.getDescription();
							addEndpoint(i, dataType, description);
							break;
						default:
							throw new Hexabus.HexabusException("Unexpected reply received");
					}
				}
			}

			eidOffset+=32;
			HexabusPacket packet = new HexabusQueryPacket(eidOffset);
			int port = packet.sendPacket(address);
			// Receive reply
			packet = Hexabus.receivePacket(port);
			switch(packet.getPacketType()) {
				case ERROR:
					moreEids = false;
					break;
				case INFO:
					reply = ((HexabusInfoPacket) packet).getUint32();
					addEndpoint(eidOffset, Hexabus.DataType.UINT32, "Hexabus device descriptor");
					break;
				default:
					throw new Hexabus.HexabusException("Unexpected reply received");
			}
		}

		return endpoints;
	}

}
