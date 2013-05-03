package de.fraunhofer.itwm.hexabus.android;

import de.fraunhofer.itwm.hexabus.android.HexaDroidDbAdapter.HexaDroidDbListener;
import android.app.Activity;
import android.os.Bundle;
import android.widget.ExpandableListView;
import android.widget.ListAdapter;
import android.widget.ListView;

public class DeviceActivity extends Activity implements HexaDroidDbListener{
    /** Called when the activity is first created. */
	private DeviceListAdapter listAdapter;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.devicelist);

		listAdapter = new DeviceListAdapter(this,this);

		ExpandableListView lv = (ExpandableListView) findViewById(R.id.devicelist);
		lv.setAdapter(listAdapter);
    }
	public void update() {
		listAdapter.update();
		runOnUiThread(new Runnable() {
			
			public void run() {
				listAdapter.updateData();
				listAdapter.notifyDataSetChanged();
			}
		});
		
	}


}
