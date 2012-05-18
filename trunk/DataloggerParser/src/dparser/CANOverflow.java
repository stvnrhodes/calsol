package dparser;

/**
 * This type of message tells when there is an 
 * overflow of info coming from CAN. There is 
 * data lost between this message and the next 
 * because too much data was coming in and the
 * datalogger was not able to log the data quickly
 * enough.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class CANOverflow extends Message {
	
	public CANOverflow(String [] info, boolean ts) {
		super(info,ts);
		header = "CAN Overflow";
	}

}
