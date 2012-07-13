package de.fraunhofer.itwm.hexabus;

import java.net.InetAddress;
import java.util.Collection;
import java.util.ArrayList;
import java.io.IOException;

public class HexabusDevice {
	private InetAddress address;
	private ArrayList<HexabusEndpoint> endpoints;

	/**
	 * @param address The InetAddress of the device
	 */
	public HexabusDevice(InetAddress address) {
		this.address = address;
		endpoints = new ArrayList<HexabusEndpoint>();
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
	}
	//TODO constructor with string

	/**
	 * Creates an Endpoint with specified EID, data type and description
	 * and adds it to the endpoints associated with the device.
	 *
	 * @param eid The EID of the new endpoint
	 * @param datatype The data type of the new endpoint
	 * @param description A description of the new endpoint
	 * @return The newly created endpoint
	 */
	//TODO check if eid exists
	public HexabusEndpoint addEndpoint(int eid, Hexabus.DataType datatype, String description) {
		HexabusEndpoint endpoint = new HexabusEndpoint(this, eid, datatype, description);
		endpoints.add(endpoint);
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
		endpoints.add(endpoint);
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
	public ArrayList<HexabusEndpoint> getEndpoints() {
		return endpoints;
	}

	/**
	 * Sends an endpoint query to the device and its endpoints.
	 * Replaces the currently asscociated endpoints with the result.
	 */
	public ArrayList<HexabusEndpoint> fetchEndpoints() throws Hexabus.HexabusException, IOException {
		ArrayList<HexabusEndpoint> oldEndpoints = endpoints;
		endpoints = new ArrayList<HexabusEndpoint>();
		addEndpoint(0, Hexabus.DataType.UINT32, "Hexabus device descriptor");
		try {
		long reply = endpoints.get(0).queryUint32Endpoint(); // First Endpoint is Device Descriptor
		} catch(HexabusException e) {
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

			HexabusPacket packet = new HexabusQueryPacket(eid);
			int port = packet.sendPacket(device.getInetAddress());
			// Receive reply
			packet = Hexabus.receivePacket(port);
			switch(packet.getPacketType()) {
				case ERROR:
					moreEids = false;
					break;
				case INFO:
					reply = ((HexabusInfoPacket) packet).getUint32();
					eidOffset+=32;
					addEndpoint(eidOffset, Hexabus.DataType.UINT32, "Hexabus device descriptor");
					break;
				default:
					throw new Hexabus.HexabusException("Unexpected reply received");
			}
		}

		return endpoints;
	}

}
