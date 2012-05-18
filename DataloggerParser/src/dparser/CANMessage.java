package dparser;

public class CANMessage extends Message {
	
	public CANMessage(String [] info, boolean ts) {
		super(info,ts);
		isCAN = true;
		
	}

}
