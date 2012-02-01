import datetime
import glob
import json
import os
import sqlite3

import matplotlib.units as units
import matplotlib.dates as dates
import pylab
import pytz

from postprocessing import *

def is_sequence(value):
    return hasattr(value, "__iter__") and (not isinstance(value, basestring))

class DatetimeConverter(units.ConversionInterface):
    @staticmethod
    def convert(value, unit, axis):
        return dates.date2num(value)

    @staticmethod
    def axisinfo(unit, axis):
        if unit != 'datetime':
            return None
        #Todo: pick a good formatter/locator for the axis
        majloc = dates.AutoDateLocator()
        majfmt = dates.AutoDateFormatter(majloc)
        return units.AxisInfo(majloc=majloc,
                              majfmt=majfmt,
                              label='datetime')

    @staticmethod
    def default_units(x, axis):
        return 'datetime'
units.registry[datetime.datetime] = DatetimeConverter()

def load_messages(packet_id, message_name, lower=None, upper=None):
    base = "SELECT time, data FROM data WHERE id = ? AND name = ? AND "
    params = [packet_id, message_name]
    if lower is not None and upper is None:
        where = "time >= ?"
        params.append(to_utc(lower))
    elif lower is None and upper is not None:
        where = "time <= ?"
        params.append(to_utc(upper))
    else:
        where = "time >= ? AND time <= ?"
        params.extend([to_utc(lower), to_utc(upper)])
    rows = db_connection.execute(base + where, params)
    return [(ts, json.loads(data)) for (ts, data) in rows]

def to_utc(dt):
    if dt.tzname() != None:
        return dt.astimezone(pytz.utc).replace(tzinfo=None)
    else:
        return dt

if __name__ == "__main__":
    DB_PATH = "test2.db"
    parse_everything = (sqlite3.PARSE_DECLTYPES | sqlite3.PARSE_COLNAMES)
    print "opening db"
    db_connection  = sqlite3.connect(DB_PATH, detect_types = parse_everything)
    print "opened db"

    wsc_race_tz = pytz.timezone("Australia/Darwin")
    race_start = wsc_race_tz.localize(datetime.datetime(2011, 10, 17, 8))
    day = datetime.timedelta(days=1)

    start = race_start+1*day
    end = race_start+datetime.timedelta(days=1, hours=12)

    import time
    now = time.time()
    print "loading data"
    samples = load_messages(0x402, "Bus Voltage", start, end)
    print "loaded data",
    print "took %s seconds to load %d data points" % (time.time() - now, len(samples))

    now = time.time()
    print "simplifying via vertical line test"
    samples = list(simplify_by_extrema(samples))
    print "simplified to %d data points in %s seconds" % (len(samples), time.time() - now)
    

    now = time.time()
    print "simplifying via ramer-douglas-peucker"
    trans = transform_to_screen(samples, [to_utc(start), to_utc(end)], [40.0, 140.0], 600, 600)
    trans_simple = simplify_ramer_douglas_peucker(list(trans), 2.0)
    samples = list(transform_to_sample(trans_simple, [to_utc(start), to_utc(end)], [40.0, 140.0], 600, 600))

    print "simplified to %d data points in %s seconds" % (len(samples), time.time() - now)
    times =[x[0] for x in samples]
    voltages = [x[1] for x in samples]

    pylab.autoscale(False)
    pylab.plot(times, voltages)
    pylab.axis([start, end, 40.0, 140.0])
    pylab.show()

