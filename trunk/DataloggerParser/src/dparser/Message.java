package dparser;

import java.util.ArrayList;

/**
 * Abstract message class. This is the basis for all messages
 * coming from the datalogger. All messages have an opcode,
 * possibly a timestamp, and maybe a payload. This class
 * handles that ambiguity. It is extended by the various types
 * of message classes to handle the different opCodes.
 * @author Derek Chou
 * @since 2012.05.15
 */
public abstract class Message {
	
	/**
	 * Does the Message have a timestamp? If it does,
	 * this is set to true. Otherwise, this defaults to 
	 * false.
	 */
	public boolean timestamped = false;
	
	/**
	 * Is the Message a CAN Message?
	 * If so, this is set to true. Otherwise, this defaults to false.
	 */
	public boolean isCAN = false;
	
	/**
	 * Does the Message have data? If so, this is set to true. Otherwise,
	 * this defaults to false.
	 */
	public boolean hasData = false;
	
	/**
	 * This Double is set to whatever the timestamp of the
	 * Message is, if there is one. It does NOT contain
	 * the timestamp error. <br>
	 * Defaults to null if there is no timestamp.
	 */
	protected Double timestamp = null;
	
	/**
	 * While <b>timestamp</b> is a 
	 * decimal number, timestampFrac is an Integer meant to 
	 * represent the numerator over 1024. Defaults to null if
	 * there is no timestamp error.
	 */
	protected Integer timestampFrac = null;
	
	/**
	 * This Double is set to whatever the timestamp error
	 * of the Message is, if there is one.<br>
	 * Defaults to null if there is no timestamp.
	 */
	protected Double tsError = null;
	
	/**
	 * While <b>tsError</b> is a decimal number,
	 * tsErrorFrac is an Integer meant to represent the
	 * numerator over 1024. Defaults to null if there is
	 * no timestamp error.
	 */
	protected Integer tsErrorFrac = null;
	
	/**
	 * All Messages *should* have a header, but this may not be
	 * the case. This is intended to be the name that is sorted by
	 * the parser in the matrix of data, and the human-readable
	 * name of the parameter being parsed.
	 */
	protected ArrayList<String> header = new ArrayList<String>();
	
	/**
	 * This is analagous to the payload in the datalogger
	 * messages.
	 */
	protected ArrayList<String> data = null;
	
	/**
	 * This is the timebase that is currently being used with the 
	 * datalogger. It is sure to change once the PRM messages
	 * are actually of use.
	 */
	protected Double time = 1.0/1024;
	
	/**
	 * This is the voltbase that is currently being used with the 
	 * datalogger. It is sure to change once the PRM messages are
	 * actually of use.
	 */
	protected Double vBase = 1.0/1024;
	
	/**
	 * Sets up a new Message. The constructor first determines whether
	 * there is a timestamp associated with the Message, and then
	 * acts accordingly.
	 * @param info The split String coming from the datalogger
	 * line. Element 0 should be an opCode. Element 1 should either
	 * be a timestamp, or the payload that should be processed 
	 * accordingly. Element 2, if it exists, will be part of the payload.
	 * @param ts <b>True</b> means that there is a timestamp. 
	 * <b>False</b> means that there is no timestamp.
	 */
	public Message(String [] info, boolean ts) {
		timestamped = ts;
		if (timestamped) {
			setTimestamp(info[1]);
		}
	}
	
	/**
	 * @return The human-readable <b>header</b> field.
	 * This is basically the column header.
	 */
	public ArrayList<String> getHeader() {
		return header;
	}
	/**
	 * @param s The String that the header is to be 
	 * set equal to.
	 */
	public void setHeader(ArrayList<String> s) {
		header = s;
	}
	
	/**
	 * @return The timestamp of the message, if it exists.
	 * Otherwise, it returns <b>null</b>
	 */
	public Double getTimestamp() {
		if (timestamped)
		    return timestamp;
		else
			return null;
	}
	
	/**
	 * Sets the timestamp for a timestamped Message.
	 * @param s The purported String that contains timestamp information.
	 */
	public void setTimestamp(String s) {
		if (timestamped)
			if (s.indexOf('/') != -1) {
			    timestampFrac = Integer.parseInt
			    		(s.substring(0,s.indexOf('/')), 16);
			    tsErrorFrac = Integer.parseInt
			    		(s.substring(s.indexOf('/')+1),16);
			    timestamp = time * timestampFrac;
			    tsError = time * tsErrorFrac;
		    }
			else {
				timestampFrac = Integer.parseInt(s, 16);
				timestamp = time * timestampFrac;
			}
		else
			return;
	}
	
	/**
	 * @return An array of Strings that includes the Message's 
	 * timestamp (if it exists), the error on the timestamp (if it exists),
	 * the header, and the data carried by the Message (if it exists).
	 */
	public String [] params() {
		String ts = "";
		if (timestamped) {
			try {
				ts = timestamp.toString() + " ± " 
						+ tsError.toString();
				String h = header.toString();
				try {
					String d =  data.toString();
					String [] out = {ts, h, d};
					return out;
				} catch (NullPointerException e) {
					String [] out = {ts, h};
					return out;
				}
			} catch (NullPointerException e) {
				ts = timestamp.toString();
				String h = header.toString();
				try {
					String d =  data.toString();
					String [] out = {ts, h, d};
					return out;
				} catch (NullPointerException f) {
					String [] out = {ts, h};
					return out;
				}
			}
		}
		else {
			String h = header.toString();
			try {
				String d =  data.toString();
				String [] out = {h, d};
				return out;
			} catch (NullPointerException e) {
				String [] out = {h};
				return out;
			}
		}
	}
	
	/**
	 * Intended only for use with timestamped Messages going to Motor team.
	 * @return The major parameters of a Message timestamp,
	 * header, and data in that order. 
	 * This is for the Matlab reader - the format will be as
	 * follows: <br>
	 * timestamp;name;description;data
	 */
	public ArrayList<String> pack() throws PackException {
		ArrayList<String> packed = new ArrayList<String>();
		String out = "";
		if (data.size() < 1 || header.size() < 2)
			throw new PackException();
		for (int i = 0; i < header.size() - 1; i++) {
			out += timestamp.toString() + ";";
			out += header.get(0) + ";";
			out += header.get(i + 1) + ";";
			out += data.get(i);
			packed.add(out);
		    out = "";
		}
		return packed;
	}
	
	/**
	 * Prints out the Message and its timestamp and its error. If the timestamp
	 * and/or error do not exist, they will not be printed.
	 */
	@Override
	public String toString() {
		String out = "";
		if (timestamped) {
			try {
				out += timestamp.toString() + " ± " 
						+ tsError.toString() + "\r\n";
			} catch (NullPointerException e) {
				out += timestamp.toString() + "\r\n";
			} finally {
				out += header.toString() + "\r\n" + data.toString() + "\r\n";
			}
		}
		else {
			out += header.toString() + "\r\n" + data.toString() + "\r\n";
		}
		return out;
	}
}
