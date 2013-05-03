package de.fraunhofer.itwm.hexabus.android;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.text.DecimalFormat;
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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.BaseExpandableListAdapter;

public class DeviceListAdapter extends BaseExpandableListAdapter {

	private ArrayList<HexabusDevice> devices;
	private ArrayList<ArrayList<HexabusEndpoint>> endpoints;
	private ArrayList<HexabusDevice> newDevices;
	private ArrayList<ArrayList<HexabusEndpoint>> newEndpoints;
	private HexaDroidDbAdapter db;
	private final String TAG = "DeviceListAdapter";
	private Context context;
	
	public DeviceListAdapter(Context context, HexaDroidDbListener dbListener) {
		this.context = context;
		db = new HexaDroidDbAdapter(context);
		db.register(dbListener);
		init();
		devices = newDevices;
		endpoints = newEndpoints;
	}
	
	private void init() {
		newEndpoints = new ArrayList<ArrayList<HexabusEndpoint>>();
		newDevices = db.getDevices();
		for(HexabusDevice device : newDevices){
			ArrayList<HexabusEndpoint> devEndpoints = new ArrayList(device.getEndpoints().values());
			newEndpoints.add(devEndpoints);
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
		LinearLayout layout = new LinearLayout(parent.getContext());
		layout.setGravity(Gravity.CENTER_VERTICAL);
		View stateView;
		TextView textView = getGenericView(parent);

		if(childPosition%2==1) {
			layout.setBackgroundColor(Color.DKGRAY);
		}
		// TODO class test
		HexabusEndpoint endpoint = (HexabusEndpoint) getChild(groupPosition, childPosition);
		HexabusDevice device = (HexabusDevice) getGroup(groupPosition);
		String state =db.getState(device,endpoint);
		switch (endpoint.getDataType()) {
		case BOOL:
			ImageView imageView = new ImageView(parent.getContext());
			stateView = new RelativeLayout(parent.getContext());
			((RelativeLayout) stateView).setGravity(Gravity.CENTER_VERTICAL | Gravity.RIGHT);
			if(state.equals("true")) {
				((ImageView) imageView).setImageResource(R.drawable.bool_on);
				((ImageView) imageView).setPadding(0, 0, 25, 0);
				((ImageView) imageView).setMinimumHeight(100);
				((RelativeLayout) stateView).addView(imageView);
			} else if(state.equals("false")) {
				((ImageView) imageView).setImageResource(R.drawable.bool_off);
				((ImageView) imageView).setPadding(0, 0, 25, 0);
				((ImageView) imageView).setMinimumHeight(100);
				((RelativeLayout) stateView).addView(imageView);
			} else {
				stateView = new TextView(parent.getContext());
				((TextView) stateView).setText("");
				((TextView) stateView).setGravity(Gravity.CENTER_VERTICAL | Gravity.RIGHT);
				((TextView) stateView).setPadding(0, 0, 25, 0);
				((TextView) stateView).setMinHeight(100);
			}
			break;
		case FLOAT:
			try {
				DecimalFormat df = new DecimalFormat();
				df.setMaximumFractionDigits(2);
				state = df.format(new Double(state));
			} catch(NumberFormatException e) {
				state = "";
			}
			stateView = new TextView(parent.getContext());
			((TextView) stateView).setText(state);
			((TextView) stateView).setGravity(Gravity.CENTER_VERTICAL | Gravity.RIGHT);
			((TextView) stateView).setPadding(0, 0, 25, 0);
			((TextView) stateView).setMinHeight(100);
			break;
		default:
			stateView = new TextView(parent.getContext());
			((TextView) stateView).setText(state);
			((TextView) stateView).setGravity(Gravity.CENTER_VERTICAL | Gravity.RIGHT);
			((TextView) stateView).setPadding(0, 0, 25, 0);
			((TextView) stateView).setMinHeight(100);
			break;
		}
		layout.addView(textView);
		layout.addView(stateView);
		LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) stateView.getLayoutParams();
		//params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		//params.addRule(RelativeLayout.RIGHT_OF,)
		params.weight = 0.3f;
		stateView.setLayoutParams(params);
		params = (LinearLayout.LayoutParams) textView.getLayoutParams();
		params.weight = 0.7f;
		textView.setLayoutParams(params);
		textView.setText(Long.toString(endpoint.getEid())+" "+endpoint.getDescription());
		return layout;
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
		Log.d(TAG, "update received");
		// TODO Auto-generated method stub
		
	}
	
	public void updateData() {
		endpoints = newEndpoints;
		devices = newDevices;
	}
	
  

}
