package dparser;

import java.util.ArrayList;

/**
 * This is a type of Message that accepts
 * Accelerometer data. Its <b>data</b> field
 * contains the X, Y, and Z params of the 
 * accelerometer.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class Accelerometer extends Message {

	public Accelerometer(String [] info, boolean ts) {
		super(info,ts);
		header.add("Accelerometer");
		data = new ArrayList<String>();
		for (int i = 3; i < info.length; i+=2) {
			data.add(info[i]);
		}
	}

}
