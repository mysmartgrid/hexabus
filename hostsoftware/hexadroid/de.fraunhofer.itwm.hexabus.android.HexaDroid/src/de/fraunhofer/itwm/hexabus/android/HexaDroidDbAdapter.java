package de.fraunhofer.itwm.hexabus.android;

import java.net.InetAddress;
import java.util.ArrayList;

import de.fraunhofer.itwm.hexabus.Hexabus;
import de.fraunhofer.itwm.hexabus.Hexabus.HexabusException;
import de.fraunhofer.itwm.hexabus.HexabusDevice;
import de.fraunhofer.itwm.hexabus.HexabusEndpoint;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.text.method.MovementMethod;
import android.util.Log;

public class HexaDroidDbAdapter {
	public static final String KEY_EID = "eid";
	public static final String KEY_DID = "did";
	public static final String KEY_TYPE = "type";
	public static final String KEY_DESC = "desc";
	public static final String KEY_READ = "read";
	public static final String KEY_WRITE = "write";
	private static final String KEY_NAME = "name";

	private static final String KEY_IP = "ip";
	private static final String TAG = "HexaDroidDbAdapter";
	private DatabaseHelper dbHelper;
	private SQLiteDatabase db;
	
	private static ArrayList<HexaDroidDbListener> listeners = new ArrayList<HexaDroidDbListener>();

	private static final String DEVICE_CREATE =
		"create table devices ( "
		+ "did integer primary key, name text not null, "
		+ "ip text not null, desc text not null);";
	private static final String HAS_EID_CREATE =
			"create table has_eid ( "
			+ "did integer not null, eid integer not null, "
			+ "primary key (did, eid));";
	private static final String EID_CREATE =
			"create table endpoints ( "
			+ "eid integer primary key, name text,"
			+ "type text not null, desc text not null, "
			+ "read boolean not null, write boolean not null);";
	private static final String DB_INITIAL_INSERT =
		"insert into endpoints ("+KEY_EID+","+KEY_TYPE+","
		+KEY_DESC+","+KEY_READ+","+KEY_WRITE+") values ";
	private static final String[] INITIAL_EIDS= new String[] {
		"(0, 'uint32', 'Hexabus device descriptor', 1, 0)",
		"(1, 'bool', 'HexabusPlug relay', 1, 1)",
		"(2, 'uint32', 'HexabusPlug+ power meter', 1, 0)",
		"(3, 'float', 'Temperature sensor (°C)', 1, 0)",
		"(4, 'bool', 'internal Button (on board / plug)', 1, 0)",
		"(5, 'float', 'Humidity sensor (%r.h.)', 1, 0)",
		"(6, 'float', 'Barometric pressure sensor (hPa)', 1, 0)",
		"(7, 'float', 'HexabusPlug+ energy meter total (kWh)', 1, 0)",
		"(8, 'float', 'HexabusPlug+ energy meter user resettable', 1, 1)",
		"(9, 'uint8', 'Statemachine control', 1, 1)",
		"(10, 'bytes', 'Statemachine upload receiver', 0, 1)",
		"(11, 'bool', 'Statemachine upload ack/nack', 1, 0)",
		"(22, 'float', 'Analogread', 1, 0)",
		"(23, 'uint8', 'Window shutter', 1, 1)",
		"(24, 'uint8', 'Hexapush pressed buttons', 1, 0)",
		"(25, 'uint8', 'Hexapush clicked buttons', 1, 0)",
		"(26, 'bool', 'Presence detector', 1, 1)",
		"(27, 'uint8', 'Hexonoff set', 1, 1)",
		"(28, 'uint8', 'Hexonoff toggle', 1, 1)",
		"(29, 'float', 'Lightsensor', 1, 0)",
		"(30, 'uint32', 'IR Receiver', 1, 0)",
		"(31, 'bool', 'Node liveness', 1, 0)"
	};
	private static final String DB_NAME = "data";
	private static final String DB_DEVICE_TABLE = "devices";
	private static final String DB_EID_TABLE = "endpoints";
	private static final String DB_HAS_EID_TABLE = "has_eid";
	private static final int DB_VERSION = 1;
	private final Context context;

