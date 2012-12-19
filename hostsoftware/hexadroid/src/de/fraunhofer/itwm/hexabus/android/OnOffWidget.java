package de.fraunhofer.itwm.hexabus.android;

import de.fraunhofer.itwm.hexabus.HexabusDevice;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;

public class OnOffWidget extends DeviceWidget {

	private final String TAG = "OnOffWidget";
	public OnOffWidget(Context context, HexabusDevice device) {
		super(context, device);
		LayoutInflater inflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        this.addView(inflater.inflate(R.layout.onoffwidget, null));
		// TODO Auto-generated constructor stub
	}
	
	protected void onButton() {
		
	}
	
	protected void offButton() {
		
	}
	
	public String getWidgetName() {
		return TAG;
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		// TODO Auto-generated method stub
        for(int i = 0 ; i < getChildCount() ; i++){
            getChildAt(i).layout(l, t, r, b);
        }
	}


}
