package de.fraunhofer.itwm.hexabus.android;

import java.util.ArrayList;

import android.annotation.TargetApi;
import android.app.Fragment;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ExpandableListView;

@TargetApi(11)
public class DeviceListFragment extends Fragment {
	private DeviceListAdapter dlAdapter;
	private ArrayList<DeviceListListener> listener;
	
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
    	dlAdapter = new DeviceListAdapter();
    	Log.e("DeviceListFragment", "created");
        ExpandableListView lv = (ExpandableListView) inflater.inflate(R.layout.devicelist, container, false);
        lv.setAdapter(dlAdapter);
        //header = (TextView) v.findViewById(R.id.header1);
        //header.setText(R.string.event_header);
        return lv;
    }
    
    public interface DeviceListListener {
    	void onDeviceSelected();
    }
}
