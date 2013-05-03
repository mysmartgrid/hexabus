package de.fraunhofer.itwm.hexabus;

import java.net.InetAddress;
import java.util.TreeMap;
import java.util.Map;
import java.io.IOException;
import java.net.UnknownHostException;

public class HexabusDevice {
	private InetAddress address;
	private String name;
	private TreeMap<Long,HexabusEndpoint> endpoints;

	/**
	 * @param address The InetAddress of the device
	 */
	public HexabusDevice(InetAddress address) {
		this.address = address;
		this.name = "";
		endpoints = new TreeMap<Long,HexabusEndpoint>();
		try{
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		}catch(Hexabus.HexabusException e){
			//TODO
		}
	}

	/**
	 * @param address The InetAddress of the device
	 * @param name The name of the device
	 */
	public HexabusDevice(InetAddress address, String name) {
		this.address = address;
		this.name = name;
		endpoints = new TreeMap<Long,HexabusEndpoint>();
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
		endpoints = new TreeMap<Long,HexabusEndpoint>();
		try{
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		}catch(Hexabus.HexabusException e){
			//TODO
		}
	}

	public HexabusDevice(String address, String name) {
		this.name = name;
		try{
		this.address = InetAddress.getByName(address);
		}catch(UnknownHostException e) {
			//TODO
		}
		endpoints = new TreeMap<Long,HexabusEndpoint>();
		try{
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		}catch(Hexabus.HexabusException e){
			//TODO
		}
	}

	public void setName(String name) {
		this.name = name;
	}

	public String getName() {
		return name;
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
	public HexabusEndpoint addEndpoint(long eid, Hexabus.DataType datatype, String description) throws Hexabus.HexabusException {
		if(endpoints.containsKey(new Long(eid))) {
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
	public HexabusEndpoint addEndpoint(long eid, Hexabus.DataType datatype) {
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
	public TreeMap<Long, HexabusEndpoint> getEndpoints() {
		return endpoints;
	}

	public HexabusEndpoint getByEid(long eid) throws Hexabus.HexabusException {
		if(eid>=Math.pow(2,32)) {
			throw new Hexabus.HexabusException("EID too large");
		}
		
		for(Map.Entry<Long, HexabusEndpoint> entry : endpoints.entrySet()) {
			if(entry.getKey().longValue() == eid) {
				return entry.getValue();
			}
		}

		throw new Hexabus.HexabusException("EID not found");
	}

	/**
	 * Sends an endpoint query to the device and its endpoints.
	 * Replaces the currently asscociated endpoints with the result.
	 */
	//TODO broken
	public TreeMap<Long, HexabusEndpoint> fetchEndpoints() throws Hexabus.HexabusException, IOException {
		TreeMap<Long, HexabusEndpoint> newEndpoints = new TreeMap<Long,HexabusEndpoint>();
		HexabusEndpoint deviceDescriptor = new HexabusEndpoint(this, 0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		newEndpoints.put(0L, deviceDescriptor);
		long reply = 0;
		try {
			reply = deviceDescriptor.queryUint32Endpoint();
		} catch(Hexabus.HexabusException e) {
			if(e.getMessage().substring(1,21).equals("Error packet received")) {
				System.err.println(e.getMessage());
				System.err.println("Error while querying device descriptor. Aborting");
				return null;
			}
		}
		boolean moreEids = true;
		int eidOffset = 0;
		while(moreEids) {
			moreEids = false; // Until more eids is implemented
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
							newEndpoints.put(new Long(i), new HexabusEndpoint(this, i, dataType, description));
							break;
						default:
							throw new Hexabus.HexabusException("Unexpected reply received");
					}
				}
			}

			/* Not yet implemented
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
			*/
		}
		endpoints = newEndpoints;

		return endpoints;
	}

}
