import datetime
import sqlite3
import traceback

from PySide import QtGui, QtCore
import pytz

def link(signal, slot):
    "Connects the signal callback to the slot notifier"
    signal.connect(lambda *args, **kwargs: slot(*args, **kwargs))

class DataDumperApplication(QtGui.QApplication):
    """\
    Implements a front-end interface to retrieve data from the sqlite
    database and export it as a csv file.
    """

    def show_exc(self, action, exception):
        formatted = traceback.format_exc()
        message = "An error occurred while %s:\n%s" % (action, formatted)

        self.error_handler.showMessage(message)
        
        with open("error.dump.py.log", "a+") as log:
            log.write(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S\n"))
            log.write(message + "\n")
        
    def show_error(self, message):
        self.error_handler.showMessage(message)

    def setup(self):
        """
        Setup the main application state.
        """

        #Do some setup here
        self.main_window = DataSelectorWindow()
        self.main_window.setup()

        self.error_handler = QtGui.QErrorMessage()

        self.lastWindowClosed.connect(self.quit)

        def trigger_dump():
            self.dump_data(self.main_window.db_filename.text(),
                           pytz.timezone(self.main_window.timezone.currentText()),
                           self.main_window.start.pyDateTime(),
                           self.main_window.end.pyDateTime(),
                           self.main_window.export_name.text(),
                           self.main_window.packet_id.text(),
                           not self.main_window.use_relative.isChecked(),
                           self.main_window.use_trim.isChecked(),
                           self.main_window.use_delete.isChecked())

        link(self.main_window.export_button.pressed, trigger_dump)

    def dump_data(self, db_name, tz, start, end, out_name, packet_id, use_timestamp=False, trim=True, delete_outliers=True):
        start = tz.localize(start).astimezone(pytz.utc)
        end = tz.localize(end).astimezone(pytz.utc)
            
        if not db_name:
            self.show_error("Please enter the path to the database file (.db)")
            return
        if not out_name:
            self.show_error("Please enter the path to the output csv file")
            return
        if not packet_id:
            self.show_error("Please enter the packet id you want to retrieve")
            return
        try:
            x = str(int(packet_id, 16))
        except ValueError:
            self.show_error("%s doesn't look like a packet id in hex" % packet_id)
            return
        
        try:
            db = sqlite3.connect(db_name, detect_types = (sqlite3.PARSE_DECLTYPES | sqlite3.PARSE_COLNAMES))
        except BaseException as e:
            self.show_exc("opening the database", e)
            return

        try:
            rows = list(db.execute("SELECT name, data, time FROM data WHERE id = ? AND time > ? AND time < ?",
                                   [int(packet_id, 16), start, end]))
            if len(rows) == 0:
                self.show_error("Didn't find any data")
                return
        except BaseException as e:
            self.show_exc("retrieving the data", e)
            db.close()
            return
        else:
            db.close()

        try:
            headers = set()
            skip = set()
            time_map = {}
            for (name, value, time) in rows:
                headers.add(name)
                if time in time_map:
                    if delete_outliers:
                        try:
                            temp = float(value)
                        except:
                            pass
                        else:
                            if not (-10000 < temp < 10000):
                                #print "Skipping bad value %s at %s" % (value, time)
                                skip.add(time)
                    time_map[time][name] = value
                else:
                    time_map[time] = {name: value}
        except BaseException as e:
            self.show_exc("aggregating data points", e)
            return

        try:
            out = open(out_name, "w+")
        except BaseException as e:
            self.show_exc("opening output file '%s'" % out_name, e)
            return
        
        try:
            key_order = sorted(headers)
            if trim:
                start = pytz.utc.localize(min(time_map.keys()))
            
            if use_timestamp:
                time_header = "time"
            else:
                time_header = "time (seconds since %s)" % start
            out.write(",".join([time_header] + key_order) + "\n")
            for time, values in sorted(time_map.items()):
                if time in skip:
                    continue
                dt = pytz.utc.localize(time)
                if use_timestamp:
                    out.write(dt.strftime("%Y-%m-%d %H:%M:%S"))
                else:
                    td = dt - start
                    out.write("%.6f" % (86400 * td.days + td.seconds + 1e-6*td.microseconds))
                for key in key_order:
                    out.write("," + str(values[key]))
                out.write("\n")
        except BaseException as e:
            out.close()
            self.show_exc("writing csv file", e)
            return
        out.close()
    def run(self):
        """
        Any code that needs to happen immediately before the program runs
        should go here. Things like making the main application window
        visible, starting side-computation threads, etc. At the end of the
        method, be sure to call and return the value of self.exec_() to tell
        PySide to start the main event loop for the entire application.
        """

        #Do something here

        self.main_window.show()

        return self.exec_()
    

class DataSelectorWindow(QtGui.QMainWindow):
    """
    """
    def setup(self):
        """
        """
        self.setWindowTitle("Data Extractor")

        #Create a central widget - just a widget to act as the main root
        #for everything else - this simplifies the layout.
        self.central_widget = QtGui.QWidget(self)

        #Setup the size policy so that the window can expand
        #QtSizePolicy.Preferred tells Qt that we want the widget (the window,
        #in this case) to be around this size when possible, but that it's
        #okay to make it expand or shrink. It appears once for each dimension.
        size_policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding,
                                        QtGui.QSizePolicy.Expanding)
        size_policy.setHorizontalStretch(1)
        size_policy.setVerticalStretch(1)
        self.central_widget.setSizePolicy(size_policy)
        self.central_widget.setMinimumSize(QtCore.QSize(400, 500))

        #Create a layout that fits your needs - probably a QFormLayout
        self.central_layout = QtGui.QFormLayout(self.central_widget)

        self.start = CustomDateTimeEdit()
        self.end = CustomDateTimeEdit()

        self.db_filename = QtGui.QLineEdit("test2.db", self)
        self.packet_id = QtGui.QLineEdit(self)        
        self.export_name = QtGui.QLineEdit(self)
        self.timezone = TimeZoneComboBox(self)

        self.use_relative = QtGui.QCheckBox(self)
        self.use_relative.setCheckState(QtCore.Qt.CheckState.Checked)
        self.use_trim = QtGui.QCheckBox(self)
        self.use_trim.setCheckState(QtCore.Qt.CheckState.Checked)
        self.use_delete = QtGui.QCheckBox(self)
        self.use_delete.setCheckState(QtCore.Qt.CheckState.Checked)
        

        self.export_button = QtGui.QPushButton("Export", self)
        
        self.central_layout.addRow("Computer Timezone", self.timezone)
        self.central_layout.addRow("Database Filename", self.db_filename)
        self.central_layout.addRow("Packet ID", self.packet_id)
        self.central_layout.addRow("Start Time", self.start)
        self.central_layout.addRow("End Time", self.end)

        self.central_layout.addRow("Use seconds since start", self.use_relative)
        self.central_layout.addRow("Trim start time to first data point", self.use_trim)
        self.central_layout.addRow("Omit outliers (+/-10000", self.use_delete)

        self.central_layout.addRow("Output Filename", self.export_name)

        self.central_layout.addRow(self.export_button)
        

        #Also important, tell the main widget to use the central_layout
        #to arrange its widgets and to implicitly act as a parent for
        #the widgets in the layout for event-handling purposes
        self.central_widget.setLayout(self.central_layout)

