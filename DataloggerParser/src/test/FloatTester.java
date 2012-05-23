package test;

/**
 * Float decoding tester.
 * There appears to be integer wraparound.
 * @author Derek Chou
 * @since 2012.05.21
 */
public class FloatTester {

	/**
	 * @param args : Main class String [] args thing.
	 */
	public static void main(String[] args) {
		String test = "AF477540";
		Long l = Long.parseLong(test, 16);
		System.out.println(Long.toBinaryString(l));
		float asFloat = Float.intBitsToFloat((int) Long.parseLong(test, 16));
		System.out.println(asFloat);
	}

}
