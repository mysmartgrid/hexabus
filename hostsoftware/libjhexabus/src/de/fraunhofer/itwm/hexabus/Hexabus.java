package de.fraunhofer.itwm.hexabus;

import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.io.IOException;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.sql.Timestamp;
import java.util.Calendar;
import java.util.GregorianCalendar;

import java.util.Map;
import java.util.HashMap;


//TODO document exceptions, maybe specify them more
//TODO maxeid
public class Hexabus {
	/** Hexabus port { @value } */
	public final static int PORT = 61616;
	/** Hexabus packet header */
	public final static byte[] HEADER = new byte[] {(byte) 0x48, (byte) 0x58, (byte) 0x30, (byte) 0x42};

	/** Listener have to extend this class to get packets from the HexabusServer */
	public static abstract class HexabusListener {
		private HexabusServer server;

		/**
		 * Registers listener with a HexabusServer
		 * @param server The HexabusServer to register with
		 */
		public void register(HexabusServer server) {
			server.register(this);
			this.server = server;
		}

		/**
		 * Unregisters listener from a HexabusServer
		 */
		public void unregister() {
			if(server!=null) {
				server.unregister(this);
			}
		}

		/** This method gets called by the server whenever it receives a packet. */
		public abstract void handlePacket(HexabusPacket packet);
	}
	
	/**
	 * Receives packet on specified port.
	 * Will block until specified timeout exceeds.
	 * 
	 * @param port The port on which the packet should be received
	 * @param timeout The timeout in milliseconds after which receive attempt stops with { @link SocketTimeoutException }
	 * @return The received and parsed hexabus packet
	 */
	public static HexabusPacket receivePacket(int port, int timeout) throws HexabusException, IOException {
		DatagramSocket socket = new DatagramSocket(port);
		socket.setSoTimeout(timeout);
		byte[] data = new byte[138]; // Largest packet: 128string info/write packet
		DatagramPacket packet = new DatagramPacket(data, data.length);
		socket.receive(packet);
		socket.close();
		return parsePacket(packet);
	}

	/**
	 * Receives packet on specified port.
	 * Will block until a packet is received.
	 * 
	 * @param port The port on which the packet should be received
	 * @return The received and parsed hexabus packet
	 */
	public static HexabusPacket receivePacket(int port) throws HexabusException, IOException {
		DatagramSocket socket = new DatagramSocket(port);
		byte[] data = new byte[138]; // Largest packet: 128string info/write packet
		DatagramPacket packet = new DatagramPacket(data, data.length);
		socket.receive(packet);
		socket.close();
		return parsePacket(packet);
	}

	/**
	 * Parses a { @link DatagramPacket } and returns extracted { @link HexabusPacket }
	 *
	 * @param packet The packet that should be parsed
	 * @return The extracted hexabus packet
	 */
	public static HexabusPacket parsePacket(DatagramPacket packet) throws HexabusException {
		InetAddress address = packet.getAddress();
		int sourcePort = packet.getPort();
		byte[] data = packet.getData();
		HexabusPacket parsedPacket;
		PacketType packetType = getPacketType(data[4]);
		DataType dataType;
		int eid;
		// ByteBuffer to get value of info/write packets
		ByteBuffer buffer = ByteBuffer.wrap(data);
		buffer.order(ByteOrder.BIG_ENDIAN);
		buffer.position(8);
		byte[] payload;
		switch(packetType) {
			case ERROR:
				parsedPacket = new HexabusErrorPacket(data[6]);
				break;
			case INFO:
				dataType = getDataType(data[7]);
				eid = data[6];
				
				payload = new byte[dataType.getSize()];
				buffer.get(payload);
				parsedPacket = new HexabusInfoPacket(eid, dataType, payload);
				break;
			case QUERY:
				parsedPacket = new HexabusQueryPacket(data[6]);
				break;
			case WRITE:
				dataType = getDataType(data[7]);
				eid = data[6];
				
				payload = new byte[dataType.getSize()];
				buffer.get(payload);
				parsedPacket = new HexabusWritePacket(eid, dataType, payload);
				break;
			case EPINFO:
				dataType = getDataType(data[7]);
				eid = data[6];

				payload = new byte[Hexabus.DataType.STRING.getSize()];
				buffer.get(payload);
				parsedPacket = new HexabusEndpointInfoPacket(eid, dataType, payload);
				break;
			case EPQUERY:
				parsedPacket = new HexabusEndpointQueryPacket(data[6]);
				break;
			default:
				throw new HexabusException("Unknown packet type");
		}
		int packetLength = parsedPacket.getPacketType().getBaseLength();
		parsedPacket.setCRC(new byte[] {data[packetLength-2], data[packetLength-1]});
		parsedPacket.setSourceAddress(address);
		parsedPacket.setSourcePort(sourcePort);
		return parsedPacket;
	}

	/**
	 * Enum for packet types
	 */
	public enum PacketType implements EnumConverter {
		ERROR(0, 9), INFO(1, 10), QUERY(2, 9), WRITE(4, 10),
			EPINFO(9,10), EPQUERY(0x0A, 9);
		PacketType(int value, int baseLength) {
			this.value = (byte) value;
			this.baseLength = baseLength;
		}
		private final byte value;
		private final int baseLength;
		public byte convert() {
			return value;
		}

		public int getBaseLength() {
			return baseLength;
		}
	}

