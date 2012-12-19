package de.fraunhofer.itwm.hexabus;

import java.nio.ByteBuffer;

public class HexabusEndpointInfoPacket extends HexabusInfoPacket {
	public HexabusEndpointInfoPacket(long eid, Hexabus.DataType dataType, byte[] payload) throws Hexabus.HexabusException {
		super(eid, Hexabus.DataType.STRING, payload);
		this.packetType = Hexabus.PacketType.EPINFO;
		this.dataType = dataType;
	}

	public String getDescription() {
		return Hexabus.parseString(payload);
	}

	protected byte[] buildPacket() throws Hexabus.HexabusException {
		byte[] packetData = new byte[packetType.getBaseLength()+Hexabus.DataType.STRING.getSize()-2];
		ByteBuffer buffer = super.buildPacket(packetData);

		byte[] eidBytes = new byte[] {(byte) ((eid & 0xFF000000L) >> 24)
			,(byte) ((eid & 0x00FF0000L) >> 16)
			,(byte) ((eid & 0x0000FF00L) >> 8)
			,(byte) (eid & 0x000000FFL)};
		buffer.put(eidBytes);
		buffer.put(dataType.convert());
		buffer.put(payload);

		return packetData;
	}
}
