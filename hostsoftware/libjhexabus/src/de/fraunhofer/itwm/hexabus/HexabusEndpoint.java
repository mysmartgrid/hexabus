package de.fraunhofer.itwm.hexabus;

import java.io.IOException;


public class HexabusEndpoint {
	private HexabusDevice device;
	private long eid;
	private Hexabus.DataType dataType;
	private String description;

	public HexabusEndpoint(HexabusDevice device, long eid, Hexabus.DataType dataType, String description) {
		this.device = device;
		this.eid = eid;
		this.dataType = dataType;
		this.description = description;
	}

	public HexabusEndpoint(HexabusDevice device, long eid, Hexabus.DataType dataType) {
		this.device = device;
		this.eid = eid;
		this.dataType = dataType;
		this.description = "";
	}

	public long getEid() {
		return eid;
	}

	public String getDescription() {
		return description;
	}

	public Hexabus.DataType getDataType() {
		return dataType;
	}

	public String toString() {
		return eid+" "+description+" "+dataType;
	}

	public void writeEndpoint(boolean value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.BOOL) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getInetAddress());
	}

	public void writeEndpoint(String value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.STRING) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getInetAddress());
	}

	public void writeEndpoint(short value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT8) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getInetAddress());
	}

	public void writeEndpoint(float value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.FLOAT) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getInetAddress());
	}

	public void writeEndpoint(long value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT32) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getInetAddress());
	}

	public void writeEndpoint(HexabusTimestamp value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.TIMESTAMP) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getInetAddress());
	}

	public void writeEndpoint(HexabusDatetime value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.DATETIME) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getInetAddress());
	}

	// TODO use setSoTimeout
	public boolean queryBoolEndpoint() throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.BOOL) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getInetAddress());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getBool();
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public String queryStringEndpoint() throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.STRING) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getInetAddress());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getString();
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public short queryUint8Endpoint() throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT8) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getInetAddress());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getUint8();
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public float queryFloatEndpoint() throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.FLOAT) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getInetAddress());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getFloat();
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public long queryUint32Endpoint() throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT32) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getInetAddress());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getUint32();
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public HexabusTimestamp queryTimestampEndpoint() throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.TIMESTAMP) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getInetAddress());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getTimestamp();
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public HexabusDatetime queryDatetimeEndpoint() throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.DATETIME) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getInetAddress());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getDatetime();
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}
}
