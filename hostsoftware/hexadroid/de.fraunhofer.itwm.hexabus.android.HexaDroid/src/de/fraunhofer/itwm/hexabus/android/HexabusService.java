package de.fraunhofer.itwm.hexabus.android;

import java.io.IOException;
import java.io.Serializable;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.HexabusDevice;
import de.fraunhofer.itwm.hexabus.HexabusInfoPacket;
import de.fraunhofer.itwm.hexabus.HexabusPacket;
import de.fraunhofer.itwm.hexabus.HexabusServer;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusListener;

public class HexabusService extends Service {
	private static final String TAG = "HexabusService";
	private HexabusServer server;
	private HexabusServiceListener listener;
	private WifiManager.MulticastLock multicastLock;
	private ArrayList<Messenger> clients;
	private boolean started = false;
	
    static final int MSG_REGISTER = 1;
    static final int MSG_UNREGISTER = 2;
    static final int MSG_RECEIVE = 3;
    
    private InetAddress bindAddress;


	/**
     * Handler of incoming messages from clients.
     */
    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_REGISTER:
                	clients.add(msg.replyTo);
                	Log.d(TAG, "Client registered");
                    break;
                case MSG_UNREGISTER:
                	clients.remove(msg.replyTo);
                	Log.d(TAG, "Client unregistered");
               	break;
                default:
                    super.handleMessage(msg);
            }
        }
    }

    /**
     * Target we publish for clients to send messages to IncomingHandler.
     */
    final Messenger mMessenger = new Messenger(new IncomingHandler());

    /**
     * When binding to the service, we return an interface to our messenger
     * for sending messages to the service.
     */
    @Override
    public IBinder onBind(Intent intent) {
        return mMessenger.getBinder();
    }
    
    public boolean onUnbind(Intent intent) {
    	clients.clear();
    	return false;
    }
	
	
	public void onCreate() {
		super.onCreate(); 
		server = new HexabusServer();
		listener = new HexabusServiceListener();
		clients = new ArrayList<Messenger>();
	}
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		if(!started) {
			WifiManager wifiManager = (WifiManager)getSystemService(Context.WIFI_SERVICE);
			multicastLock = wifiManager.createMulticastLock("HexabusService");
			multicastLock.setReferenceCounted(true);
			multicastLock.acquire();
			Log.d(TAG, "multicastLock.isHeld() = " + multicastLock.isHeld());
			Log.d(TAG, "multicastLock.toString() = " + multicastLock.toString());
			NetworkInterface inf = null;
			
			try {
				WifiManager wifiMan = (WifiManager) getSystemService(Context.WIFI_SERVICE);
				WifiInfo wifiInf = wifiMan.getConnectionInfo();
				String wifiMacAddr = wifiInf.getMacAddress();

				InetAddress interfaceAddress = null;
				Enumeration<NetworkInterface> nets = NetworkInterface.getNetworkInterfaces();
				for (NetworkInterface netint : Collections.list(nets)) {
					byte[] mac = netint.getHardwareAddress();
					if(mac != null) {
						StringBuilder sb = new StringBuilder();
						for (int i = 0; i < mac.length; i++) {
							sb.append(String.format("%02X%s", mac[i], (i < mac.length - 1) ? ":" : ""));		
						}
						if(wifiMacAddr.equals(sb.toString().toLowerCase())) {
							Enumeration<InetAddress> inetAddresses = netint.getInetAddresses();
							for (InetAddress inetAddress : Collections.list(inetAddresses)) {
								byte[] addrByte = inetAddress.getAddress();
								if(addrByte != null && addrByte.length == 16) {
									if(addrByte[11] == (byte) 0xff && addrByte[12] == (byte) 0xfe ) {
										if(!inetAddress.isLinkLocalAddress()) {
											Log.d(TAG, inetAddress.toString());
											interfaceAddress = inetAddress;
											inf = netint;
										}
									}
								}
							}
						}
					}
		        }
				
				if(interfaceAddress!=null) {
					bindAddress = interfaceAddress;
				}
			} catch (SocketException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
			server.start(inf);
			listener.register(server);
			Log.d(TAG, "HexabusServer started");
			started = true;
		} 
		// Continue until explicitly stopped
		return START_STICKY;
	}
	
	public void onDestroy() {
		if(started) {
			server.shutdown();
			multicastLock.release();
			Log.d(TAG, "HexabusServer stopped");
			started = false;
		}
		super.onDestroy();
	}
	
	public class HexabusServiceListener extends HexabusListener {
		@Override
		public void handlePacket(HexabusPacket packet) {
			Log.d(TAG, "Packet received: " + packet.getPacketType());
			Message received = Message.obtain(null, MSG_RECEIVE, packet);
			try {
				for(int i=0; i<clients.size();i++){
				clients.get(i).send(received);
				Log.d(TAG, "Message send");
				}
			} catch (RemoteException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	
	private class sendPacketTask extends AsyncTask<HexabusPacket,Void,Void> {

		@Override
		protected Void doInBackground(HexabusPacket... params) {
			try {		
				params[0].broadcastPacket();
			} catch (HexabusException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			return null;
		}
		
	}

}
