package de.fraunhofer.itwm.hexabus.android;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.HexabusDevice;
import de.fraunhofer.itwm.hexabus.HexabusEndpoint;
import de.fraunhofer.itwm.hexabus.android.HexaDroidDbAdapter.HexaDroidDbListener;
import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.BaseExpandableListAdapter;

public class DeviceListAdapter extends BaseExpandableListAdapter {

	private ArrayList<HexabusDevice> devices;
	private ArrayList<ArrayList<HexabusEndpoint>> endpoints;
	private HexaDroidDbAdapter db;
	private final String TAG = "DeviceListAdapter";
	
	public DeviceListAdapter(Context context, HexaDroidDbListener dbListener) {
		db = new HexaDroidDbAdapter(context);
		db.register(dbListener);
		init();		
	}
	
	private void init() {
		//devices = new ArrayList<HexabusDevice>();
		endpoints = new ArrayList<ArrayList<HexabusEndpoint>>();
		devices = db.getDevices();
		class EndpointComparator implements Comparator<HexabusEndpoint>{

			public int compare(HexabusEndpoint lhs, HexabusEndpoint rhs) {
				// TODO Auto-generated method stub
				long lhseid = lhs.getEid();
				long rhseid = rhs.getEid();
				return (lhseid < rhseid ? -1 :
		               (lhseid == rhseid ? 0 : 1));
			}
			
		};
		for(HexabusDevice device : devices){
			ArrayList<HexabusEndpoint> devEndpoints = new ArrayList(device.getEndpoints().values());
			Collections.sort(devEndpoints, new EndpointComparator());
			
			endpoints.add(devEndpoints);
		}
		
	}

	public Object getChild(int groupPosition, int childPosition) {
		return endpoints.get(groupPosition).get(childPosition);
	}

	public long getChildId(int groupPosition, int childPosition) {
		return childPosition;
	}
	
    public TextView getGenericView(ViewGroup parent) {
        // Layout parameters for the ExpandableListView
       /* AbsListView.LayoutParams lp = new AbsListView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 64);*/

        TextView textView = new TextView(parent.getContext());
        textView.setMinHeight(100);
        //textView.setLayoutParams(lp);
        // Center the text vertically
        textView.setGravity(Gravity.CENTER_VERTICAL | Gravity.LEFT);
        // Set the text starting position
        textView.setPadding(75, 0, 0, 0);
        return textView;
    }

	public View getChildView(int groupPosition, int childPosition,
			boolean isLastChild, View convertView, ViewGroup parent) {
		TextView textView = getGenericView(parent);

		if(childPosition%2==1) {
			textView.setBackgroundColor(Color.DKGRAY);
		}
		// TODO class test
		HexabusEndpoint endpoint = (HexabusEndpoint) getChild(groupPosition, childPosition);
		textView.setText(Long.toString(endpoint.getEid())+" "+endpoint.getDescription());
		return textView;
	}

	public int getChildrenCount(int groupPosition) {
		return endpoints.get(groupPosition).size();
	}

	public Object getGroup(int groupPosition) {
		return devices.get(groupPosition);
	}

	public int getGroupCount() {
		return devices.size();
	}

	public long getGroupId(int groupPosition) {
		return groupPosition;
	}

	public View getGroupView(int groupPosition, boolean isExpanded,
			View convertView, ViewGroup parent) {
		TextView textView = getGenericView(parent);
		textView.setText(((HexabusDevice) getGroup(groupPosition)).getName());
		return textView;
	}

	public boolean hasStableIds() {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean isChildSelectable(int groupPosition, int childPosition) {
		return true;
	}

	public void update() {
		init();
		notifyDataSetChanged();
		Log.d(TAG, "update received");
		// TODO Auto-generated method stub
		
	}
	
  

}
