import de.fraunhofer.itwm.hexabus.*;

public class ExampleHexabusListener extends Hexabus.HexabusListener {
	public void handlePacket(HexabusPacket packet) {
		HexabusWritePacket writePacket = (HexabusWritePacket) packet;
		Hexabus.DataType type = writePacket.getDataType();
		String value = "";
		try {
			switch(type) {
				case BOOL:
					value = new Boolean(writePacket.getBool()).toString();
					break;
				case UINT8:
					value = new Short(writePacket.getUint8()).toString();
					break;
				case UINT32:
					value = new Long(writePacket.getUint32()).toString();
					break;
				case FLOAT:
					value = new Float(writePacket.getFloat()).toString();
					break;
				case STRING:
					value = writePacket.getString();
					break;
				case TIMESTAMP:
					value = writePacket.getTimestamp().toString();
					break;
				case DATETIME:
					value = writePacket.getDatetime().toString();
					break;
				default:
					break;
			}
		} catch(Hexabus.HexabusException e) {
			// Do some excpetion handling
		}

		System.out.println("Eid: "+writePacket.getEid()+", Data type: "+type+", Value: "+value);
	}
}
