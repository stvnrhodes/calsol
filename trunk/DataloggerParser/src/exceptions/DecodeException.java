package exceptions;

/**
 * Thrown when the CAN Message decoder runs into too few bytes
 * to decode a payload properly.
 * @author Derek Chou
 * @since 2012.05.23
 */
public class DecodeException extends Exception {

	/**
	 * Default serialVersionUID.
	 */
	private static final long serialVersionUID = 1L;

}
