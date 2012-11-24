package dparser;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Scanner;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;
import javax.swing.filechooser.FileNameExtensionFilter;

import messages.Message;

import org.json.JSONException;

import exceptions.PackException;

/**
 * Parser written for the CalSol datalogger
 * @author Derek Chou
 * @since 2012.05.13
 * @version 1.20
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
                    decode(file);
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
     * <b>(filename)rawdata.txt</b>, which includes all non-error data <br>
     * <b>(filename)errors.txt</b>, which includes all error data <br>
     * <b>debug.txt</b> If the parser is set to verbose mode. This includes
     * every message sent, as a dump into a text file.
     * @param file The original file that was opened for parsing - needed
     * to get the name of the file.
     */
    private static void decode(File file) throws IOException {
        String fileName = file.getName().substring
                (0,file.getName().indexOf('.'))+ "rawdata.txt";
        try {
            Scanner f = new Scanner(file);
            FileWriter fr = new FileWriter(new File(fileName));
            BufferedWriter output = new BufferedWriter(fr);
            int i = 0;
            while (f.hasNext()) {
                i++;
                Message m = p.parse(f.nextLine());
                if (i >= (Integer.MAX_VALUE >>> 10)) {
                    System.gc();
                    i = 0;
                }
                if (m != null) {
                    String [] temp = process(m);
                    if (temp != null) {
                        for (String s : temp) {
                            if (s != null) {
                                output.write(s);
                                output.write("\r\n");
                            }
                        }
                    }
                }
            }
            f.close();
            output.close();
        } catch (FileNotFoundException e) {
            // Do nothing!
            e.printStackTrace();
        }
    }


    /**
     * @param m The message that we're trying to process data from.
     * @return All data going to whomever wishes to process
     * the Datalogger output in MATLAB.
     */
    public static String[] process(Message m) {
        if (!m.hasData)
            return null;
        ArrayList<String> temp;
        try {
            temp = m.pack();
        } catch (PackException e) {
            return null;
        }
        String[] out = new String[temp.size()];
        for (int j = 0; j < temp.size(); j++) {
            out[j] = temp.get(j);
        }
        return out;
    }
}
