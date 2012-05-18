package dparser;

public class VoltageMessage extends Message {
	
	public VoltageMessage(String [] info, boolean ts) {
		super(info,ts);
		header = "Statistical Voltage Measurement";
		
	}

}
