package de.fraunhofer.itwm.hexabus;

import java.nio.ByteBuffer;

public class HexabusQueryPacket extends HexabusPacket {
	private byte eid;

	public HexabusQueryPacket(int eid) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.QUERY;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
	}	

	protected byte[] buildPacket() {
		byte[] packetData = new byte[packetType.getBaseLength()-2];
		ByteBuffer buffer = super.buildPacket(packetData);
		buffer.put(eid);
		return packetData;
	}
		
}
