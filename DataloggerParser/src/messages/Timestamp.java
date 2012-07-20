package messages;

/**
 * All Messages have a timestamp. This class
 * handles the decoding of all timestamps that
 * Messages have.
 * @author Derek Chou
 * @since 2012.0717
 */
public class Timestamp {
	
	/**
	 * The number of seconds since the car has started
	 */
	private Integer seconds;
	
	/**
	 * The fractions of seconds associated with the timestamp
	 */
	private Integer fractions;
	
	/**
	 * The error on the timestamp, in timebase fractions of 
	 * seconds.
	 */
	private Integer error;
	
	/**
	 * 
	 */
	private boolean hasError = false;
	
	/**
	 * The double value of the timestamp.
	 */
	private Double time;
	
	/**
	 * This number goes in the denominator of the fractions
	 * of seconds bit.
	 */
	private static final int TIMEBASE = 1024;
	
	
	/**
	 * Sets the value of the timestamp.
	 * @param s The String containing the timestamp
	 * data.
	 */
	public void setTime(String s) {
		if(s.indexOf('/') != -1) {
			seconds = Integer.parseInt(
					s.substring(0, s.indexOf('.')));
			fractions = Integer.parseInt(
					s.substring(s.indexOf('.') + 1, 
					s.indexOf('/')));
			error = Integer.parseInt(
					s.substring(s.indexOf('/') + 1));
			time = (seconds + (double)fractions / TIMEBASE);
		} else {
			seconds = Integer.parseInt(
					s.substring(0, s.indexOf('.')));
			fractions = Integer.parseInt(
					s.substring(s.indexOf('.') + 1));
			error = null;
			time = (seconds + (double)fractions / TIMEBASE);
		}
	}

	/**
	 * @return The Double value of the timestamp.
	 */
	public Double getValue() {
		return time;
	}
	
	@Override
	public String toString() {
		String out = "";
		out += time.toString();
		if (hasError)
			out += " ± " + error.toString();
		return out;
	}

}
