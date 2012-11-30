package de.fraunhofer.itwm.hexabus;

import java.util.Calendar;

public class HexabusDatetime {
	private int year;
	private int month;
	private int day;
	private int weekday; // numbers from 0 to 6, sunday as the first day of the week.
	private int hour;
	private int minute;
	private int second;

	public HexabusDatetime(int year, int month, int day, int weekday, int hour, int minute, int second) {
		this.year = year;
		this.month = month;
		this.day = day;
		this.weekday = weekday;
		this.hour = hour;
		this.minute = minute;
		this.second = second;
	}

	public HexabusDatetime(Calendar calendar) {
		this.year = calendar.get(Calendar.YEAR);
		this.month = calendar.get(Calendar.MONTH)+1;
		this.day = calendar.get(Calendar.DAY_OF_MONTH);
		this.weekday = calendar.get(Calendar.DAY_OF_WEEK)-1;
		this.hour = calendar.get(Calendar.HOUR_OF_DAY);
		this.minute = calendar.get(Calendar.MINUTE);
		this.second = calendar.get(Calendar.SECOND);
	}

	public int getYear() {
		return year;
	}

	public int getMonth() {
		return month;
	}

	public int getDay() {
		return day;
	}

	public int getWeekday() {
		return weekday;
	}

	public int getHour() {
		return hour;
	}

	public int getMinute() {
		return minute;
	}

	public int getSecond() {
		return second;
	}

	public String toString() {
		String date = "";
		switch(weekday) {
			case 0:
				date += "Sun, ";
				break;
			case 1:
				date += "Mon, ";
				break;
			case 2:
				date += "Tue, ";
				break;
			case 3:
				date += "Wed, ";
				break;
			case 4:
				date += "Thu, ";
				break;
			case 5:
				date += "Fri, ";
				break;
			case 6:
				date += "Sat, ";
				break;
			default:
				//TODO Exception?
				date += "unknown weekday, ";
		}

		date += day+"."+month+"."+year+", "+hour+":"+minute+":"+second;

		return date;
	}

}
