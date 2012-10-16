import de.fraunhofer.itwm.hexabus.*;
import java.net.InetAddress;

public class ExampleHexabusServer {
	public static void main(String[] args) {
		try{
			HexabusServer server = new HexabusServer();
			server.start();
			server.register(new ExampleHexabusListener());
			HexabusWritePacket packet = new HexabusWritePacket(1, true);
			for(int i=0; i<150; i++) {
				packet.sendPacket(InetAddress.getLocalHost());
			}
			Thread.sleep(10);
			server.shutdown();
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
}
