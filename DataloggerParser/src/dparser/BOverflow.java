package dparser;

public class BOverflow extends Message {
	
	public BOverflow(String [] info, boolean ts) {
		super(info,ts);
		header = "Buffer Overflow";
	}

}
