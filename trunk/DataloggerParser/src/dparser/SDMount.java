package dparser;

/**
 * This type of Message tells when the SD card
 * was mounted.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class SDMount extends Message {
	
	public SDMount(String [] info, boolean ts) {
		super(info,ts);
		header = "SD Card Mounted";
	}

}
