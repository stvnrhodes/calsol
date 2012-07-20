package messages;

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

	/**
	 * Sets up a new Accelerometer Message. The constructor first determines
	 * whether there is a stamp associated with the Message, and then
	 * acts accordingly.
	 * @param info The split String coming from the datalogger
	 * line. Element 0 should be an opCode. Element 1 should either
	 * be a stamp, or the payload that should be processed 
	 * accordingly. Element 2, if it exists, will be part of the payload.
	 * @param ts <b>True</b> means that there is a stamp. 
	 * <b>False</b> means that there is no stamp.
	 */
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
			out += stamp.toString() + ";";
			out += header.get(0) + ";" + (char)('X' + i) + ";";
		    out += data.get(i);
			packed.add(out);
		    out = "";
		}
		return packed;
	}
}
