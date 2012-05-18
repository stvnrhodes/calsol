package dparser;

/**
 * This type of Message is created when the parser
 * encounters a dismount line. Most likely, this means
 * that there isn't any data after this point in the file.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class dismountMessage extends Message {
	
	public dismountMessage(String [] info, boolean ts) {
		super(info,ts);
		header = "SD Card Dismounted";
	}

}
