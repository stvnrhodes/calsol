package dparser;

/**
 * If a Message does not have a header of length at least 2, or a 
 * data size of at least 1, this Exception is thrown.
 * @author Derek Chou
 * @since 2012.05.26
 */
public class PackException extends Exception {

	/**
	 * Default serial version UID
	 */
	private static final long serialVersionUID = 1L;

}
