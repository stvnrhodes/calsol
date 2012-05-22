package dparser;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Scanner;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Parser written for the CalSol datalogger
 * This is the actual class that does the hard work of
 * interpreting the datalogger signals. It deciphers the 
 * datalogger opCode first and then interprets the payload.
 * @author Derek Chou
 * @since 2012.05.13
 * @version 1.00
 */
public class Parser {
	
	/**
	 * Tells whether we want to output the ArrayLists' toString()s stored 
	 * inside each message. Defaults to false.
	 */
	private boolean debug = false;
	
	/**
	 * An ArrayList of all JSON Objects created from initializing the Parser.
	 */
	private static ArrayList<JSONObject> decoder = new ArrayList<JSONObject>();
	
	/**
	 * An ArrayList of all timestamped non-error data, be it accelerometer 
	 * data, CAN messages, etc.
	 */
	private static ArrayList<Message> data = new ArrayList<Message>();
	
	/**
	 * An ArrayList of SD card information and other configuration things.
	 */
	private static ArrayList<Message> cfg = new ArrayList<Message>();
	
	/**
	 * An ArrayList of all error messages logged by the datalogger in the 
	 * .dla file. This includes BOVF and COVF messages.
	 */
	private static ArrayList<Message> errors = new ArrayList<Message>();
	
	/**
	 * Contains the headers for the <b>matrix</b> ArrayList of ArrayLists so 
	 * that it is easy to locate the correct ArrayList in the matrix.
	 */
	private static ArrayList<ArrayList<String>> matrixIndex = 
			new ArrayList<ArrayList<String>>();
	
	/**
	 * This contains sorted data; basically a message-type determined matrix of
	 * timestamped non-error data.
	 */
	private static ArrayList<ArrayList<Message>> matrix = 
			new ArrayList<ArrayList<Message>>();
	
	/*
	private int CANChannel;
	private String CANChannelName;
	
	private int formatVer;
	
	private String firmwareVersion = "";
	
	private String hardwareVersion = "";
	
	private String timeBase = "";
	
	
	private String voltMeas = "";
	
	private String voltBase = "";
	*/
	
	
	/**
	 * This enum contains the different opCodes that are specified
	 * in the datalogger documentation.<br>
	 * Accelerometer: <b>ACL = 0</b> <br>
	 * Buffer Overflow: <b>BOVF = 1</b> <br>
	 * CAN Message: <b>CM = 2</b> <br>
	 * CAN Overflow: <b>COVF = 3</b> <br>
	 * SD Card Information: <b>CRD = 4</b> <br>
	 * Parameters: <b>PRM = 5</b> <br>
	 * Parameters(typo): <b>PWM = 5</b> <br>
	 * SD Card Mounted: <b>MNT = 6</b> <br>
	 * Statistical Performance Measurement: <b>PS = 7</b> <br>
	 * Statistical Voltage Measuremet: <b>VS = 8</b> <br>
	 * SD Card Dismount: <b>DM = 9</b> <br>
	 * CAN Transmit Error: <b>CT = 10</b> <br>
	 */
	private enum code {
		ACL  (0),
		BOVF (1),
		CM   (2),
		COVF (3),
		CRD  (4),
		PRM  (5),
		PWM  (5),
		MNT  (6),
		PS   (7),
		VS   (8),
		DM   (9),
		CT   (10);
		private int num;
		code (int num) {
			this.setNum(num);
		}
		public void setNum(int num) {
			this.num = num;
		}
	}
	
	/* 
	This will be implemented in a later version of the parser.
	private enum param {
		CANCHA   (0),
		FMT      (1),
		SW       (2),
		HW       (3),
		TIMEBASE (4),
		VOLTMEAS (5),
		VOLTBASE (6),
		INIT     (7);
		private int num;
		param (int num) {
			this.setNum(num);
		}
		public void setNum(int num) {
			this.num = num;
		}
	}
	*/
	
