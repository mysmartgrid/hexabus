package de.fraunhofer.itwm.hexabus;

import java.io.IOException;
import java.sql.Timestamp;
import java.util.Calendar;


public class HexabusEndpoint {
	private HexabusDevice device;
	private int eid;
	private Hexabus.DataType dataType;
	private String description;

	public HexabusEndpoint(HexabusDevice device, int eid, Hexabus.DataType dataType, String description) {
		this.device = device;
		this.eid = eid;
		this.dataType = dataType;
		this.description = description;
	}

	public HexabusEndpoint(HexabusDevice device, int eid, Hexabus.DataType dataType) {
		this.device = device;
		this.eid = eid;
		this.dataType = dataType;
		this.description = "";
	}

	public void writeEndpoint(boolean value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.BOOL) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getIP());
	}

	public void writeEndpoint(String value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.STRING) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getIP());
	}

	public void writeEndpoint(short value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT8) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getIP());
	}

	public void writeEndpoint(float value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.FLOAT) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getIP());
	}

	public void writeEndpoint(long value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT32) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getIP());
	}

	public void writeEndpoint(Timestamp value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.TIMESTAMP) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getIP());
	}

	public void writeEndpoint(Calendar value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.DATETIME) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusWritePacket(eid, value);
		packet.sendPacket(device.getIP());
	}

	// TODO use setSoTimeout
	public boolean queryEndpoint(boolean value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.BOOL) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getIP());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getValue(value);
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public String queryEndpoint(String value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.STRING) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getIP());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getValue(value);
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public short queryEndpoint(short value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT8) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getIP());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getValue(value);
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public float queryEndpoint(float value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.FLOAT) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getIP());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getValue(value);
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public long queryEndpoint(long value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.UINT32) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getIP());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getValue(value);
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public Timestamp queryEndpoint(Timestamp value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.TIMESTAMP) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getIP());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getValue(value);
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}

	public Calendar queryEndpoint(Calendar value) throws Hexabus.HexabusException, IOException {
		if(dataType != Hexabus.DataType.DATETIME) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected");
		}
		HexabusPacket packet = new HexabusQueryPacket(eid);
		int port = packet.sendPacket(device.getIP());
		// Receive reply
		packet = Hexabus.receivePacket(port);
		switch(packet.getPacketType()) {
			case ERROR:
				throw new Hexabus.HexabusException("Error packet received: "+((HexabusErrorPacket) packet).getErrorCode());
			case INFO:
				return ((HexabusInfoPacket) packet).getValue(value);
			default:
				throw new Hexabus.HexabusException("Unexpected reply received");
		}
	}
}
