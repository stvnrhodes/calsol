package test;

/**
 * Float decoding tester.
 * @author Derek Chou
 * @since 2012.05.21
 */
public class FloatTester {

	/**
	 * @param args : Main class String [] args thing.
	 */
	public static void main(String[] args) {
		String test = "C0000000";
		float asFloat = Float.intBitsToFloat((int) Long.parseLong(test, 16));
		System.out.println(asFloat);
	}

}
