package dparser;

import java.util.ArrayList;
/**
 * This is a message from CAN. This class implements the
 * format from JSON configuration files to decode what type of CAN
 * message is being sent, and has data corresponding to that.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class CANMessage extends Message {
	
	public CANMessage(String [] info, 
			boolean ts, boolean error, String format) {
		super(info,ts);
		isCAN = true;
		if (error)
			return;
		data = new ArrayList<String>();
		try {
		    ArrayList<Object> payload = 
				    decode(info[info.length-1].split(","), format);
		    for (Object f : payload) {
			    data.add(f.toString());
	        }
		} catch (IndexOutOfBoundsException e) {
			data.add("Error decoding payload");
			return;
		} catch (NullPointerException e) {
			data.add("Error decoding payload");
			return;
		}
	}
	
	/**
	 * Converts an array of Strings (presumably representing bytes)
	 * into values. This uses the format found on
	 * <a href = "http://docs.python.org/py3k/library/struct.html">
	 * http://docs.python.org/py3k/library/struct.html</a>
	 * The ArrayList of Objects is meant to hold Strings and Numbers.
	 * It might be changed into an ArrayList of Strings since the 
	 * output will still be equivalent.
	 * @param bytes : Input payload of bytes to be converted to values.
	 * @param format : The format used to decode the String array. <br>
	 * x, s, p, and P require a number preceding them to designate
	 * the number of bytes allocated to them.
	 * As of 2012.05.22, only x, B, H, f, and s are implemented.
	 * <table>
	 * <tr><td>Code</td><td>C Type</td><td>Python Type</td>
	 * <td>Size(Bytes)</td>
	 * 
	 * <tr><td><b>x</b></td><td>pad byte</td>
	 * <td>no value</td><td>1</td></tr>
	 * 
	 * <tr><td><b>c</b></td><td><code>char</code></td><td>bytes, length 1</td>
	 * <td>1</td></tr>
	 * 
	 * <tr><td><b>b</b></td><td><code>signed char</code></td><td>integer</td>
	 * <td>1</td></tr>
	 * 
	 * <tr><td><b>B</b></td><td><code>unsigned char</code></td><td>integer</td>
	 * <td>1</td></tr>
	 * 
	 * <tr><td><b>?</b></td><td><code>_Bool</code></td><td>bool</td>
	 * <td>1</td></tr>
	 * 
	 * <tr><td><b>h</b></td><td><code>short</code></td><td>integer</td>
	 * <td>2</td></tr>
	 * 
	 * <tr><td><b>H</b></td><td><code>unsigned short</code></td>
	 * <td>integer</td><td>2</td></tr>
	 * 
	 * <tr><td><b>i</b></td><td><code>int</code></td><td>integer</td>
	 * <td>4</td></tr>
	 * 
	 * <tr><td><b>I</b></td><td><code>unsigned int</code></td>
	 * <td>integer</td><td>4</td></tr>
	 * 
	 * <tr><td><b>l</b></td><td><code>long</code></td>
	 * <td>integer</td><td>4</td></tr>
	 * 
	 * <tr><td><b>L</b></td><td><code>unsigned long</code></td>
	 * <td>integer</td><td>4</td></tr>
	 * 
	 * <tr><td><b>q</b></td><td><code>long long</code></td>
	 * <td>integer</td><td>8</td></tr>
	 * 
	 * <tr><td><b>Q</b></td><td><code>unsigned long long</code></td>
	 * <td>integer</td><td>8</td></tr>
	 * 
	 * <tr><td><b>f</b></td><td><code>float</code></td><td>float</td>
	 * <td>4</td></tr>
	 * 
	 * <tr><td><b>d</b></td><td><code>double</code></td><td>float</td>
	 * <td>8</td></tr>
	 * 
	 * <tr><td><b>s</b></td><td><code>char[]</code></td><td>bytes</td>
	 * <td></td></tr>
	 * 
	 * <tr><td><b>p</b></td><td><code>char[]</code></td><td>bytes</td>
	 * <td></td></tr>
	 * 
	 * <tr><td><b>P</b></td><td><code>void *</code></td><td>integer</td>
	 * <td></td></tr>
	 * 
	 * </table>
	 * @return An ArrayList of Objects, decoded from the array of
	 * bytes.
	 * @throws IndexOutOfBoundsException If the array decoding somehow
	 * reaches an end of a list unexpectedly
	 * @throws NullPointerException If one of the objects referenced
	 * somehow becomes null.
	 */
	private ArrayList<Object> decode(String [] bytes, String format) 
			throws IndexOutOfBoundsException, NullPointerException {
		String lng = "";
		ArrayList<Object> out = new ArrayList<Object>();
		char [] formatted = format.toCharArray();
		int index = 0;
		int num = 0;
		for (char c : formatted) {
			switch (c) {
			case 'B':
				out.add(Integer.parseInt(bytes[index], 16));
				index++;
				break;
			case 'H':
				for (int i = 1; i <= 2; i++, index++)
					lng += bytes[index];
				out.add(Integer.parseInt(lng, 16));
				lng = "";
				break;
			case 'f':
				for (int i = 1; i <= 4; i++, index++)
					lng += bytes[index];
				out.add(Float.intBitsToFloat((int) Long.parseLong(lng, 16)));
				lng = "";
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '0':
				num = c - 48;
				break;
			case 's':
				String s = "";
				for(; index <= num; index++) {
					s += (char)Integer.parseInt(bytes[index]);
				}
				out.add(s);
				break;
			/* Ignore num bytes */
			case 'x':
				for(; index <= num; index++);
				break;
			default:
				break;
			}
		}
		return out;
	}
}