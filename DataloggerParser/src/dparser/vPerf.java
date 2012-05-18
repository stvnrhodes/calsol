package dparser;

/**
 * This is a "statistical performance measurement" as
 * documented by the Datalogger ASCII format specs.
 * It appears to have the same parameters as the statistical
 * voltage measurement, VoltageMessage.
 * It includes in its data ArrayList
 * a channel ID, the number of samples taken, min, avg, and max
 * values for the voltage measurements in the interval.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class vPerf extends Message {
	
	public vPerf(String [] info, boolean ts) {
		super(info,ts);
		header.add("Statistical Voltage Performance");
	}

}
