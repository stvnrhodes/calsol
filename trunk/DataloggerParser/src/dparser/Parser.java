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
import messages.Message;
import messages.SDMessage;
import messages.SDMount;
import messages.VoltageMessage;
import messages.VoltagePerformance;

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
 * @version 1.20
 */
public class Parser {

    private final int CAN_ID_LOC = 3;

    /**
     * Tells whether we want to output the ArrayLists' toString()s stored
     * inside each message. Defaults to false.
     */
    private boolean debug = false;

    /**
     * Tells whether we are in valid data territory. If the lines being
     * parsed are still initialization messages, then ignore everything
     * until the initialization is done.
     */
    private boolean dataValid = false;

    /**
     * An ArrayList of all JSON Objects created from initializing the Parser.
     */
    private static ArrayList<JSONObject> decoder = new ArrayList<JSONObject>();

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
    @SuppressWarnings("unused")
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
     * <tr><td>CAN Message</td><td><b>CRM</b></td><td>2</td></tr>
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
     * <td><b>PFM</b></td><td>7</td></tr>
     * 
     * <tr><td>Statistical Voltage Measurement</td>
     * <td><b>VLT</b></td><td>8</td></tr>
     * 
     * <tr><td>SD Card Dismount</td><td><b>DM</b></td><td>9</td></tr>
     * 
     * <tr><td>CAN Transmit Error</td><td><b>CTE</b></td><td>10</td></tr>
     * 
     * <tr><td>Real Time Clock</td><td><b>T_RTC</b></td><td>11</td></tr>
     * 
     * <tr><td>CAN Receive Error</td><td><b>CRE</b></td><td>12</td></tr>
     */
    private enum code {
        ACL  (0),
        BOVF (1),
        CRM  (2),
        COVF (3),
        MOVF (3),
        CRD  (4),
        PRM  (5),
        PWM  (5),
        MNT  (6),
        PFM  (7),
        VLT  (8),
        DM   (9),
        CTE  (10),
        T_RTC(11),
        CRE  (12);
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
     * @return An ArrayList of Strings that have been parsed.
     */
    public Message parse(String data) {
        ArrayList<String> split = new ArrayList<String>();
        String [] op = data.split("\\s");
        int opCode = -1;
        try {
            opCode = code.valueOf(op[0]).num;
        } catch (IllegalArgumentException e) {
            // Do absolutely nothing!
        }
        if (op[0].equalsIgnoreCase("T_RTC"))
            dataValid = true;
        if (dataValid) {
            for (String s : op)
                if (!s.isEmpty())
                    split.add(s);
            String [] sp = split.toArray(new String[split.size()]);
            Message temp = null;
            switch (opCode) {
            case 0:
                return new Accelerometer(sp, true);
            case 1:
                return new BOverflow(sp, true);
            case 2:
                boolean isError = false;
                if (sp[2].equalsIgnoreCase("covf")
                        || sp[2].equalsIgnoreCase("movf")) {
                    isError = true;
                    return new CANMessage(sp, true, isError, null);
                } else {
                    ArrayList<String> h = decodeJSON(sp);
                    try {
                        String format = h.remove(h.size()-1);
                        temp = new CANMessage(sp, true, isError, format);
                        temp.setHeader(h);
                    } catch (IndexOutOfBoundsException e) {
                        temp = new CANMessage(sp, true, isError, null);
                        ArrayList<String> tempList =
                                new ArrayList<String>();
                        tempList.add("Unknown CAN Message");
                        temp.setHeader(tempList);
                    }
                }
                return temp;
            case 3:
                return new CANOverflow(sp, false);
            case 4:
                return new SDMessage(sp, true);
                /*
				Don't know if we want parameters messages later.
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
                return new SDMount(sp, true);
            case 7:
                return new VoltagePerformance(sp, true);
            case 8:
                return new VoltageMessage(sp, true);
                /*
				Don't want dismount messages as of now
				case 9:
					temp = new DismountMessage(sp, true);
					addToMatrix(temp);
					break;
                 */
            case 10:
                return new CTMessage(sp, true);
            default:
                return temp;
            }
        }
        return null;
    }

    /**
     * Decodes a CAN Message using JSON libraries. This method
     * implements some pre-made JSON decoders, imported. This triple
     * for loop is there because someone decided to put values inside
     * values for the JSON files.
     * @param sp The payload incoming from a CAN Message.
     * @return An ArrayList of Strings that indicates the parsed data.
     */
    private ArrayList<String> decodeJSON(String[] sp) {
        JSONObject A = null;
        ArrayList<String> h = new ArrayList<String>();
        for (int i = 0; i < decoder.size(); i++) {
            if(decoder.get(i).has("0x" + sp[CAN_ID_LOC]))
                try {
                    A = decoder.get(i).getJSONObject("0x" + sp[CAN_ID_LOC]);
                    try {
                        h.add("0x" + sp[3] + " " + A.getString("name"));
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

    /**
     * @param t True or false, whether we want to debug this program or not.
     * @throws FileNotFoundException If we can't open the debug file output.
     */
    public void setVerbose(boolean t) throws FileNotFoundException {
        debug = t;
        db = new PrintStream(new File("Debug.txt"));
    }

    /**
     * @return Whether we're in Debug mode or not.
     */
    public boolean getVerbose() {
        return debug;
    }

    /**
     * Garbage collection.
     */
    public void clearFields() {
        dataValid = false;
    }
}
