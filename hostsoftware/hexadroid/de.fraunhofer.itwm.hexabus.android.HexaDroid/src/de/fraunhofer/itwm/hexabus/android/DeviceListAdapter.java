package de.fraunhofer.itwm.hexabus.android;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.HexabusDevice;
import de.fraunhofer.itwm.hexabus.HexabusEndpoint;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.BaseExpandableListAdapter;

public class DeviceListAdapter extends BaseExpandableListAdapter {

	private ArrayList<HexabusDevice> devices;
	private ArrayList<ArrayList<HexabusEndpoint>> endpoints;
	
	public DeviceListAdapter() {
		devices = new ArrayList<HexabusDevice>();
		endpoints = new ArrayList<ArrayList<HexabusEndpoint>>();
		try {
			HexabusDevice testD = new HexabusDevice(InetAddress.getByName("fc01::224:7eff:fede:6918"));
			HexabusEndpoint testE = testD.addEndpoint(1, Hexabus.DataType.BOOL);
			devices.add(testD);
			ArrayList<HexabusEndpoint> tmp = new ArrayList<HexabusEndpoint>();
			tmp.add(testE);
			endpoints.add(tmp);
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
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
        //textView.setLayoutParams(lp);
        // Center the text vertically
        textView.setGravity(Gravity.CENTER_VERTICAL | Gravity.LEFT);
        // Set the text starting position
        textView.setPadding(36, 0, 0, 0);
        return textView;
    }

	public View getChildView(int groupPosition, int childPosition,
			boolean isLastChild, View convertView, ViewGroup parent) {
		TextView textView = getGenericView(parent);

		// TODO class test
		textView.setText(Long.toString(((HexabusEndpoint) getChild(groupPosition, childPosition)).getEid()));
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
		textView.setText(((HexabusDevice) getGroup(groupPosition)).getInetAddress().toString());
		return textView;
	}

	public boolean hasStableIds() {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean isChildSelectable(int groupPosition, int childPosition) {
		return true;
	}
	
  

}
