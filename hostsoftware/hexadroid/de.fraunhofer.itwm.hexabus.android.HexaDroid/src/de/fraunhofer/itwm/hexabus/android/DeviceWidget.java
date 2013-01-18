package de.fraunhofer.itwm.hexabus.android;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.HexabusDevice;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

public abstract class DeviceWidget extends ViewGroup {
	
	private Hexabus.DataType dataType;
	private HexabusDevice device;

	public DeviceWidget(Context context, HexabusDevice device) {
		super(context);
		this.device = device;
		// TODO Auto-generated constructor stub
	}

}
