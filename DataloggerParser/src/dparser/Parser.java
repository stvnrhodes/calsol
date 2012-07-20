package dparser;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Scanner;

import messages.Accelerometer;
import messages.BOverflow;
import messages.CANMessage;
import messages.CANOverflow;
import messages.CTMessage;
import messages.DismountMessage;
import messages.Message;
import messages.SDMessage;
import messages.SDMount;
import messages.VoltageMessage;
import messages.vPerf;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import exceptions.PackException;

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

	/**
	 * The first header that Motor Team is interested in.
	 */
	private ArrayList<String> m1 = new ArrayList<String>();
	
	/**
	 * The second header that Motor Team is interested in.
	 */
	private ArrayList<String> m2 = new ArrayList<String>();
	
	/**
	 * The third header that Motor Team is interested in.
	 */
	private ArrayList<String> m3 = new ArrayList<String>();
	
	/**
	 * If debug is set to <b>true</b>, then this printstream will be
	 * initialized. Otherwise, it will not.
	 */
	private PrintStream db;
	
	
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
	 * in the datalogger documentation. CAN Overflow may be unused since
	 * the code inside the CAN Message already handles the overflow message.
	 * <table>
	 * <tr><td>Message Type</td><td>OpCode</td><td>Value in Enum</td>
	 * <tr><td>Accelerometer</td><td><b>ACL</b></td><td>0</td></tr>
	 * 
	 * <tr><td>Buffer Overflow</td><td><b>BOVF</b></td><td>1</td></tr>
	 * 
	 * <tr><td>CAN Message</td><td><b>CM</b></td><td>2</td></tr>
	 * 
	 * <tr><td>CAN Overflow</td><td><b>COVF</b></td><td>3</td></tr>
	 * 
	 * <tr><td>CAN Overflow (typo)</td><td><b>MOVF</b></td><td>3</td></tr>
	 * 
	 * <tr><td>SD Card Information</td><td><b>CRD</b></td><td>4</td></tr>
	 * 
	 * <tr><td>Parameters</td><td><b>PRM</b></td><td>5</td></tr>
	 * 
	 * <tr><td>Parameters(typo)</td><td><b>PWM</b></td><td>5</td></tr> 
	 * 
	 * <tr><td>SD Card Mounted</td><td><b>MNT</b></td><td>6</td></tr> 
	 * 
	 * <tr><td>Statistical Performance Measurement</td>
	 * <td><b>PS</b></td><td>7</td></tr>
	 * 
	 * <tr><td>Statistical Voltage Measurement</td>
	 * <td><b>VS</b></td><td>8</td></tr>
	 * 
	 * <tr><td>SD Card Dismount</td><td><b>DM</b></td><td>9</td></tr>
	 * 
	 * <tr><td>CAN Transmit Error</td><td><b>CT</b></td><td>10</td></tr>
	 */
	private enum code {
		ACL  (0),
		BOVF (1),
		CM   (2),
		COVF (3),
		MOVF (3),
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
	 * @throws FileNotFoundException If the Scanner cannot
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
		m1.add("Accelerometer");
		m2.add("0x403 Velocity Measurement");
		m2.add(" Motor Velocity rpm Motor angular frequency in revolutions per minute.");
		m2.add(" Vehicle Velocity m/s Vehicle velocity in metres / second.");
		m3.add("0x40E Odometer & Bus AmpHours Measurement");
		m3.add(" Odometer meters the distance the vehicle has travelled " +
				"since reset.");
		m3.add(" DC Bus AmpHours Ampere-hours The charge flow into the " +
				"controller bus voltage from the time of reset.");
	}
	
	
	/**
	 * Parses the datalogger output into more intelligible
	 * things. Where the data goes or which ArrayList it's added
	 * into is dependent on its opCode and payload. This method
	 * currently ignores PRM/PWM messages.
	 * @param data A line of datalogger output.
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
			if (sp[2].equalsIgnoreCase("covf") 
					|| sp[2].equalsIgnoreCase("movf")) {
				isError = true;
				temp = new CANMessage(sp, true, isError, null);
				errors.add(temp);
				break;
			} else {
				ArrayList<String> h = decodeJSON(sp);
				try {
					String format = h.remove(h.size()-1);
					temp = new CANMessage(sp, true, isError, format);
					temp.setHeader(h);
				} catch (IndexOutOfBoundsException e) {
					temp = new CANMessage(sp, true, isError, null);
					ArrayList<String> tempList = new ArrayList<String>();
					tempList.add("Unknown CAN Message");
					temp.setHeader(tempList);
				}
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
				db.println(temp);
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
	 * @param sp The payload incoming from a CAN Message.
	 */
	private ArrayList<String> decodeJSON(String[] sp) {
		JSONObject A = null;
		ArrayList<String> h = new ArrayList<String>();
		for (int i = 0; i < decoder.size(); i++) {
			if(decoder.get(i).has("0x" + sp[5]))
				try {
					A = decoder.get(i).getJSONObject("0x" + sp[5]);
					try {
						h.add("0x" + sp[5] + " " + A.getString("name"));
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
					h.add(A.getString("format"));
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
	 * @param temp The message to be added to the large matrix of data
	 */
	private void addToMatrix(Message temp) {
		int i = findIndex(temp.getHeader());
		if (i != -1) {
			matrix.get(findIndex(temp.getHeader())).add(temp);
		} 
		else {
			matrix.add(new ArrayList<Message>());
			matrix.get(matrix.size()-1).add(temp);
			matrixIndex.add(temp.getHeader());
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
	
	public void setVerbose(boolean t) throws FileNotFoundException {
		debug = t;
		db = new PrintStream(new File("Debug.txt"));
	}
	
	public boolean getVerbose() {
		return debug;
	}
	
	public ArrayList<Message> getData() {
		return data;
	}
	
	/**
	 * @return All non-error data in an ArrayList of arrays of Strings.
	 */
	public ArrayList<String []> getStrings() {
		ArrayList<String []> list = new ArrayList<String []>();
		for (int i = 0; i < data.size(); i++) 
			list.add(data.get(i).params());
		return list;
	}
	
	/**
	 * @return All error data in an ArrayList of arrays of Strings.
	 */
	public ArrayList<String[]> getErrorStrings() {
		ArrayList<String []> list = new ArrayList<String []>();
		for (int i = 0; i < errors.size(); i++)
			list.add(errors.get(i).params());
		return list;
	}
	
	/**
	 * @return All data going to whomever wishes to process
	 * the Datalogger output in MATLAB.
	 */
	public ArrayList<String> getMatStrings() {
		ArrayList<String> out = new ArrayList<String>();
		out.ensureCapacity((int)(data.size() * 1.5));
		for(int i = 0; i < data.size(); i++) {
			Message tempMsg = data.get(i);
			if (!tempMsg.hasData)
				continue;
			ArrayList<String> temp;
			try {
				temp = tempMsg.pack();
			} catch (PackException e) {
				continue;
			}
			for (int j = 0; j < temp.size(); j++) {
				out.add(temp.get(j));
			}
		}
		return out;
	}
	
	/**
	 * @return All data going to Motor Team.
	 * This output is to be processed in MATLAB.
	 */
	public ArrayList<String> getMotorStrings() {
		ArrayList<String> out = new ArrayList<String>();
		out.ensureCapacity((int)(data.size() * 0.4));
		for(int i = 0; i < data.size(); i++) {
			Message tempMsg = data.get(i);
			if (!tempMsg.hasData)
				continue;
			if (tempMsg.getHeader().equals(m1) 
					|| tempMsg.getHeader().equals(m2) 
					|| tempMsg.getHeader().equals(m3)) {
				ArrayList<String> temp;
				try {
					temp = tempMsg.pack();
				} catch (PackException e) {
					continue;
				}
				if (tempMsg.getHeader().equals(m3)) {
					out.add(temp.get(0));
				} else {
				    for (int j = 0; j < temp.size(); j++) {
					    out.add(temp.get(j));
				    }
				}
			}
		}
		return out;
	}

	/**
	 * Garbage collection.
	 */
	public void clearFields() {
		data = new ArrayList<Message>();
		cfg = new ArrayList<Message>();
		errors = new ArrayList<Message>();
		matrixIndex = new ArrayList<ArrayList<String>>();
		matrix = new ArrayList<ArrayList<Message>>();
	}
}