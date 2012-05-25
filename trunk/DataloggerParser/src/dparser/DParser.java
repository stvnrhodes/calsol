package dparser;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Scanner;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;
import javax.swing.filechooser.FileNameExtensionFilter;

import org.json.JSONException;
import au.com.bytecode.opencsv.CSVWriter;

/**
 * Parser written for the CalSol datalogger
 * @author Derek Chou
 * @since 2012.05.13
 * @version 1.00
 */

public class DParser {
	
	/**
	 * The parser object that this class uses to parse the datalogger
	 * lines.
	 */
	private static Parser p;
	
	/**
	 * Creates a Parser object, and also prompts the user for a .dla
	 * file location. Then it attempts to parse the file
	 * and then output. It repeats this process for as long as the user
	 * chooses a file and does not hit the "cancel" button.
	 * This method fails to execute if the Parser 
	 * object cannot locate where the JSON files for the CAN Message
	 * decoder are. It also fails if the parser cannot read the input 
	 * file.
	 * @param args : Main class; args is unused.
	 */
	public static void main(String[] args) {
		try {
			p = new Parser();
			p.setVerbose(true);
		} catch(FileNotFoundException e) {
			e.printStackTrace();
			return;
		} catch (JSONException e) {
			e.printStackTrace();
			return;
		}
		while (true) {
			JFileChooser fd = new JFileChooser(".");
			FileFilter filter = new FileNameExtensionFilter(
					"Datalogger ASCII file (*.DLA)", "dla");
			fd.addChoosableFileFilter(filter);
			int returnVal = fd.showOpenDialog(null);
			if(returnVal == JFileChooser.APPROVE_OPTION) {
			    File file=fd.getSelectedFile();
			    try {
					Scanner f = new Scanner(file);
					while (f.hasNext()) {
						p.parse(f.nextLine());
					}
					writeFiles(file);
				} catch (FileNotFoundException e) {
					/*Do nothing!*/
					e.printStackTrace();
				} catch (IOException e) {
					/*Do nothing!*/
					e.printStackTrace();
				}
			}
			else if (returnVal == JFileChooser.CANCEL_OPTION)
				break;
			else
				System.out.print("Error opening file!");
		}
	}

	/**
	 * Writes the parsed data into .csv and .txt files. The configuration,
	 * firmware, and datalogger version info will be in a .txt file named 
	 * "(dataloggerfilename)cfg.txt" This method makes use of opencsv for 
	 * csv writing.
	 * @param file : The original file that was opened for parsing - needed
	 * to get the name of the file.
	 */
	private static void writeFiles(File file) throws IOException {
		String fileName = file.getName();
		fileName = fileName.substring(0,fileName.indexOf('.')) + "p.csv";
		try {
			
			FileWriter f = new FileWriter(new File(fileName));
			CSVWriter wr = new CSVWriter(f);
			String [] out = {"Timestamp", "Message", "Data"};
			wr.writeNext(out);
			wr.writeAll(p.getStrings());
		} catch (FileNotFoundException e) {
			// Do nothing!
			e.printStackTrace();
		}
	}

}
