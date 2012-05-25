package test;

/**
 * Float decoding tester.
 * The bytes coming in seem to be
 * in reverse order.
 * @author Derek Chou
 * @since 2012.05.21
 */
public class FloatTester {

	/**
	 * @param args : Main class String [] args thing.
	 */
	public static void main(String[] args) {
		String test = "43010560";
		Long l = Long.parseLong(test, 16);
		float asFloat = Float.intBitsToFloat((int)Long.parseLong(test, 16));
		double asDouble = Double.longBitsToDouble(l);
		System.out.println(asFloat);
		System.out.println(asDouble);
		System.out.println(Integer.parseInt("EC", 16));
	}

}
