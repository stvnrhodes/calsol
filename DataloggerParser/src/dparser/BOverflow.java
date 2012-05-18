package dparser;

/**
 * This Message tells when there is a buffer overflow
 * during a datalogger write session. There is data lost
 * between this message and the next because the datalogger
 * was not able to write the info onto the SD card quickly
 * enough.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class BOverflow extends Message {
	
	public BOverflow(String [] info, boolean ts) {
		super(info,ts);
		header = "Buffer Overflow";
	}

}
