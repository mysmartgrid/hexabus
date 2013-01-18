package de.fraunhofer.itwm.hexabus.android;

import java.io.IOException;
import java.net.InetAddress;
import java.net.MulticastSocket;
import java.net.NetworkInterface;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.HexabusDevice;
import de.fraunhofer.itwm.hexabus.HexabusEndpoint;
import de.fraunhofer.itwm.hexabus.HexabusInfoPacket;
import de.fraunhofer.itwm.hexabus.HexabusPacket;
import de.fraunhofer.itwm.hexabus.HexabusQueryPacket;
import de.fraunhofer.itwm.hexabus.HexabusWritePacket;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.DownloadManager.Query;
import android.content.Context;
import android.content.DialogInterface;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnLongClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ExpandableListAdapter;
import android.widget.Toast;

public class RemoteActivity extends Activity {

	private static final String TAG = "RemoteActivity";
	private ExpandableListAdapter mAdapter;
	private Context mContext;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

		mContext = this;
		//mAdapter = new DeviceListAdapter();
		setContentView(R.layout.remote);
		for(int i=1;i<=16;i++) {
			int id = getResources().getIdentifier("Button"+Integer.toString(i), "id", mContext.getPackageName());
			Button button = (Button) findViewById(id);
			ButtonLongClickListener buttonLongClick = new ButtonLongClickListener();
			button.setOnLongClickListener(buttonLongClick);
		}

		//setListAdapter(mAdapter);
		//registerForContextMenu(getExpandableListView());
  }

	public void shortToast(String message) {
		Context context = getApplicationContext();
		CharSequence text = message;
		int duration = Toast.LENGTH_SHORT;

		Toast toast = Toast.makeText(context, text, duration);
		toast.show();
	}

	private class sendIRInfoPacketTask extends AsyncTask<Integer,Void,Void> {
		protected Void doInBackground(Integer... values) {
			HexabusInfoPacket irInfo = null;
			try {
				irInfo = new HexabusInfoPacket(30, values[0].longValue());
				

			} catch (HexabusException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
			if(irInfo!=null) {
				try {					

					MulticastSocket s = new MulticastSocket();
					Log.d(TAG, "Sending IR info packet");
					irInfo.broadcastPacket();
					
					
					//s.joinGroup(InetAddress.getByAddress(Hexabus.MULTICAST_GROUP));
					//Hexabus.setInterfaceAddress(Collections.list(NetworkInterface.getNetworkInterfaces()).get(0).getInterfaceAddresses().get(0));
					/*InetAddress address = InetAddress.getByName("fd01:2:0:0:50:c4ff:fe04:8383");
					HexabusDevice device = new HexabusDevice(address);
					HexabusEndpoint relais = device.addEndpoint(1, Hexabus.DataType.BOOL);
					HexabusPacket packet = new HexabusQueryPacket(relais.getEid());
					int port = packet.sendPacket(device.getInetAddress());
					// Receive reply
					packet = Hexabus.receivePacket(port, 200);
					if(packet!=null) {
						boolean state = ((HexabusInfoPacket) packet).getBool();
						HexabusWritePacket iPac = new HexabusWritePacket(1, !state);
						iPac.broadcastPacket();
						Log.d(TAG, "Send IR info packet");
					}*/
				} catch (UnknownHostException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (HexabusException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			return null;
		}
	}
	
	// ButtonClickListener
	public void onButtonClick(View v) {
		
		new sendIRInfoPacketTask().execute(Integer.valueOf(((String) ((Button) v).getText())));
	}

	private class ButtonLongClickListener implements OnLongClickListener {

		public boolean onLongClick(View v) {
	        final Button button = (Button) v;
	        final EditText input = new EditText(mContext);
	        input.setText(button.getText());
	        new AlertDialog.Builder(mContext)
	        .setTitle("Change button text")
	        .setView(input)
	        .setPositiveButton("Ok", new DialogInterface.OnClickListener() {
	            public void onClick(DialogInterface dialog, int whichButton) {
	                String value = input.getText().toString();
	                button.setText(value);
	            }
	        }).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
	            public void onClick(DialogInterface dialog, int whichButton) {
	                // Do nothing.
	            }
	        }).show();
			return true;
		}
		
	}
}
