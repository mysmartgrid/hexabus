import de.fraunhofer.itwm.hexabus.*;
import java.net.InetAddress;
import java.sql.Timestamp;
import java.util.Calendar;
import java.util.GregorianCalendar;

public class Example {
	public static void main(String[] args) {
		try {
			/*Calendar calendar = new GregorianCalendar();
			HexabusDatetime dt = new HexabusDatetime(calendar);
			System.out.println(dt);*/
			//HexabusWritePacket iPac = new HexabusWritePacket(1, true);
			//iPac.sendPacket(InetAddress.getByName("fd01:2:0:0:50:c4ff:fe04:8383"));
			//InetAddress address = InetAddress.getByName("ff05::114"); // Insert address of your hexabus device
			//iPac.broadcastPacket();

			InetAddress address = InetAddress.getByName("fd01:2:0:0:50:c4ff:fe04:8383"); // Insert address of your hexabus device
			//InetAddress address = InetAddress.getByName("ff05::114"); // Insert address of your hexabus device
			//HexabusDevice device1 = new HexabusDevice(address1);
			HexabusDevice device = new HexabusDevice(address);
			//device.fetchEndpoints();
			//HexabusEndpoint relais1 = device1.addEndpoint(1, Hexabus.DataType.BOOL);
			HexabusEndpoint relais = device.addEndpoint(1, Hexabus.DataType.BOOL);
			boolean state = relais.queryBoolEndpoint();
			HexabusWritePacket iPac = new HexabusWritePacket(1, !state);
			iPac.broadcastPacket();
			//relais.writeEndpoint(!state);
			//relais.writeEndpoint(false);
		} catch(Exception e) {
			// Do some exception handling
			e.printStackTrace();
		}
	}
}
