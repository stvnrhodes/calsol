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
		if (format == null)
			return;
		data = new ArrayList<String>();
		try {
		    ArrayList<String> payload = 
				    decodeBytes(info[info.length-1].split(","), format);
		    for (String f : payload) {
			    data.add(f);
	        }
		} catch (IndexOutOfBoundsException e) {
			data.add("Error decoding payload");
			e.printStackTrace();
			return;
		} catch (NullPointerException e) {
			data.add("Error decoding payload");
			e.printStackTrace();
			return;
		} catch (DecodeException e) {
			data.add("Error decoding payload");
			e.printStackTrace();
			return;
		}
	}
	
	/**
	 * Converts an array of Strings (presumably representing bytes)
	 * into values. This uses the format found on
	 * <a href = "http://docs.python.org/py3k/library/struct.html">
	 * http://docs.python.org/py3k/library/struct.html</a>
	 * @param bytes : Input payload of bytes to be converted to values.
	 * The endianness on x86 machines is "little", so the chunks of 
	 * bytes read must be reversed before they are of any use.
	 * @param format : The format used to decode the String array. <br>
	 * x, s, p, and P require a number preceding them to designate
	 * the number of bytes allocated to them.
	 * As of 2012.05.22, only x, B, H, f, L, and s are implemented.
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
	 * @return An ArrayList of Strings, decoded from the array of
	 * bytes. These represent the numerical values of the bytes.
	 * @throws IndexOutOfBoundsException If the array decoding somehow
	 * reaches an end of a list unexpectedly
	 * @throws NullPointerException If one of the objects referenced
	 * somehow becomes null.
	 * @throws DecodeException If the payload does not have sufficient
	 * information for the parser to decode. (aka. missing bytes)
	 */
	private ArrayList<String> decodeBytes(String [] bytes, String format) 
			throws IndexOutOfBoundsException, NullPointerException,
			DecodeException {
		String lng = "";
		ArrayList<String> out = new ArrayList<String>();
		char [] formatted = format.toCharArray();
		int index = 0;
		int num = 0;
		for (char c : formatted) {
			switch (c) {
			case 'B':
				out.add("" + Integer.parseInt(bytes[index], 16));
				index++;
				break;
			case 'H':
				index++;
				for (int i = 0; i < 2; i++)
					lng += bytes[index-i];
				out.add("" + Integer.parseInt(lng, 16));
				lng = "";
				index++;
				break;
			case 'f':
				index += 3;
				for (int i = 0; i < 4; i++)
					lng += bytes[index-i];
				out.add(""
					+ Float.intBitsToFloat((int)(Long.parseLong(lng, 16))));
				lng = "";
				index++;
				break;
			case '1':
			    /*fall through*/
			case '2':
			    /*fall through*/	
			case '3':
			    /*fall through*/
			case '4':
			    /*fall through*/
			case '5':
				/*fall through*/
			case '6':
				/*fall through*/
			case '7':
				/*fall through*/
			case '8':
				/*fall through*/
			case '9':
				/*fall through*/
			case '0':
				num = c - 48;
				break;
			case 's':
				String s = "";
				try {
					for(int i = 0; i <= num; i++, ++index) {
					    s += (char)Integer.parseInt(bytes[index]);
				        }
				} catch (NumberFormatException e) {
					throw new DecodeException();
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