class CustomDateTimeEdit(QtGui.QDateTimeEdit):

    pyDateTimeChanged = QtCore.Signal([object])
    def __init__(self, date=None, time=None, parent=None):
        QtGui.QDateTimeEdit.__init__(self, parent)
        date = date if date is not None else QtCore.QDate.currentDate()
        self.setDate(date)
        self.setMinimumDate(QtCore.QDate(1993, 6, 20))
        self.setDisplayFormat("ddd MMM d yyyy h:mm:ss AP")

        self.setCalendarPopup(True)
        link(self.dateTimeChanged, self.emit_datetime)


    def pyDateTime(self):
        qdt = self.dateTime().toUTC()
        qdate, qtime = qdt.date(), qdt.time()

        dt = datetime.datetime(qdate.year(), qdate.month(), qdate.day(),
                               qtime.hour(), qtime.minute(), qtime.second())
        return dt

    def emit_datetime(self, qdt):
        qdate, qtime = qdt.date(), qdt.time()

        dt = datetime.datetime(qdate.year(), qdate.month(), qdate.day(),
                               qtime.hour(), qtime.minute(), qtime.second())
        self.pyDateTimeChanged.emit(dt)

    def setPyDateTime(self, dt):
        qdate = QtCore.QDate(dt.year, dt.month, dt.day)
        qtime = QtCore.QTime(dt.hour, dt.minute, dt.second)
        qdt = QtCore.QDateTime(qdate, qtime)

class TimeZoneComboBox(QtGui.QComboBox):
    def __init__(self, parent=None):
        QtGui.QComboBox.__init__(self, parent)
        self.addItems(pytz.common_timezones)
        self.setCurrentIndex(self.findText("US/Pacific"))
        

if __name__ == "__main__":
    import sys

    app = DataDumperApplication(sys.argv)
    app.setup()

    sys.exit(app.run())

##if __name__ == "__main__":
##    conn = sqlite3.connect("test2.db", detect_types = sqlite3.PARSE_DECLTYPES | sqlite3.PARSE_COLNAMES)
    
