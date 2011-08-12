from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler

import logging
import serial
import sqlite3

class TelemetryServer(HTTPServer):
    DB_ERROR_LIMIT = 5
    SERIAL_ERROR_LIMIT = 5
    def __init__(self, server_address, handler_class):
        HTTPServer.__init__(self, server_address, handler_class)
        self.serial_port = None
        self.db_connection = None
        self.handler = xsp.XSPHandler(self.lookup_format, self.lookup_descr)

        self.serial_error_count = 0
        self.db_error_count = 0

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
            self.db_connection = sqlite3.connect(config["database"])
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
        except ConnectionProblem:
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
            except ConnectionProblem:
                self.db_error_count += 1
                logging.exception("Connection problem with database detected. Occurance %d/%d",
                                  self.db_error_count, self.DB_ERROR_LIMIT)
                if self.db_error_count >= self.DB_ERROR_LIMIT:
                    logging.critical("Database error limit exceeded. Disconnecting.")
                    self.disconnect_db()
                    return

    def log_can_message(self, time, id_, name, datum):
        if self.db_connection is None:
            logging.warning("Trying to access database without an active connection")
            return
        else:
            try:
                self.db_connection.execute("INSERT INTO messages (id, name, time, datum) VALUES (?,?,?,?)",
                                           [id_, name, time, json.dumps(datum, ensure_ascii=False)])
            except:
                raise ConnectionProblem("Error while inserting CAN message records")
                
                

    def load_can_descriptors(self, config):
        if self.can_descriptors:
            logging.info("Replacing current CAN descriptors")
        else:
            logging.info("Loading CAN descriptors")

class TelemetryHTTPHandler(BaseHTTPRequestHandler):
    pass

if __name__ == "__main__":
    logging.basicConfig(filename="server.log")