	/**
	 * Enum for data types
	 */
	public enum DataType implements EnumConverter {
		BOOL(1, 1), UINT8(2, 1), UINT32(3, 4), DATETIME(4, 8),
			FLOAT(5, 4), STRING(6, 128), TIMESTAMP(7, 4);
		DataType(int value, int size) {
			this.value = (byte) value;
			this.size = size;
		}
		private final byte value;
		private final int size;
		public byte convert() {
			return value;
		}

		public int getSize() {
			return size;
		}
	}

	/**
	 * Enum for error codes
	 */
	public enum ErrorCode implements EnumConverter {
		UNKNOWNEID(1), WRITEREADONLY(2),
			CRCFAILED(3), DATATYPE(4);
		private final byte value;
		ErrorCode(int value) {
			this.value = (byte) value;
		}
		public byte convert() {
			return value;
		}
	}


	// TODO: error handling in parse functions

	/**
	 * Parses a byte array and extracts a boolean value from the first field.
	 * Expects the value of the field to be either 0 or 1.
	 *
	 * @param value The byte array that should be parsed
	 * @return The extracted value
	 */
	public static boolean parseBool(byte[] value) throws HexabusException {
		//if(value.length > 1) {...
		switch(value[0]) {
			case 0:
				return false;
			case 1:
				return true;
			default:
				throw new HexabusException("Wrong data type. BOOL expected");
		}
	}

	/**
	 * Parses a byte array and extracts a short value representing a uint_8 value.
	 *
	 * @param value The byte array that should be parsed
	 * @return The extracted value
	 */
	public static short parseUint8(byte[] value) {
		return (short) (value[0] & 0xFF);
	}

	/**
	 * Parses a byte array and extracts a long value representing a uint_32 value.
	 *
	 * @param value The byte array that should be parsed
	 * @return The extracted value
	 */
	public static long parseUint32(byte[] value) {
		return ((long) (value[0] << 24
			| value[1] << 16
			| value[2] << 8
			| value[3]))
				& 0xFFFFFFFFL;
	}

	/**
	 * Parses a byte array and extracts a float value.
	 *
	 * @param value The byte array that should be parsed
	 * @return The extracted value
	 */
	public static float parseFloat(byte[] value) {
		ByteBuffer buffer = ByteBuffer.wrap(value);
		buffer.order(ByteOrder.BIG_ENDIAN);		
		return buffer.getFloat();

	}

	/**
	 * Parses a byte array and extracts a String value.
	 *
	 * @param value The byte array that should be parsed
	 * @return The extracted value
	 */
	public static String parseString(byte[] value) {
		return new String(value);
	}

	/**
	 * Parses a byte array and extracts a { @link Timestamp } value.
	 *
	 * @param value The byte array that should be parsed
	 * @return The extracted value
	 */
	public static Timestamp parseTimestamp(byte[] value) {
		long timestamp = ((long) (value[0] << 24
			| value[1] << 16
			| value[2] << 8
			| value[3]))
				& 0xFFFFFFFFL;
		return new Timestamp(timestamp);
	}

	/**
	 * Parses a byte array and extracts a { @link Calendar } value representing a datetime value.
	 *
	 * @param value The byte array that should be parsed
	 * @return The extracted value
	 */
	public static Calendar parseDatetime(byte[] value) {
		ByteBuffer buffer = ByteBuffer.wrap(value);
		buffer.order(ByteOrder.BIG_ENDIAN);		
		int hour = buffer.get();
		int minute = buffer.get();
		int second = buffer.get();
		int day = buffer.get();
		int month = buffer.get()-1;
		int year = buffer.get() << 8
			| buffer.get();
		int weekday = buffer.get();

		Calendar calendar = new GregorianCalendar();
		calendar.add(Calendar.HOUR_OF_DAY, hour);
		calendar.add(Calendar.MINUTE, minute);
		calendar.add(Calendar.SECOND, second);
		calendar.add(Calendar.DAY_OF_MONTH, day);
		calendar.add(Calendar.MONTH, month);
		calendar.add(Calendar.YEAR, year);
		calendar.add(Calendar.DAY_OF_WEEK, weekday);

		return calendar;
	}

	public static ErrorCode getErrorCode(byte code) {
			ReverseEnumMap<ErrorCode> reverse = new ReverseEnumMap<ErrorCode>(ErrorCode.class);
		return reverse.get(code);
	}

	public static PacketType getPacketType(byte type) {
			ReverseEnumMap<PacketType> reverse = new ReverseEnumMap<PacketType>(PacketType.class);
		return reverse.get(type);
	}

	public static DataType getDataType(byte type) {
			ReverseEnumMap<DataType> reverse = new ReverseEnumMap<DataType>(DataType.class);
		return reverse.get(type);
	}
		

	public static class HexabusException extends Exception {
		public HexabusException(String s) {
			super(s);
		}
	}

	// See: http://www.javaspecialists.co.za/archive/Issue113.html
	private interface EnumConverter {
		public byte convert();
	}
	private static class ReverseEnumMap<V extends Enum<V> & EnumConverter> {
		private Map<Byte, V> map = new HashMap<Byte, V>();
		public ReverseEnumMap(Class<V> valueType) {
			for (V v : valueType.getEnumConstants()) {
				map.put(v.convert(), v);
			}
		}

		public V get(byte num) {
			return map.get(num);
		}
	}
}
