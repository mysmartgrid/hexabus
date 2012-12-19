package de.fraunhofer.itwm.hexabus.android;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.HexabusDatetime;
import de.fraunhofer.itwm.hexabus.HexabusEndpointInfoPacket;
import de.fraunhofer.itwm.hexabus.HexabusEndpointQueryPacket;
import de.fraunhofer.itwm.hexabus.HexabusErrorPacket;
import de.fraunhofer.itwm.hexabus.HexabusInfoPacket;
import de.fraunhofer.itwm.hexabus.HexabusPacket;
import de.fraunhofer.itwm.hexabus.Hexabus.ErrorCode;
import de.fraunhofer.itwm.hexabus.HexabusQueryPacket;
import de.fraunhofer.itwm.hexabus.HexabusWritePacket;

import android.R.color;
import android.content.Context;
import android.graphics.Color;
import android.text.Spannable;
import android.text.style.RelativeSizeSpan;
import android.text.style.StyleSpan;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.BaseExpandableListAdapter;
import android.widget.LinearLayout;
import android.widget.TextView;

public class MonitorListAdapter extends ArrayAdapter {
	
	  private  Context context;
	  private final ArrayList<HexabusPacket> values;

	public MonitorListAdapter(Context context, ArrayList<HexabusPacket> objects) {
		super(context, R.layout.monitorlistrows, objects);
		
		this.context = context;
		this.values = objects;
	}

	public View getView(int position, View convertView, ViewGroup parent) {

		
		context = context.getApplicationContext();
		LayoutInflater inflater = (LayoutInflater) context
		        .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		LinearLayout rowView = (LinearLayout) inflater.inflate(R.layout.monitorlistrows, parent, false);
		LinearLayout layout = new LinearLayout(context);
		TextView textView = (TextView) rowView.findViewById(R.id.monitorListRow);
		if(position%2==1) {
			//textView.setGravity(Gravity.RIGHT);
			textView.setBackgroundColor(Color.DKGRAY);
		}
		    
		HexabusPacket packet = values.get(position);
		String text = "";
		Hexabus.PacketType type = packet.getPacketType();
		int boldText = 0;
		Spannable str;
		switch(packet.getPacketType()) {
			case ERROR:
				text += "ERROR\n";
				boldText = text.length();
				text += "\tSource: " + packet.getSourceAddress() + "\n";
				text += "\tError Code: " + ((HexabusErrorPacket)packet).getErrorCode();
				textView.setText(text, TextView.BufferType.SPANNABLE);
				str = (Spannable) textView.getText();
				str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, boldText, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				str.setSpan(new RelativeSizeSpan(0.8f), boldText, str.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				break;
			case INFO:
				HexabusInfoPacket infoPacket = (HexabusInfoPacket) packet;
				text += "INFO\n";
				boldText = text.length();
				text += "\tSource: " + packet.getSourceAddress() + "\n";
				text += "\tEID: " + Integer.toString(infoPacket.getEid()) + "\n";
				text += "\tData Type: " + infoPacket.getDataType() + "\n";
				text += "\tValue: ";
				switch(infoPacket.getDataType()) {
				case BOOL:
					try {
						text += (infoPacket.getBool()?"true":"false");
					} catch (HexabusException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					break;
				case DATETIME:
					try {
						HexabusDatetime datetime = infoPacket.getDatetime();
						text += datetime.toString();
					} catch (HexabusException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					break;
				case FLOAT:
					try {
						text += infoPacket.getFloat();
					} catch (HexabusException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					break;
				case STRING:
					try {
						text += infoPacket.getString();
					} catch (HexabusException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					} 
					break;
				case TIMESTAMP:
					try {
						text += infoPacket.getTimestamp();
					} catch (HexabusException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					break;
				case UINT32:
					try {
						text += infoPacket.getUint32();
					} catch (HexabusException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					break;
				case UINT8:
					try {
						text += infoPacket.getUint8();
					} catch (HexabusException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					break;
				default:
				}
						
				textView.setText(text, TextView.BufferType.SPANNABLE);
				str = (Spannable) textView.getText();
				str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, boldText, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				str.setSpan(new RelativeSizeSpan(0.8f), boldText, str.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				break;
			case QUERY:
				HexabusQueryPacket queryPacket = (HexabusQueryPacket) packet;
				text += "QUERY\n";
				boldText = text.length();
				text += "\tSource: " + packet.getSourceAddress() + "\n";
				//TODO
				text += "\tEID: " ;//+ Integer.toString(queryPacket.getEid()) + "\n";

				textView.setText(text, TextView.BufferType.SPANNABLE);
				str = (Spannable) textView.getText();
				str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, boldText, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				str.setSpan(new RelativeSizeSpan(0.8f), boldText, str.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				break;
			case WRITE:
				HexabusWritePacket writePacket = (HexabusWritePacket) packet;
				text += "WRITE\n";
				boldText = text.length();
				text += "\tSource: " + packet.getSourceAddress() + "\n";
				text += "\tEID: " + Integer.toString(writePacket.getEid()) + "\n";
				text += "\tData Type: " + writePacket.getDataType() + "\n";
				text += "\tValue: ";
				
				textView.setText(text, TextView.BufferType.SPANNABLE);
				str = (Spannable) textView.getText();
				str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, boldText, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				str.setSpan(new RelativeSizeSpan(0.8f), boldText, str.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				break;
			case EPINFO:
				HexabusEndpointInfoPacket endpointInfoPacket = (HexabusEndpointInfoPacket) packet;
				text += "EPINFO\n";
				boldText = text.length();
				text += "\tSource: " + packet.getSourceAddress() + "\n";
				text += "\tEID: " + Integer.toString(endpointInfoPacket.getEid()) + "\n";
				text += "\tData Type: " + endpointInfoPacket.getDataType() + "\n";
				text += "\tValue: ";
				textView.setText(text, TextView.BufferType.SPANNABLE);
				str = (Spannable) textView.getText();
				str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, boldText, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				str.setSpan(new RelativeSizeSpan(0.8f), boldText, str.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				break;
			case EPQUERY:
				HexabusEndpointQueryPacket endpointQueryPacket = (HexabusEndpointQueryPacket) packet;
				text += "EPQUERY\n";
				boldText = text.length();
				text += "\tSource: " + packet.getSourceAddress() + "\n";
				//TODO
				text += "\tEID: " ;//+ Integer.toString(queryPacket.getEid()) + "\n";
				textView.setText(text, TextView.BufferType.SPANNABLE);
				str = (Spannable) textView.getText();
				str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, boldText, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				str.setSpan(new RelativeSizeSpan(0.8f), boldText, str.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				break;
			default:
				text += "UNKNOWN";

				boldText = text.length();
				text += "\tSource: " + packet.getSourceAddress() + "\n";
				textView.setText(text, TextView.BufferType.SPANNABLE);
				str = (Spannable) textView.getText();
				str.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, boldText, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				str.setSpan(new RelativeSizeSpan(0.8f), boldText, str.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
		}

		return rowView;
		
	}

}
