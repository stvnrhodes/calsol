package messages;

import java.util.ArrayList;

import exceptions.PackException;

/**
 * Abstract message class. This is the basis for all messages
 * coming from the datalogger. All messages have an opcode,
 * possibly a stamp, and maybe a payload. This class
 * handles that ambiguity. It is extended by the various types
 * of message classes to handle the different opCodes.
 * @author Derek Chou
 * @since 2012.05.15
 */
public abstract class Message {
	
	/**
	 * Does the Message have a stamp? If it does,
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
	 * This is set to whatever the stamp of the
	 * Message is, if there is one. It does NOT contain
	 * the stamp error. <br>
	 * Defaults to null if there is no stamp.
	 */
	protected Timestamp stamp = new Timestamp();
	
	
	
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
	 * there is a stamp associated with the Message, and then
	 * acts accordingly.
	 * @param info The split String coming from the datalogger
	 * line. Element 0 should be an opCode. Element 1 should either
	 * be a stamp, or the payload that should be processed 
	 * accordingly. Element 2, if it exists, will be part of the payload.
	 * @param ts <b>True</b> means that there is a stamp. 
	 * <b>False</b> means that there is no stamp.
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
	 * @return The stamp of the message, if it exists.
	 * Otherwise, it returns <b>null</b>
	 */
	public Double getTimestamp() {
		if (timestamped)
		    return stamp.getValue();
		else
			return null;
	}
	
	/**
	 * Sets the stamp for a timestamped Message.
	 * @param s The purported String that contains stamp information.
	 */
	public void setTimestamp(String s) {
		if (timestamped)
			stamp.setTime(s);
		else
			return;
	}
	
	/**
	 * @return An array of Strings that includes the Message's 
	 * stamp (if it exists), the error on the stamp (if it exists),
	 * the header, and the data carried by the Message (if it exists).
	 */
	public String [] params() {
		String ts = "";
		if (timestamped) {
			try {
				ts = stamp.toString();
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
				ts = stamp.toString();
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
	 * @return The major parameters of a Message stamp,
	 * header, and data in that order. 
	 * This is for the Matlab reader - the format will be as
	 * follows: <br>
	 * stamp;name;description;data
	 */
	public ArrayList<String> pack() throws PackException {
		ArrayList<String> packed = new ArrayList<String>();
		String out = "";
		if (data.size() < 1 || header.size() < 2)
			throw new PackException();
		for (int i = 0; i < header.size() - 1; i++) {
			out += stamp.toString() + ";";
			out += header.get(0) + ";";
			out += header.get(i + 1) + ";";
			out += data.get(i);
			packed.add(out);
		    out = "";
		}
		return packed;
	}
	
	/**
	 * Prints out the Message and its stamp and its error. If the stamp
	 * and/or error do not exist, they will not be printed.
	 */
	@Override
	public String toString() {
		String out = "";
		if (timestamped) {
			try {
				out += stamp.toString() + "\r\n";
			} catch (NullPointerException e) {
				out += stamp.toString() + "\r\n";
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
