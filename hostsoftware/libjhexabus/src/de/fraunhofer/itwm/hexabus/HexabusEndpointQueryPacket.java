package de.fraunhofer.itwm.hexabus;
public class HexabusEndpointQueryPacket extends HexabusQueryPacket {
	private int eid;
	public HexabusEndpointQueryPacket(int eid) throws Hexabus.HexabusException {
		super(eid);
		this.packetType = Hexabus.PacketType.EPQUERY;
	}	
}
