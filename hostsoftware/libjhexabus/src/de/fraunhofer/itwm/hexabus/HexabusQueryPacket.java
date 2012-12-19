package de.fraunhofer.itwm.hexabus;

import java.nio.ByteBuffer;

public class HexabusQueryPacket extends HexabusPacket {
	private long eid;

	public HexabusQueryPacket(long eid) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.QUERY;
		if(eid>=(2^32)) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = eid;
	}	

	protected byte[] buildPacket() {
		byte[] packetData = new byte[packetType.getBaseLength()-2];
		ByteBuffer buffer = super.buildPacket(packetData);
		byte[] eidBytes = new byte[] {(byte) ((eid & 0xFF000000L) >> 24)
			,(byte) ((eid & 0x00FF0000L) >> 16)
			,(byte) ((eid & 0x0000FF00L) >> 8)
			,(byte) (eid & 0x000000FFL)};
		buffer.put(eidBytes);
		return packetData;
	}

	public long getEid() {
		return eid;
	}
		
}
