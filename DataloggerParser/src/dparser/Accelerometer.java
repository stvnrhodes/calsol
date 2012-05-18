package dparser;

import java.util.ArrayList;

public class Accelerometer extends Message {

	public Accelerometer(String [] info, boolean ts) {
		super(info,ts);
		header = "Accelerometer";
		data = new ArrayList<String>();
		for (int i = 3; i < info.length; i+=2) {
			data.add(info[i]);
		}
	}

}
