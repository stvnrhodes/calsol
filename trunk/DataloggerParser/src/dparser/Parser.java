package dparser;

import java.util.ArrayList;

/**
 * Parser written for the CalSol datalogger
 * This is the actual class that does the hard work of
 * interpreting the datalogger signals. It deciphers the 
 * datalogger opCode first and then interprets the payload.
 * @author Derek Chou
 * @since 2012.05.12
 * @version 1.00
 */
public class Parser {
	
	private static ArrayList<Message> data = new ArrayList<Message>();
	
	private static ArrayList<Message> cfg = new ArrayList<Message>();
	
	private static ArrayList<Message> errors = new ArrayList<Message>();
	
	private static ArrayList<String> matrixIndex = new ArrayList<String>();
	
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
	
	private double time = 1.0/1024;
	private double vBase = 1.0/1024;
	
	
	/**
	 * This enum contains the different opCodes that are specified
	 * in the datalogger documentation.<br>
	 * ACL = 0 <br>
	 * BOVF = 1 <br>
	 * CM = 2 <br>
	 * COVF = 3 <br>
	 * CRD = 4 <br>
	 * PRM = 5 <br>
	 * PWM = 5 <br>
	 * MNT = 6 <br>
	 * PS = 7 <br>
	 * VS = 8 <br>
	 * DM = 9 <br>
	 * CT = 10 <br>
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
	
	public void parse(String data) {
		String [] sp = data.split(" ");
		int opCode = code.valueOf(sp[0]).num;
		Message temp;
		switch (opCode) {
		case 0:
			temp = new Accelerometer(sp, true);
			addToMatrix(temp);
			break;
		case 1:
			errors.add(new BOverflow(sp, true));
			break;
		case 2:
			temp = new CANMessage(sp, true);
			if (sp[2].equalsIgnoreCase("covf")) {
				errors.add(temp);
				break;
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
			temp = new dismountMessage(sp, true);
			addToMatrix(temp);
			break;
		case 10:
			errors.add(new CTMessage(sp, true));
			break;
		default:
			break;
			
		}
	}
	
	/**
	 * @param header : The header of the Message that
	 * the method is using to find the 
	 * @return The index at which the specified header exists.
	 * -1 if the header does not exist.
	 */
	private int findIndex(String header) {
		return matrixIndex.indexOf(header);
	}
	
	/**
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

	@Override
	public String toString() {
		String out = "";
		matrix.toString();
		return out;
	}
}