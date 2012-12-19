import de.fraunhofer.itwm.hexabus.*;

public class ExampleHexabusListener extends Hexabus.HexabusListener {
	public void handlePacket(HexabusPacket packet) {
		if(packet.getPacketType()!=Hexabus.PacketType.INFO){
			System.err.println(packet.getPacketType());
			return;
		}
		HexabusInfoPacket infoPacket = (HexabusInfoPacket) packet;
		Hexabus.DataType type = infoPacket.getDataType();
		String value = "";
		try {
			switch(type) {
				case BOOL:
					value = new Boolean(infoPacket.getBool()).toString();
					break;
				case UINT8:
					value = new Short(infoPacket.getUint8()).toString();
					break;
				case UINT32:
					value = new Long(infoPacket.getUint32()).toString();
					break;
				case FLOAT:
					value = new Float(infoPacket.getFloat()).toString();
					break;
				case STRING:
					value = infoPacket.getString();
					break;
				case TIMESTAMP:
					value = infoPacket.getTimestamp().toString();
					break;
				case DATETIME:
					value = infoPacket.getDatetime().toString();
					break;
				default:
					break;
			}
		} catch(Hexabus.HexabusException e) {
			// Do some excpetion handling
		}

		System.out.println("IP: "+infoPacket.getSourceAddress()+", Eid: "+infoPacket.getEid()+", Data type: "+type+", Value: "+value);
	}
}
