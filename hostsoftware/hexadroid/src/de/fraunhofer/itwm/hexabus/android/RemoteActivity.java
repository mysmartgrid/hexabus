package de.fraunhofer.itwm.hexabus.android;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.HexabusInfoPacket;
import android.app.Activity;
import android.app.AlertDialog;
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
					//irInfo.broadcastPacket();
					irInfo.sendPacket(InetAddress.getByName("fe80::224:d7ff:fee7:99c8"));
					//irInfo.sendPacket(InetAddress.getByName("10.23.1.192"));
					Log.d(TAG, "Send IR info packet");
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
		
		new sendIRInfoPacketTask().execute(1);
	}

	private class ButtonLongClickListener implements OnLongClickListener {

		public boolean onLongClick(View v) {
	        final Button button = (Button) v;//findViewById(R.id.button1);
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
