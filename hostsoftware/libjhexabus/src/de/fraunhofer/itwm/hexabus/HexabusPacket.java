package de.fraunhofer.itwm.hexabus;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.net.InetAddress;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.io.IOException;

public abstract class HexabusPacket {
	protected InetAddress sourceAddress;
	protected int sourcePort;
	protected Hexabus.PacketType packetType;
	protected byte[] crc;

	/**
	 * Sends the packet to specified address on port { @value Hexabus#PORT }
	 *
	 * @param address The InetAddress where the packet should be sent to
	 * @return The source port on which the packet has been sent out
	 */
	public int sendPacket(InetAddress address) throws Hexabus.HexabusException, IOException {
		byte[] data = buildPacket();
		crc = crc(data);
		byte[] packetData = new byte[data.length+crc.length];
		ByteBuffer buffer = ByteBuffer.wrap(packetData);
		buffer.order(ByteOrder.BIG_ENDIAN);
		buffer.put(data);
		buffer.put(crc);
		// send packet
		DatagramSocket socket = new DatagramSocket();
		DatagramPacket packet = new DatagramPacket(packetData, packetData.length, 
				address, Hexabus.PORT);
		socket.send(packet);
		int sourcePort = socket.getLocalPort();
		socket.close();
		return sourcePort;
	}

	/**
	 * Sends the packet to specified address on specified port
	 *
	 * @param address The InetAddress where the packet to should be sent to
	 * @param port The port where the packet should be sent to
	 * @return The source port on which the packet has been sent out
	 */
	public int sendPacket(InetAddress address, int port) throws Hexabus.HexabusException, IOException {
		byte[] data = buildPacket();
		crc = crc(data);
		byte[] packetData = new byte[data.length+crc.length];
		ByteBuffer buffer = ByteBuffer.wrap(packetData);
		buffer.order(ByteOrder.BIG_ENDIAN);
		buffer.put(data);
		buffer.put(crc);
		// send packet
		DatagramSocket socket = new DatagramSocket();
		DatagramPacket packet = new DatagramPacket(packetData, packetData.length, 
				address, port);
		socket.send(packet);
		socket.close();
		return socket.getLocalPort();
	}
	
	/**
	 * @return The source address where a packet came from
	 */
	public InetAddress getSourceAddress() {
		return sourceAddress;
	}

	protected void setSourceAddress(InetAddress address) {
		sourceAddress = address;
	}
	
	/**
	 * @return The source port where a packet came from
	 */
	public int getSourcePort() {
		return sourcePort;
	}

	protected void setSourcePort(int port) {
		sourcePort = port;
	}

	/**
	 * @return The packet type of this packet
	 */
	public Hexabus.PacketType getPacketType() {
		return packetType;
	}

	public byte[] getCRC() {
		return crc;
	}

	protected void setCRC(byte[] crc) throws Hexabus.HexabusException {
		if(crc.length > 2) {
			throw new Hexabus.HexabusException("CRC invalid. 2 bytes expected");
		}
		this.crc = crc;
	}
	
	/**
	 * Checks if the CRC of the packet is correct.
	 *
	 * @return <code>true</code> if the CRC is correct, <code>false</code> otherwise
	 */
	public boolean checkCRC() throws Hexabus.HexabusException {
		byte[] checked = crc(buildPacket());
		return (checked[0] == crc[0]) && (checked[1] == crc[1]);
	}

	protected abstract byte[] buildPacket() throws Hexabus.HexabusException;

	protected ByteBuffer buildPacket(byte[] packetData) {
		ByteBuffer buffer = ByteBuffer.wrap(packetData);
		buffer.order(ByteOrder.BIG_ENDIAN);
		// Header = "HX0B"
		buffer.put(Hexabus.HEADER);

		// Type
		buffer.put(packetType.convert());

		// Flags?
		buffer.put((byte) 0x00);
		return buffer;
	}

	protected byte[] crc(byte[] bytes) {

		char crc = 0x00; // char is an unsigned 16 bit value
		for(byte b : bytes) {
			crc ^= (b & 0xff);
			crc  = (char) ((crc >> 8) | (crc << 8));
			crc ^= (crc & 0xff00) << 4;
			crc ^= (crc >> 8) >> 4;
			crc ^= (crc & 0xff00) >> 5;
		}
		//int print = crc & 0xffff;
		//System.out.println(Integer.toHexString(print));
		byte firstByte = (byte) ((crc & 0xff00) >> 8);
		byte secondByte = (byte) (crc & 0x00ff);
		return new byte[] {firstByte, secondByte};
	}
}
