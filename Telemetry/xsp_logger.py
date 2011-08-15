from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SocketServer import ThreadingMixIn

import datetime
import glob
import json
import logging
import os
from urllib import unquote
import urlparse
import re
import serial
import sqlite3
import traceback

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

from odict import OrderedDict

import xsp

def tableExists(conn, name):
    cur = conn.cursor()
    cur.execute('SELECT name FROM sqlite_master where name = ?;', (name,))
    exists = cur.fetchone() != None
    cur.close()
    return exists

class XSPLogger(object):
    DB_ERROR_LIMIT = 5
    SERIAL_ERROR_LIMIT = 5
    def __init__(self):
        self.serial_port = None
        self.db_connection = None

        self.can_descriptors = {}

        self.handler = xsp.XSPHandler(self.lookup_format, self.lookup_descr)

        self.serial_error_count = 0
        self.db_error_count = 0
        self.do_shutdown = False

    def lookup_format(self, id_):
        return "<" + str(self.can_descriptors["%#x" % id_].get("format"))

    def lookup_descr(self, id_):
        descr = self.can_descriptors["%#x" % id_].copy()
        if "messages" in descr:
            descr["messages"] = [d[0] for d in descr["messages"]]
        return descr

    def connect_serial(self, config):
        if self.serial_port is not None:
            self.disconnect_serial()
        
        try:
            self.serial_port = serial.Serial(**config)
        except ValueError:
            logging.exception("Unable to connect to serial port - invalid port configuration %s", config)
            return False
        except serial.SerialException:
            logging.exception("Unable to connect to serial port - port cannot be opened or configured as specified: %s", config)
            return False
        except:
            logging.exception("Unable to connect to serial port due to general error")
            return False
        
        logging.info("Connected to serial port %r", self.serial_port)
        return True

    def disconnect_serial(self):
        if self.serial_port is not None:
            self.serial_port.close()
            logging.info("Disconnected from serial port %r", self.serial_port)
            self.serial_port = None
            self.serial_error_count = 0
        else:
            logging.warning("Tried to disconnect serial port, but no serial port was connected")

    def connect_db(self, config):
        if self.db_connection is not None:
            self.disconnect_db()

        try:
            parse_everything = (sqlite3.PARSE_DECLTYPES | sqlite3.PARSE_COLNAMES)
            self.db_connection = sqlite3.connect(config["database"],
                                                 detect_types = parse_everything)
        except KeyError:
            logging.error("Unable to connect to database - path to database missing from config %r", config)
            return False
        except sqlite3.Error:
            logging.exception("Unable to connect to database '%s' due to a database error", config["database"])
            return False
        except:
            logging.exception("Unable to connect to database '%s' due to a general error", config["database"])
            return False

        logging.info("Connected to database %s", config["database"])
        return True

    def disconnect_db(self):
        if self.db_connection is not None:
            logging.info("Committing data before closing database connection %r", self.db_connection)
            self.db_connection.commit()
            logging.info("Closing database connection %r", self.db_connection)
            self.db_connection.close()
            logging.info("Closed database connection")
            self.db_connection = None
            self.db_error_count = 0
        else:
            logging.warning("Tried to disconnect database connection but no database was connected")

    def poll_serial(self, num_bytes=4096):
        if self.serial_port is None:
            logging.warning("Can't poll serial port - no serial port connected")
            return
        elif self.db_connection is None:
            logging.warning("Can't store data - no database connected")
            return

        try:
            messages = self.handler.poll(num_bytes)
        except xsp.ConnectionProblem:
            self.serial_error_count += 1
            logging.error("Connection problem with serial port %r detected. Occurance %d/%d",
                          self.serial_port, self.serial_error_count, self.SERIAL_ERROR_LIMIT)
            if self.serial_error_count >= self.SERIAL_ERROR_LIMIT:
                self.handler.unbind()
                logging.critical("Serial port error limit exceeded. Disconnecting.", self.serial_port)
                self.disconnect_serial()
            return
        
        
        for (time, id_, name, datum) in messages:
            try:
                self.log_can_message(time, id_, name, datum)
            except xsp.ConnectionProblem:
                self.db_error_count += 1
                logging.exception("Connection problem with database detected. Occurance %d/%d",
                                  self.db_error_count, self.DB_ERROR_LIMIT)
                if self.db_error_count >= self.DB_ERROR_LIMIT:
                    logging.critical("Database error limit exceeded. Disconnecting.")
                    self.disconnect_db()
                    return

    def log_can_message(self, time, id_, name, datum):
        #print "Recording packet %#x at %s" % (id_, time)
        if self.db_connection is None:
            logging.warning("Trying to access database without an active connection")
            return
        else:
            try:
                self.db_connection.execute("INSERT INTO messages (id, name, time, data) VALUES (?,?,?,?)",
                                           [id_, name, time, json.dumps(datum, ensure_ascii=False)])
            except:
                logging.exception("Exception occurred while inserting CAN message record")                
                raise xsp.ConnectionProblem("Error while inserting CAN message records")

    def load_can_descriptors(self, config):
        if self.can_descriptors:
            logging.info("Replacing current CAN descriptors")
        else:
            logging.info("Loading CAN descriptors")
            self.can_descriptors = {}
            fnames = glob.glob(os.path.join("config", "*.can.json"))
            if not fnames:
                logging.warning("Warning, no CAN message description files found!")
            for fname in fnames:
                try:
                    f = open(fname, "r")
                except IOError:
                    logging.exception("Unable to open can descriptor file '%s'", fname)
                    continue
                try:
                    json_ = json.load(f)
                    for key in json_:
                        try:
                            normalized = "%#x" % int(key, 16)
                        except ValueError:
                            logging.error("Bad packet id: %s", key)
                        self.can_descriptors[normalized] = json_[key]
                except:
                    logging.exception("Error while parsing can descriptor file '%s'", fname)
                finally:
                    f.close()

    def run(self):
        while not self.do_shutdown:
            try:
                self.poll_serial()
            except KeyboardInterrupt:
                self.do_shutdown = True

    def shutdown(self):
        self.handler.unbind()
        self.disconnect_serial()
        self.disconnect_db()

    def config_database(self, conn, drop_tables=False):
        "Sets up the database with the intervals and data tables"
        cursor = conn.cursor()
        if tableExists(conn, "intervals") and drop_tables:
            cursor.execute("DROP TABLE intervals;")
        if not tableExists(conn, "intervals"):
            cursor.execute("CREATE TABLE intervals (name text, start timestamp, end timestamp);")

        if tableExists(conn, "messages") and drop_tables:
            cursor.execute("DROP TABLE ,essages;")
        if not tableExists(conn, "messages"):
            cursor.execute("CREATE TABLE messages (id integer, name text, time timestamp, data text);")
        conn.commit()
        cursor.close()

    def setup(self):
        self.load_can_descriptors({})
        if self.connect_db({'database': 'telemetry.db'}):
            self.config_database(self.db_connection)
            
        if self.connect_serial({'port': 'COM10', 'baudrate': 115200, 'timeout': 0}):
            self.handler.bind(self.serial_port)
if __name__ == "__main__":
    logging.basicConfig(filename="xsp.log")
    xsp_logger = XSPLogger()
    xsp_logger.setup()
    print "Starting up"
    xsp_logger.run()
    print "Exiting"