	/**
	 * Initializes the directory and files where the JSON 
	 * files are kept, so that the Parser may decode CAN 
	 * Message payloads when needed.
	 * @throws FileNotFoundException : If the Scanner cannot
	 * read the JSON files needed to decode the CAN Messages.
	 */
	public Parser() throws FileNotFoundException, JSONException {
		/*
		I suppose there's not another way to do this other than
		just coding each of the json files in...
		*/
		Scanner r;

		File [] json = {
				new File("CFG\\batteries.can.json"),
			    new File("CFG\\cutoff.can.json"),
			    new File("CFG\\dashboard.can.json"),
			    new File("CFG\\mppts.can.json"),
			    new File("CFG\\tritium.can.json")
			    };
		for (int i = 0; i < json.length; i++) {
			String f = "";
			r = new Scanner(json[i]);
			while(r.hasNext()) {
				f += "\n" + r.nextLine();
			}
			decoder.add(new JSONObject(f));
		}
	}
	
	
	/**
	 * Parses the datalogger output into more intelligible
	 * things. Where the data goes or which ArrayList it's added
	 * into is dependent on its opCode and payload. This method
	 * currently ignores PRM/PWM messages.
	 * @param data : A line of datalogger output.
	 */
	public void parse(String data) {
		String [] sp = data.split(" ");
		int opCode = code.valueOf(sp[0]).num;
		Message temp = null;
		switch (opCode) {
		case 0:
			temp = new Accelerometer(sp, true);
			addToMatrix(temp);
			break;
		case 1:
			errors.add(new BOverflow(sp, true));
			break;
		case 2:
			boolean isError = false;
			if (sp[2].equalsIgnoreCase("covf")) {
				isError = true;
				temp = new CANMessage(sp, true, isError);
				errors.add(temp);
				break;
			} else {
				ArrayList<String> h = decode(sp);
				temp = new CANMessage(sp, true, isError);
				temp.setHeader(h);
			}
			addToMatrix(temp);
			break;
		case 3:
			errors.add(new CANOverflow(sp, false));
			break;
		case 4:
			cfg.add(new SDMessage(sp, true));
			break;
		/* 
		This will be implemented in a later version of the parser
		case 5:
			try {
				params(sp);
			} catch (IndexOutOfBoundsException e) {
				// Do nothing!
				e.printStackTrace();
			}
			break;
		*/
		case 6:
			temp = new SDMount(sp, true);
			addToMatrix(temp);
			break;
		case 7:
			temp = new vPerf(sp, true);
			addToMatrix(temp);
			break;
		case 8:
			temp = new VoltageMessage(sp, true);
			addToMatrix(temp);
			break;
		case 9:
			temp = new DismountMessage(sp, true);
			addToMatrix(temp);
			break;
		case 10:
			errors.add(new CTMessage(sp, true));
			break;
		default:
			break;
			
		}
		try {
			if (debug && temp != null) {
				System.out.println(temp);
		    }	
		} catch(NullPointerException e) {
			/* Do absolutely nothing! */			
		}
	}
	
	/**
	 * Decodes a CAN Message using JSON libraries. This method 
	 * implements some pre-made JSON decoders, imported. This triple
	 * for loop is there because someone decided to put values inside
	 * values for the JSON files.
	 * @param sp : The payload incoming from a CAN Message.
	 */
	private ArrayList<String> decode(String[] sp) {
		JSONObject A = null;
		ArrayList<String> h = new ArrayList<String>();
		for (int i = 0; i < decoder.size(); i++) {
			if(decoder.get(i).has("0x" + sp[5]))
				try {
					A = decoder.get(i).getJSONObject("0x" + sp[5]);
					try {
						h.add(A.getString("name"));
					} catch (JSONException e) {
						/* Do nothing! */
					}
					JSONArray B = A.getJSONArray("messages");
					for (int j = 0; j < B.length(); j++) {
						JSONArray C = B.getJSONArray(j);
						String message = "";
						for (int k = 0; k < C.length(); k++) {
							message += " " + C.getString(k);
						}
						h.add(message);
					}
					return h;
				} catch (JSONException e) {
					/* Do nothing! */
				}
		}
		return h;
	}

	/**
	 * Finds the index that a piece of data should be 
	 * added to in the <b>matrix</b> ArrayList.
	 * @param header : The header of the Message that
	 * the method is using to find the 
	 * @return The index at which the specified header exists.
	 * -1 if the header does not exist.
	 */
	private int findIndex(ArrayList<String> header) {
		return matrixIndex.indexOf(header);
	}
	
	/**
	 * Adds a piece of non-error data to the ArrayList <b>matrix</b>.
	 * @param temp : The message to be added to the large matrix of data
	 */
	private void addToMatrix(Message temp) {
		int i = findIndex(temp.header);
		if (i != -1) {
			matrix.get(findIndex(temp.header)).add(temp);
		} 
		else {
			matrix.add(new ArrayList<Message>());
			matrix.get(matrix.size()-1).add(temp);
			matrixIndex.add(temp.header);
		}
		data.add(temp);
	}
	
	/* This will be implmented in a later version of the datalogger parser.
	private void params(String[] sp) throws IndexOutOfBoundsException {
		int paramCode = param.valueOf(sp[1]).num;
		switch (paramCode) {
		case 0:
			CANChannel = new Integer(sp[2]);
			CANChannelName = sp[3];
			break;
		case 1:
			formatVer = new Integer(sp[2]);
			break;
		case 2:
			for (int i = 2; i < sp.length; i++)
			    firmwareVersion += sp[i] + " ";
			break;
		case 3:
			hardwareVersion = sp[2];
			break;
		case 4:
			timeBase = sp[2];
			break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			break;
		default:
			break;
		}
	}
	*/
	
	public void setVerbose(boolean t) {
		debug = t;
	}
	
	public boolean getVerbose() {
		return debug;
	}
	
	@Override
	public String toString() {
		String out = "";
		matrix.toString();
		return out;
	}
}