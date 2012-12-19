package de.fraunhofer.itwm.hexabus;
public class HexabusEndpointQueryPacket extends HexabusQueryPacket {
	private long eid;
	public HexabusEndpointQueryPacket(long eid) throws Hexabus.HexabusException {
		super(eid);
		this.packetType = Hexabus.PacketType.EPQUERY;
	}	
}
