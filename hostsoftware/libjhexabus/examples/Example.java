import de.fraunhofer.itwm.hexabus.*;
import java.net.InetAddress;

public class Example {
	public static void main(String[] args) {
		try {
			InetAddress address = InetAddress.getByName("1313::50:c4ff:fe04:8299"); // Insert address of your hexabus device
			HexabusDevice device = new HexabusDevice(address);
			device.fetchEndpoints();
			HexabusEndpoint relais = device.getByEid(1);
			boolean state = relais.queryBoolEndpoint();
			relais.writeEndpoint(!state);
		} catch(Exception e) {
			// Do some exception handling
		}
	}
}
