package dparser;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
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
 * @version 1.1
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
     * @param args Main class; args is unused.
     */
    public static void main(String[] args) {
        try {
            p = new Parser();
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
            if (returnVal == JFileChooser.APPROVE_OPTION) {
                File file = fd.getSelectedFile();
                try {
                    Scanner f = new Scanner(file);
                    while (f.hasNext()) {
                        ArrayList<String> line = p.parse(f.nextLine());
                        write(line);
                    }
                    f.close();
                } catch (FileNotFoundException e) {
                    /*Do nothing!*/
                    e.printStackTrace();
                } catch (IOException e) {
                    /*Do nothing!*/
                    e.printStackTrace();
                }
                p.clearFields();
            }
            else if (returnVal == JFileChooser.CANCEL_OPTION)
                break;
            else
                System.out.print("Error opening file!");
        }
    }

    /**
     * Writes the parsed data into .csv and .txt files.
     * Current output is <br>
     * <b>(filename)rawdata.csv</b>, which includes all non-error data <br>
     * <b>(filename)errors.csv</b>, which includes all error data <br>
     * <b>(filename)toMat.txt</b>, which includes all data to be converted
     * later into .mat form. This may be changed in the future, so that this
     * parser program may generate the .mat file in one shot without having
     * to use this intermediate. <br>
     * <b>debug.txt</b> If the parser is set to verbose mode. This includes
     * every message sent, as a dump into a text file.
     * @param file The original file that was opened for parsing - needed
     * to get the name of the file.
     */
    private static void write(ArrayList<String> input) throws IOException {
        String fileName = file.getName().substring
                (0,file.getName().indexOf('.'))+ "rawdata.csv";
        try {

            FileWriter f = new FileWriter(new File(fileName));
            CSVWriter wr = new CSVWriter(f);
            String [] data = {"Timestamp", "Message", "Data"};
            wr.writeNext(data);
            wr.writeAll(p.getStrings());
            f.close();
            wr.close();

            fileName = file.getName().substring(0,file.getName().indexOf('.'))
                    + "errors.csv";
            f = new FileWriter(new File(fileName));
            wr = new CSVWriter(f);
            String [] errors = {"Timestamp", "Error Type", "Data"};
            wr.writeNext(errors);
            wr.writeAll(p.getErrorStrings());
            f.close();
            wr.close();

            fileName = file.getName().substring(0,file.getName().indexOf('.'))
                    + "toMat.txt";
            PrintStream pr = new PrintStream(new File(fileName));
            ArrayList<String> mat = p.getMatStrings();
            for(int i = 0; i < mat.size(); i++)
                pr.println(mat.get(i));
            pr.close();

            fileName = file.getName().substring(0,file.getName().indexOf('.'))
                    + "MotorTeam.txt";
            pr = new PrintStream(new File(fileName));
            ArrayList<String> motorTeam = p.getMotorStrings();
            for(int i = 0; i < motorTeam.size(); i++)
                pr.println(motorTeam.get(i));
            pr.close();

        } catch (FileNotFoundException e) {
            // Do nothing!
            e.printStackTrace();
        }
    }

}
