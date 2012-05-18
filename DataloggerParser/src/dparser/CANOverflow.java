package dparser;

public class CANOverflow extends Message {
	
	public CANOverflow(String [] info, boolean ts) {
		super(info,ts);
		header = "CAN Overflow";
	}

}
