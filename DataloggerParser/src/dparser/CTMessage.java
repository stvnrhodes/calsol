package dparser;

import java.util.ArrayList;

public class CTMessage extends Message {
	
	public CTMessage(String [] info, boolean ts) {
		super(info,ts);
		header = "CAN Transmit Message";
		data = new ArrayList<String>();
		for (int i = 2; i < info.length; i++)
			data.add(info[i]);
	}

}
