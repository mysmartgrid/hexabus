package de.fraunhofer.itwm.hexabus.android;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.Hexabus.ErrorCode;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.HexabusErrorPacket;
import de.fraunhofer.itwm.hexabus.HexabusInfoPacket;
import de.fraunhofer.itwm.hexabus.HexabusPacket;
import de.fraunhofer.itwm.hexabus.HexabusWritePacket;
import de.fraunhofer.itwm.hexabus.android.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Inet6Address;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ExpandableListActivity;
import android.app.TabActivity;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.database.Cursor;
import android.widget.ExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.ExpandableListContextMenuInfo;
import android.widget.TabHost;
import android.widget.TabHost.TabSpec;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.Button;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.Toast;
import android.app.Dialog;
import android.text.Editable;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.Menu;
import android.view.MenuItem;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;

import de.fraunhofer.itwm.hexabus.android.HexaDroidDbAdapter;

//TODO switch to fragments, use android.support.v4.app...
public class HexaDroid extends TabActivity {
    private HexaDroidDbAdapter db;
	private static final String TAG = "HexaDroid";
	private Context context;
	private MonitorActivity monitorActivity;
	private InetAddress bindAddress;


    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		db = new HexaDroidDbAdapter(this);
		context = this;
		Intent service = new Intent(context, HexabusService.class);
		context.startService(service);
		
		setContentView(R.layout.main);
		
		TabHost tabHost = getTabHost();
		
        // Tab for remote control
        TabSpec remoteSpec = tabHost.newTabSpec("Remote");
        // setting title for the tab
        remoteSpec.setIndicator("Remote");
        Intent remoteIntent = new Intent(this, RemoteActivity.class);
        remoteSpec.setContent(remoteIntent);
        
