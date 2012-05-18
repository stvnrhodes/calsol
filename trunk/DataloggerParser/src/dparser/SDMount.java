package dparser;

public class SDMount extends Message {
	
	public SDMount(String [] info, boolean ts) {
		super(info,ts);
		header = "SD Card Mounted";
	}

}
