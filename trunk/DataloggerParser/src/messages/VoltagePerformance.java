package messages;

import java.util.ArrayList;

/**
 * This is a "statistical performance measurement" as
 * documented by the Datalogger ASCII format specs.
 * It includes in its data ArrayList
 * a channel ID, the number of samples taken, min, avg, and max
 * values for the voltage measurements in the interval.
 * @author Derek Chou
 * @since 2012.05.15
 */
public class VoltagePerformance extends Message {

    public VoltagePerformance(String [] info, boolean ts) {
        super(info,ts);
        header.add("Statistical Performance Measurement");
        /* This is a little bit hard-coded, but then again,
         * this was the format given me.
         */
        header.add("Channel ID");
        header.add("Samples");
        header.add("Min");
        header.add("Avg");
        header.add("Max");
        data = new ArrayList<String>();
        for (int i = 2; i < info.length; i++)
            data.add(info[i]);
    }

}
