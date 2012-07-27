package de.fraunhofer.itwm.hexabus;

import java.nio.ByteBuffer;

public class HexabusErrorPacket extends HexabusPacket {
	private byte errorCode;

	public HexabusErrorPacket(Hexabus.ErrorCode errorCode) {
		this.packetType = Hexabus.PacketType.ERROR;
		this.errorCode = (byte) errorCode.convert();
	}	

	public HexabusErrorPacket(byte errorCode) {
		this.packetType = Hexabus.PacketType.ERROR;
		this.errorCode = errorCode;
	}	

	public Hexabus.ErrorCode getErrorCode() {
		return Hexabus.getErrorCode(errorCode);
	}

	protected byte[] buildPacket() {
		byte[] packetData = new byte[packetType.getBaseLength()-2];
		ByteBuffer buffer = super.buildPacket(packetData);
		buffer.put(errorCode);
		return packetData;
	}
		
}
