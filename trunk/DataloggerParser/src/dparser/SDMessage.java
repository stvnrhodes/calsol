package dparser;

/**
 * This is a message pertaining to the configs of the 
 * SD card. It contains SD card information as 
 * specified by the Datalogger ASCII Format pdf.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class SDMessage extends Message {
	
	public SDMessage(String [] info, boolean ts) {
		super(info,ts);
		
	}

}
