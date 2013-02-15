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
    
    public void handlePacket(HexabusPacket packet) {
    	listAdapter.add(packet);
    }
  
    

}
/*

private ArrayList<DeviceListListener> listener;



public interface DeviceListListener {
	void onDeviceSelected();
}*/