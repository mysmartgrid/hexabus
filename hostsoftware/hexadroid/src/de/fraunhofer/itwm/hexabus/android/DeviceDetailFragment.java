package de.fraunhofer.itwm.hexabus.android;

import de.fraunhofer.itwm.hexabus.HexabusDevice;
import android.annotation.TargetApi;
import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

@TargetApi(11)
public class DeviceDetailFragment extends Fragment {
	private DeviceWidget widget;
	
	private void resetFragment() {
		//...
		widget = null;
	}
	
	public void displayDevice(HexabusDevice device) {
	}
	
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
    	return null;
    }

}