	private static class DatabaseHelper extends SQLiteOpenHelper {
		
        DatabaseHelper(Context context) {
            super(context, DB_NAME, null, DB_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {

            db.execSQL(DEVICE_CREATE);
            db.execSQL(EID_CREATE);
            db.execSQL(HAS_EID_CREATE);

            
			for(String eid : INITIAL_EIDS ) {
	            db.execSQL(DB_INITIAL_INSERT+eid+";");
			}
			
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            Log.w(TAG, "Upgrading database from version " + oldVersion + " to "
                    + newVersion + ", which will destroy all old data");
            db.execSQL("drop table if exists " + DB_EID_TABLE);
            onCreate(db);
        }
	}

	public HexaDroidDbAdapter(Context ctx) {
		this.context = ctx;
		dbHelper = new DatabaseHelper(context);
	}

	private HexaDroidDbAdapter open() throws SQLException {
		db = dbHelper.getWritableDatabase();
		return this;
	}

	private void close() {
		dbHelper.close();
	}
	
	public long addDevice(String name, String ip, String desc) {
		long result;
		synchronized (dbHelper) {
			open();
			ContentValues values = new ContentValues();
			values.put(KEY_NAME, name);
			values.put(KEY_IP, ip);
			values.put(KEY_DESC, desc);
			result = db.insert(DB_DEVICE_TABLE, null, values);
			close();
		}
		
		for(HexaDroidDbListener listener : listeners) {
			listener.update();
			Log.d(TAG, "Listener notified");
		}
		
		return result;
	}
	
	public ArrayList<HexabusDevice> getDevices() {
		ArrayList<HexabusDevice> result = new ArrayList<HexabusDevice>();
		Cursor cursor;
		synchronized(dbHelper) {
			open();
			cursor = db.query(DB_DEVICE_TABLE, new String[] {KEY_DID, KEY_NAME, KEY_IP},
					null,  null, null, null, null);
			if(cursor != null) {
				while(cursor.moveToNext()) {
					HexabusDevice device = new HexabusDevice(cursor.getString(2), cursor.getString(1));
					ArrayList<Long> eids = getDeviceEids(cursor.getInt(0));
					for(Long eid : eids) {
						Cursor eidCursor = fetchEndpoint(eid);
						if(eidCursor!=null) {
							try {
								if(eid!=0){
									device.addEndpoint(eid, Hexabus.getDataTypeByString(eidCursor.getString(2)), eidCursor.getString(3));
								}
							} catch (HexabusException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
						}
					}
					result.add(device);
				}
			}
			
			close();
		}
		return result;
	}
	
	public HexabusDevice getDeviceByIp(InetAddress address) {
		HexabusDevice result = null;
		String ip = address.getHostAddress();
		Cursor cursor;
		synchronized (dbHelper) {
			open();
			cursor = db.query(true, DB_DEVICE_TABLE, new String[] {KEY_DID, KEY_IP},
					KEY_IP + "= ?" , new String[] {ip}, null, null, null, null);
			if(cursor != null && cursor.moveToFirst()) {
				//cursor.moveToFirst();
				result = new HexabusDevice(address);
				ArrayList<Long> eids = getDeviceEids(cursor.getInt(0));
				for(Long eid : eids) {
					Cursor eidCursor = fetchEndpoint(eid);
					if(eidCursor!=null) {
						try {
							if(eid!=0){
								result.addEndpoint(eid, Hexabus.getDataTypeByString(eidCursor.getString(2)), eidCursor.getString(3));
							}
						} catch (HexabusException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}
				}
			}
			close();

		}
		return result;
	}
	
	public ArrayList<Long> getDeviceEids(int deviceId) {
		Cursor cursor;
		ArrayList<Long> result= new ArrayList<Long>();
		synchronized(dbHelper) {
			open();
			cursor = db.query(true,DB_HAS_EID_TABLE, new String[] {KEY_EID}, KEY_DID + "=" + deviceId, null, null, null, null, null);
			if(cursor != null) {
				while(cursor.moveToNext()) {
					result.add(cursor.getLong(0));
				}
			}
			close();
		}
		return result;
	}
	
	public long addEndpointToDevice(HexabusDevice device, long eid) {
		long result = -1;
		String ip = device.getInetAddress().getHostAddress();
		Cursor cursor;
		synchronized (dbHelper) {
			open();
			cursor = db.query(true, DB_DEVICE_TABLE, new String[] {KEY_DID},
					KEY_IP + "= ?" , new String[] {ip}, null, null, null, null);
			if(cursor!=null && cursor.moveToFirst()) {
				ContentValues values = new ContentValues();
				values.put(KEY_DID, cursor.getInt(0));
				values.put(KEY_EID, eid);
				result = db.insert(DB_HAS_EID_TABLE, null, values);
			}
			close();			
		}
		
		for(HexaDroidDbListener listener : listeners) {
			listener.update();
		}
		return result;
	}

	public long addEndpoint(long eid, String name, String type, String description, boolean readable, boolean writable) {
		long result;
		synchronized(dbHelper) {
			open();
			ContentValues values = new ContentValues();
			values.put(KEY_EID, eid);
			values.put(KEY_NAME, name);
			values.put(KEY_TYPE, type);
			values.put(KEY_DESC, description);
			values.put(KEY_READ, readable);
			values.put(KEY_WRITE, writable);
			result = db.insert(DB_EID_TABLE, null, values);
			close();
		}

		
		for(HexaDroidDbListener listener : listeners) {
			listener.update();
		}
		return result;
	}

	public int updateEndpoint(long eid, String name, String type, String description, boolean readable, boolean writable) {
		int result;
		synchronized(dbHelper) {
			open();
			ContentValues values = new ContentValues();
			values.put(KEY_NAME, name);
			values.put(KEY_TYPE, type);
			values.put(KEY_DESC, description);
			values.put(KEY_READ, readable);
			values.put(KEY_WRITE, writable);
			result = db.update(DB_EID_TABLE, values, KEY_EID + "=" + eid, null);
			close();
		}

		
		for(HexaDroidDbListener listener : listeners) {
			listener.update();
		}
		return result;		
	}

	public boolean removeEndpoint(long eid) {
		boolean result;
		synchronized(dbHelper) {
			open();
			result = db.delete(DB_EID_TABLE, KEY_EID + "=" + eid, null) > 0;
			close();
		}
		
		for(HexaDroidDbListener listener : listeners) {
			listener.update();
		}
		return result;
	}

	public Cursor fetchAllEndpoints() {
		Cursor cursor;
		synchronized(dbHelper) {
			open();
			cursor = db.query(DB_EID_TABLE, new String[] {KEY_EID, KEY_NAME, KEY_TYPE, KEY_DESC, KEY_READ, KEY_WRITE},
					null, null, null, null, null);
			close();
		}
		return cursor;
	}

	public Cursor fetchEndpoint(long eid) {
		Cursor cursor;
		synchronized(dbHelper) {
			open();
			cursor = db.query(true, DB_EID_TABLE, new String[] {KEY_EID, KEY_NAME, KEY_TYPE, KEY_DESC, KEY_READ, KEY_WRITE},
					KEY_EID + "=" + eid, null, null, null, null, null);
			if(cursor != null) {
				cursor.moveToFirst();
			}
			close();
		}
		return cursor;
	}
	
	public void register(HexaDroidDbListener listener) {
		synchronized(listeners) {
			listeners.add(listener);
		}
	}
	
	public interface HexaDroidDbListener {
		public void update();
	}

}
