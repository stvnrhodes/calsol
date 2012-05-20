package dparser;

import java.util.ArrayList;
/**
 * This is a message from CAN. This class implements the
 * JSON configuration files to decode what type of CAN
 * message is being sent, and has data corresponding to that.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class CANMessage extends Message {
	
	public CANMessage(String [] info, boolean ts, boolean error) {
		super(info,ts);
		isCAN = true;
		if (error) 
			return;
		data = new ArrayList<String>();
		String [] payload = info[info.length-1].split(",");
		for (String s : payload) {
			data.add(new Integer(Integer.parseInt(s,16)).toString());
		}
	}

}
