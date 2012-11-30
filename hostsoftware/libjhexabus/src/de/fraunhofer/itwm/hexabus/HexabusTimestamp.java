package de.fraunhofer.itwm.hexabus;

public class HexabusTimestamp {
	private long timestamp = 0;
	public HexabusTimestamp(long timestamp) throws Hexabus.HexabusException {
		if(timestamp > 4294967295L) {
			// HexabusTimestamp is only 32 bit
			throw new Hexabus.HexabusException("Timestamp too large");
		} else {
			this.timestamp = timestamp;
		}
	}

	public long getLong() {
		return timestamp;
	}

	public String toString() {
		return Long.toString(timestamp);
	}
}
