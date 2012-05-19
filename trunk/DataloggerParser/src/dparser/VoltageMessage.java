package dparser;

import java.util.ArrayList;

/**
 * This is a statistical voltage measurement message
 * from the datalogger. It includes in its data ArrayList
 * a channel ID, the number of samples taken, min, avg, and max
 * values for the voltage measurements in the interval.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class VoltageMessage extends Message {
	
	public VoltageMessage(String [] info, boolean ts) {
		super(info,ts);
		header.add("Statistical Voltage Measurement");
		/* This is a little bit hard-coded, but then again,
		 * this was the format given me.
		 */
		header.add("");
		data = new ArrayList<String>();
		for (int i = 2; i < info.length; i++)
			data.add(info[i]);
	}

}
