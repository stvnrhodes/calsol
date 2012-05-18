package dparser;

public class vPerf extends Message {
	
	public vPerf(String [] info, boolean ts) {
		super(info,ts);
		header = "Statistical Voltage Performance";
	}

}
