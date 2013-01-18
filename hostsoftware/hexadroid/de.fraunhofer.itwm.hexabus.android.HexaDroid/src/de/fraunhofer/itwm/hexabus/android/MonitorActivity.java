package de.fraunhofer.itwm.hexabus.android;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.GregorianCalendar;

import de.fraunhofer.itwm.hexabus.Hexabus.DataType;
import de.fraunhofer.itwm.hexabus.Hexabus.ErrorCode;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusListener;
import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.HexabusDatetime;
import de.fraunhofer.itwm.hexabus.HexabusEndpointInfoPacket;
import de.fraunhofer.itwm.hexabus.HexabusEndpointQueryPacket;
import de.fraunhofer.itwm.hexabus.HexabusErrorPacket;
import de.fraunhofer.itwm.hexabus.HexabusInfoPacket;
import de.fraunhofer.itwm.hexabus.HexabusPacket;
import de.fraunhofer.itwm.hexabus.HexabusQueryPacket;
import de.fraunhofer.itwm.hexabus.HexabusTimestamp;
import de.fraunhofer.itwm.hexabus.HexabusWritePacket;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.RemoteException;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ExpandableListView;
import android.widget.ListView;
import android.widget.Toast;

public class MonitorActivity extends Activity {
	
	private static final String TAG = "Monitor";
	private Context mContext;
	private MonitorListAdapter listAdapter;
	
    /** Messenger for communicating with the service. */
    Messenger mService = null;

    /** Flag indicating whether we have called bind on the service. */
    boolean mBound;
    
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
        	Toast.makeText(getApplicationContext(), "HexabusService started", Toast.LENGTH_SHORT).show();
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
    protected void onStart() {
        super.onStart();
        // Bind to the service
        getApplicationContext().bindService(new Intent(this, HexabusService.class), mConnection,
            Context.BIND_AUTO_CREATE);
    }  
    
    @Override
    protected void onStop() {
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
        super.onStop();
    }
	
	

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.monitor);
        
        ArrayList<HexabusPacket> values = new ArrayList<HexabusPacket>();
		listAdapter = new MonitorListAdapter(this, values);

		ListView lv = (ListView) findViewById(R.id.monitorListView);
		lv.setAdapter(listAdapter);
		/*
		try {
			HexabusPacket packet = new HexabusErrorPacket(ErrorCode.CRCFAILED);

			values.add(packet);
			packet = new HexabusInfoPacket(1, true);
			values.add(packet);
			Calendar calendar = new GregorianCalendar();
			packet = new HexabusInfoPacket(1, new HexabusDatetime(calendar));
			values.add(packet);
			packet = new HexabusInfoPacket(1, (float) 0.4);
			values.add(packet);
			packet = new HexabusInfoPacket(1, "Horst");
			values.add(packet);
			packet = new HexabusInfoPacket(1, new HexabusTimestamp(1231231232));
			
			values.add(packet);
			packet = new HexabusInfoPacket(1, 100L);
			values.add(packet);
			packet = new HexabusInfoPacket(1, (short) 12);
			//packet.setSourceAddress(InetAddress.getByAddress(new byte[] {127,0,0,1}));
			values.add(packet);
			packet = new HexabusEndpointQueryPacket(1);
			//packet.setSourceAddress(InetAddress.getByAddress(new byte[] {127,0,0,1}));
			values.add(packet);
			packet = new HexabusWritePacket(1, true);
			//packet.setSourceAddress(InetAddress.getByAddress(new byte[] {127,0,0,1}));
			values.add(packet);
			packet = new HexabusQueryPacket(1);
			//packet.setSourceAddress(InetAddress.getByAddress(new byte[] {127,0,0,1}));
			values.add(packet);
		} catch (HexabusException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		*/

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
            			listAdapter.add((HexabusPacket) msg.obj);
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
    

}
/*

private ArrayList<DeviceListListener> listener;



public interface DeviceListListener {
	void onDeviceSelected();
}*/