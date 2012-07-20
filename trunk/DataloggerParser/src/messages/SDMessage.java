package messages;

import java.util.ArrayList;

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
		
		/* This is a little bit hard-coded, but then again,
		 * this was the format given me.
		 */
		header.add("SD Card Information");
		header.add("Manufacturer ID (hex)");
		header.add("OEM/Application ID");
		header.add("Product Name");
		header.add("Product Revision");
		header.add("Product Serial Number (hex)");
		header.add("Manufacturing Date (hex)");
		
		data = new ArrayList<String>();
		for (int i = 2; i < info.length; i++) {
				data.add(info[i]);
			}
	}
}
