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

public class Hexabus {
	public final static int PORT = 61616;
	public final static byte[] HEADER = new byte[] {(byte) 0x48, (byte) 0x58, (byte) 0x30, (byte) 0x42};

	//public HexabusDevice getDevices(InetAddress ip);
	public static HexabusPacket receivePacket(int port) throws HexabusException, IOException {
		DatagramSocket socket = new DatagramSocket(port);
		byte[] data = new byte[138]; // Largest packet: 128string info/write packet
		DatagramPacket packet = new DatagramPacket(data, data.length);
		socket.receive(packet);
		socket.close();
		InetAddress address = packet.getAddress();
		int sourcePort = packet.getPort();
		HexabusPacket hxbPacket;
		PacketType packetType = getPacketType(data[4]);
		DataType dataType;
		int eid;
		//ByteBuffer to get value of info/write packets
		ByteBuffer buffer = ByteBuffer.wrap(data);
		buffer.order(ByteOrder.BIG_ENDIAN);
		buffer.position(8);
		byte[] payload;
		switch(packetType) {
			case ERROR:
				hxbPacket = new HexabusErrorPacket(data[6]);
				break;
			case INFO:
				dataType = getDataType(data[7]);
				eid = data[6];
				
				payload = new byte[dataType.getSize()];
				buffer.get(payload);
				hxbPacket = new HexabusInfoPacket(eid, dataType, payload);
				break;
			case QUERY:
				hxbPacket = new HexabusQueryPacket(data[6]);
				break;
			case WRITE:
				dataType = getDataType(data[7]);
				eid = data[6];
				
				payload = new byte[dataType.getSize()];
				buffer.get(payload);
				hxbPacket = new HexabusWritePacket(eid, dataType, payload);
				break;
			default:
				throw new HexabusException("Unknown packet type");
		}
		int packetLength = hxbPacket.getPacketType().getBaseLength();
		hxbPacket.setCRC(new byte[] {data[packetLength-2], data[packetLength-1]});
		hxbPacket.setSourceAddress(address);
		hxbPacket.setSourcePort(sourcePort);
		return hxbPacket;
	}

	public enum PacketType implements EnumConverter {
		ERROR(0, 9), INFO(1, 10), QUERY(2, 9), WRITE(4, 10);
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

	public static boolean parseBool(byte[] value) throws HexabusException {
		switch(value[0]) {
			case 0:
				return false;
			case 1:
				return true;
			default:
				throw new HexabusException("Wrong data type. BOOL expected");
		}
	}

	// TODO: error handling in parse functions
	public static short parseUint8(byte[] value) {
		return (short) (value[0] & 0xFF);
	}

	public static long parseUint32(byte[] value) {
		return ((long) (value[0] << 24
			| value[1] << 16
			| value[2] << 8
			| value[3]))
				& 0xFFFFFFFFL;
	}

	public static float parseFloat(byte[] value) {
		ByteBuffer buffer = ByteBuffer.wrap(value);
		buffer.order(ByteOrder.BIG_ENDIAN);		
		return buffer.getFloat();

	}

	public static String parseString(byte[] value) {
		return new String(value);
	}

	public static Timestamp parseTimestamp(byte[] value) {
		long timestamp = ((long) (value[0] << 24
			| value[1] << 16
			| value[2] << 8
			| value[3]))
				& 0xFFFFFFFFL;
		return new Timestamp(timestamp);
	}

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
