package test;

import java.io.File;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;
import javax.swing.filechooser.FileNameExtensionFilter;

/**
 * GUI File selector tester.
 * @author Derek Chou
 * @since 2012.05.20
 */
public class FileTester {

	/**
	 * @param args : Main class String [] args thing.
	 */
	public static void main(String[] args) {
		JFileChooser fd = new JFileChooser(".");
		FileFilter filter = new FileNameExtensionFilter(
				"JSON file (*.json)", "json");
		fd.addChoosableFileFilter(filter);
		int returnVal = fd.showOpenDialog(null);
		if(returnVal == JFileChooser.APPROVE_OPTION) {
		    File file=fd.getSelectedFile();
		    System.out.println(file.getAbsolutePath());
		}
		File newF = new File("CFG\\batteries.can.json");
		System.out.println(newF.getAbsolutePath());
	}

}
