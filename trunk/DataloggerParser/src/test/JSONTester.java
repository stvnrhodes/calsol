package test;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Scanner;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * JSON decoder tester.
 * @author Derek Chou
 * @since 2012.05.20
 */
public class JSONTester {

	private static ArrayList<JSONObject> decoder =
			new ArrayList<JSONObject>();
	
	/**
	 * @param args : Main class String [] args thing.
	 * @throws FileNotFoundException 
	 * @throws JSONException 
	 */
	public static void main(String[] args) throws FileNotFoundException, JSONException {
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
				f += r.nextLine();
			}
			JSONObject in = new JSONObject(f);
			System.out.println(in);
			decoder.add(in);
			System.out.println(in.has("0x501"));
			try {
				JSONObject a = in.getJSONObject("0x501");
				System.out.println(a);
				JSONArray b = a.getJSONArray("messages");
				System.out.println(b);
				JSONArray c = b.getJSONArray(0);
				JSONArray d = b.getJSONArray(1);
				for (int j = 0; j < c.length(); j++) {
					System.out.println(c.getString(j));
					System.out.println(d.getString(j));
				}
				String e = a.getString("name");
				System.out.println(e);
			} catch (JSONException e) {
				/* Do nothing! */
			}
		}
	}

}
