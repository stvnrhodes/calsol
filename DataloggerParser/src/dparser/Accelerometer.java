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
		hasData = true;
	}
	
	@Override
	public ArrayList<String> pack() {
		ArrayList<String> packed = new ArrayList<String>();
		String out = "";
		for (int i = 0; i < data.size(); i++) {
			out += timestamp.toString() + ";";
			out += header.get(0) + ";" + (char)('X' + i) + ";";
		    out += data.get(i);
			packed.add(out);
		    out = "";
		}
		return packed;
	}
}
