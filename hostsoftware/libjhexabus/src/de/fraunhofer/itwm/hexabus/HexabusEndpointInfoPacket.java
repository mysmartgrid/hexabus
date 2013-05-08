package de.fraunhofer.itwm.hexabus;

import java.nio.ByteBuffer;

public class HexabusEndpointInfoPacket extends HexabusInfoPacket {
	public HexabusEndpointInfoPacket(int eid, Hexabus.DataType dataType, byte[] payload) throws Hexabus.HexabusException {
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

		buffer.put(eid);
		buffer.put(dataType.convert());
		buffer.put(payload);

		return packetData;
	}
}
