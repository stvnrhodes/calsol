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
		ArrayList<Float> payload = decode(info[info.length-1].split(","));
		for (Float f : payload) {
			data.add(f.toString());
		}
	}
	
	/**
	 * Converts an array of Strings into Float values.
	 * @param s : Input payload of bytes to be converted to Float.
	 * @return An ArrayList of Floats, decoded from the array of
	 * bytes, represented by an array of Strings.
	 */
	private ArrayList<Float> decode(String [] s) {
		int ct = 1;
		String lng = "";
		ArrayList<Float> out = new ArrayList<Float>();
		for (String Byte : s) {
			lng += Byte;
			ct++;
			if (ct % 4 == 0) {
				ct = 1;
				out.add(Float.intBitsToFloat((int) Long.parseLong(lng, 16)));
			}
		}
		return out;
	}

}
