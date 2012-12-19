package de.fraunhofer.itwm.hexabus.android;

import java.io.IOException;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.util.Log;

import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.HexabusDevice;
import de.fraunhofer.itwm.hexabus.HexabusPacket;
import de.fraunhofer.itwm.hexabus.HexabusServer;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusListener;

public class HexabusService extends Service {
	private static final String TAG = "HexabusService";
	private HexabusServer server;
	private HexabusServiceListener listener;
	private WifiManager.MulticastLock multicastLock;
	
	public void onCreate() {
		super.onCreate(); 
		server = new HexabusServer();
		listener = new HexabusServiceListener();
		listener.register(server);
	}
	
	// Deprecated as of API level 5, left for backwards compatibility
	@Override
	public void onStart(Intent intent, int startId) {
		WifiManager wifiManager = (WifiManager)getSystemService(Context.WIFI_SERVICE);
		multicastLock = wifiManager.createMulticastLock("HexabusService");
		multicastLock.setReferenceCounted(true);
		multicastLock.acquire();
		Log.d(TAG, "multicastLock.isHeld() = " + multicastLock.isHeld());
		Log.d(TAG, "multicastLock.toString() = " + multicastLock.toString()); 
		server.start();
	}
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		WifiManager wifiManager = (WifiManager)getSystemService(Context.WIFI_SERVICE);
		multicastLock = wifiManager.createMulticastLock("HexabusService");
		multicastLock.setReferenceCounted(true);
		multicastLock.acquire();
		Log.d(TAG, "multicastLock.isHeld() = " + multicastLock.isHeld());
		Log.d(TAG, "multicastLock.toString() = " + multicastLock.toString()); 
		server.start();
		Log.d(TAG, "HexabusServer started");
		
		// Continue until explicitly stopped
		return START_STICKY;
	}
	
	public void onDestroy() {
		server.shutdown();
		multicastLock.release();
		Log.d(TAG, "HexabusServer stopped");
		super.onDestroy();
	}

	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return null;
	}
	
	private class HexabusServiceListener extends HexabusListener {
		@Override
		public void handlePacket(HexabusPacket packet) {
			Log.d(TAG, "Packet received: " + packet.getPacketType());
			HexabusDevice device = new HexabusDevice(packet.getSourceAddress());
			/*try {
			device.fetchEndpoints();
			Log.d(TAG, "Endpoints fetched");
			Log.d(TAG, device.getEndpoints().toString());
			} catch(IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (HexabusException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}*/
		}
	}

}