        // Tab for monitor
        TabSpec monitorSpec = tabHost.newTabSpec("Monitor");
        // setting title for the tab
        monitorSpec.setIndicator("Monitor");
        Intent monitorIntent = new Intent(this, MonitorActivity.class);
        monitorSpec.setContent(monitorIntent);
        tabHost.addTab(remoteSpec);
        tabHost.addTab(monitorSpec);
        tabHost.setCurrentTab(1);
        tabHost.setCurrentTab(0);
        monitorActivity = (MonitorActivity) getLocalActivityManager().getActivity("Monitor");
        if(monitorActivity!=null) {
        	Log.d(TAG, monitorActivity.toString());
        } else {
        	Log.d(TAG, "derp");
        }
    }
    

	public InetAddress getInterfaceAddress() {
		InetAddress interfaceAddress = null;

		try {
			WifiManager wifiMan = (WifiManager) getSystemService(Context.WIFI_SERVICE);
			WifiInfo wifiInf = wifiMan.getConnectionInfo();
			String wifiMacAddr = wifiInf.getMacAddress();

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
									}
								}
							}
						}
					}
				}
	        }
			bindAddress = interfaceAddress;
		} catch (SocketException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
			return interfaceAddress;
	}
	
    /** Messenger for communicating with the service. */
    Messenger mService = null;

    /** Flag indicating whether we have called bind on the service. */
    boolean mBound = false;
    
   /**
     * Class for interacting with the main interface of the service.
     */
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            // This is called when the connection with the service has been
            // established, giving us the object we can use to
            // interact with the service.  We are communicating with the
            // service using a Messenger, so here we get a client-side
            // representation of that from the raw IBinder object.
            mService = new Messenger(service);
            mBound = true;
            Message msg = Message.obtain(null, HexabusService.MSG_REGISTER, 0, 0);
            msg.replyTo = mMessenger;
            try {
                mService.send(msg);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            // This is called when the connection with the service has been
            // unexpectedly disconnected -- that is, its process crashed.
            mService = null;
            mBound = false;
        }
    };
    
    
    @Override
    protected void onResume() {
        super.onResume();
        // Bind to the service
        if(!mBound) {
        	Log.d(TAG, "binding");
        getApplicationContext().bindService(new Intent(this, HexabusService.class), mConnection,
            Context.BIND_AUTO_CREATE);
        }

		HexabusInfoPacket livenessReport = null;
		try {
			livenessReport = new HexabusInfoPacket(31, true);
		} catch (HexabusException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		new sendPacketTask().execute(livenessReport);
    } 
    

	private class sendPacketTask extends AsyncTask<HexabusPacket,Void,Void> {

		@Override
		protected Void doInBackground(HexabusPacket... params) {
			try {
				getInterfaceAddress();
				Hexabus.setInterfaceAddress(bindAddress);
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
    
    @Override
    protected void onPause() {
        // Unbind from the service
        if (mBound) {
            Message msg = Message.obtain(null, HexabusService.MSG_UNREGISTER, 0, 0);
            msg.replyTo = mMessenger;
            try {
                mService.send(msg);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
            getApplicationContext().unbindService(mConnection);
            mService = null;
            mBound = false;
        }
        super.onPause();
    }

    
    final Messenger mMessenger = new Messenger(new IncomingHandler());

    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
        	Log.d(TAG, "Message received");
            switch (msg.what) {
            case HexabusService.MSG_RECEIVE:
            	Log.d(TAG, "PAKET!");
            	
            	class ListUpdateRunnable implements Runnable {
            		Message msg;
            		public void setMsg(Message msg) {
            			this.msg = msg;
            		}
            		public void run() {
            			monitorActivity.handlePacket((HexabusPacket) msg.obj);
            			
            		}
            	};
            	ListUpdateRunnable r = new ListUpdateRunnable();
            	r.setMsg(msg);
            	runOnUiThread(r);
            	break;
            default:
                super.handleMessage(msg);
            }
        }
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        menu.setHeaderTitle("Sample menu");
        menu.add(0, 0, 0, "Change name");
    }
    
    @Override
    public boolean onContextItemSelected(MenuItem item) {
        ExpandableListContextMenuInfo info = (ExpandableListContextMenuInfo) item.getMenuInfo();

        String title = ((TextView) info.targetView).getText().toString();
        
        int type = ExpandableListView.getPackedPositionType(info.packedPosition);
        if (type == ExpandableListView.PACKED_POSITION_TYPE_CHILD) {
            int groupPos = ExpandableListView.getPackedPositionGroup(info.packedPosition); 
            int childPos = ExpandableListView.getPackedPositionChild(info.packedPosition); 
            Toast.makeText(this, title + ": Child " + childPos + " clicked in group " + groupPos,
                    Toast.LENGTH_SHORT).show();
            return true;
        } else if (type == ExpandableListView.PACKED_POSITION_TYPE_GROUP) {
            int groupPos = ExpandableListView.getPackedPositionGroup(info.packedPosition); 
            Toast.makeText(this, title + ": Group " + groupPos + " clicked", Toast.LENGTH_SHORT).show();
            return true;
        }
        
        return false;
    }


	@Override
	protected Dialog onCreateDialog(int id) {
				final Dialog dialog;

		switch(id) {

			default:
				return null;
		}
	}
	
	public void fillData() {

	}
	

	public void shortToast(String message) {
		Context context = getApplicationContext();
		CharSequence text = message;
		int duration = Toast.LENGTH_SHORT;

		Toast toast = Toast.makeText(context, text, duration);
		toast.show();
	}
	
    private void addEndpoint(int eid, String name, String type, String description, boolean readable, boolean writable) {
		db.addEndpoint(eid, name, type, description, readable, writable);
		// TODO: Sanity checks

/*
		if(db.fetchEid(eid)) {
			throw new EndpointExistsException(); // TODO
		} else {
			db.addEid(eid, type, description, readable, writable); //TODO: check return value
		}
*/
    }

    private void removeEndpoint(int eid)
    {
    }

    private void updateEndpoint(int eid, String name, String type, String description, boolean readable, boolean writable)
    {
		db.updateEndpoint(eid, name, type, description, readable, writable);
    }

	private String[] fetchEndpoint(int eid) {
		Cursor cursor = db.fetchEndpoint(eid);
		String[] result = new String[] { cursor.getString(0), cursor.getString(1), cursor.getString(2),
			cursor.getString(3), cursor.getString(4) };
		return result;
	}

	private ArrayList<String[]> fetchAllEndpoints() {
		Cursor cursor = db.fetchAllEndpoints();
		if(cursor.moveToFirst()) { // TODO rewrite loop
		ArrayList<String[]> result = new ArrayList<String[]>();
		do {
			result.add(new String[] { cursor.getString(0), cursor.getString(1), cursor.getString(2),
				cursor.getString(3), cursor.getString(4), cursor.getString(5) }); // eid, name, type, description, readable, writable
		} while(cursor.moveToNext());
		return result;
		}else{
		return null;
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		//menu.add(0, ADD_ID, 0, R.string.menu_add_eid);
		return true;
}

	@Override
	public boolean onMenuItemSelected(int featureId, MenuItem item) {

		return super.onMenuItemSelected(featureId, item);
	}

	private class sendWritePacketTask extends AsyncTask<Boolean, Void, Void> {
		protected Void doInBackground(Boolean... values) {
			return null;
		}
	}
	

}
