package dparser;

public class dismountMessage extends Message {
	
	public dismountMessage(String [] info, boolean ts) {
		super(info,ts);
		header = "SD Card Dismounted";
	}

}
