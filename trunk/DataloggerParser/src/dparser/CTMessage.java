package dparser;

import java.util.ArrayList;

/**
 * This is a CAN Transmit message. It is usually some
 * sort of error from CAN.
 * @author Derek Chou
 * @since 2012.05.16
 */
public class CTMessage extends Message {
	
	public CTMessage(String [] info, boolean ts) {
		super(info,ts);
		header.add("CAN Transmit Message");
		isCAN = true;
		data = new ArrayList<String>();
		for (int i = 2; i < info.length; i++)
			data.add(info[i]);
	}

}
