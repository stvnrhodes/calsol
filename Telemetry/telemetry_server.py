from BaseHTTPServer import HTTPServer
from SocketServer import ThreadingMixIn

from httplite import *

import datetime
import glob
import json
import logging
import os
import re
import sqlite3
from textwrap import dedent

from websocket import WebSocket, SingleOptionPolicy, Message
from postprocessing import *

def normalize(id_):
    "Normalize a packet id in hex so that capitalization is consistent"
    return "%#x" % int(id_, 16)

class TelemetryServer(HTTPServer, ThreadingMixIn):
    def __init__(self, server_address, handler_class):
        HTTPServer.__init__(self, server_address, handler_class)
        self.can_descriptors = {}
        self.descriptor_sets = {}
        
        self.shutdown = False

    def load_can_descriptors(self, config):
        if self.descriptor_sets:
            logging.info("Replacing current CAN descriptors")
        else:
            logging.info("Loading CAN descriptors")
            self.descriptor_sets = {}
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
                    head, tail = os.path.split(fname)
                    set_name = tail[:-9]
                    descriptors = {}
                    json_ = json.load(f)
                    for key in json_:
                        try:
                            normalized = "%#x" % int(key, 16)
                        except ValueError:
                            logging.error("Bad packet id: %s", key)
                        descriptors[normalized] = json_[key]
                        self.can_descriptors[normalized] = json_[key]
                    self.descriptor_sets[set_name] = descriptors
                except:
                    logging.exception("Error while parsing can descriptor file '%s'", fname)
                finally:
                    f.close()

    def setup(self):
        self.load_can_descriptors({})

delta_units = ['weeks', 'days', 'hours', 'mins', 'secs']
delta_pattern = ''.join((r'(?:(?P<%s>0|[1-9]\d*)%s(?:%ss?)?)?' %
                         (unit, unit[0], unit[1:-1])) for unit in delta_units)
delta_regex = re.compile(delta_pattern, re.I)
def parse_timestamp(ts):
    if not ts:
        raise ValueError("No timestamp found")
    elif ts.startswith('-'):
        now = datetime.datetime.utcnow()
        match = delta_regex.match(ts[1:])
        if not match or not any(match.groups()):
            raise ValueError("Timedelta doesn't adhere to standard format")
        weeks = int(match.group('weeks')) if match.group('weeks') else 0
        days = int(match.group('days')) if match.group('days') else 0
        hours = int(match.group('hours')) if match.group('hours') else 0
        mins = int(match.group('mins')) if match.group('mins') else 0
        secs = int(match.group('secs')) if match.group('secs') else 0
        delta = datetime.timedelta(days = 7*weeks + days,
                                   seconds = 3600*hours + 60*mins + secs)
        return now - delta
    else:
        try:
            return datetime.datetime.strptime(ts, "%Y%m%d%H%M%S")
        except ValueError:
            raise ValueError("Invalid timestamp %s doesn't match compact ISO format" % ts)

def parse_timespan(r):
    if not r:
        raise ValueError("No time span specified")
    if "_" not in r:
        raise ValueError("Invalid time span '%s' doesn't match format [start]_[end]" % r)
    start, end = r.split("_", 1)
    
    start_ts = parse_timestamp(start) if start else None
    end_ts = parse_timestamp(end) if end else None

    return (start_ts, end_ts)

class TelemetryHTTPHandler(LiteHTTPHandler):
    def setup(self):
        LiteHTTPHandler.setup(self)
        self.db_connection = None
        try:
            parse_everything = (sqlite3.PARSE_DECLTYPES | sqlite3.PARSE_COLNAMES)
            self.db_connection  = sqlite3.connect(config["database"],
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

    def finish(self):
        LiteHTTPHandler.finish(self)
        if self.db_connection:
            self.db_connection.close()
        self.db_connection = None


    def load_can_messages(self, id_, name, lower=None, upper=None):
        try:
            descr = self.server.can_descriptors[id_]
        except KeyError:
            logging.error("Loading data from database failed - no descriptor found for packet with id %s", id_)
            return []

        if self.db_connection is None:
            logging.warning("Trying to access database without an active connection")
            return []
        if not lower and not upper:
            logging.warning("Trying to execute unbounded query - returning empty list instead")
            return []
        base = "SELECT time, data FROM data WHERE id = ? AND name = ? AND "
        try:
            if lower is not None and upper is None:
                where = "time >= ?"
                rows = self.db_connection.execute(base + where, [int(id_, 16), name, lower])
            elif lower is None and upper is not None:
                where = "time <= ?"
                rows = self.db_connection.execute(base + where, [int(id_, 16), name, upper])
            else:
                where = "time >= ? AND time <= ?"
                rows = self.db_connection.execute(base + where, [int(id_, 16), name, lower, upper])
        except:
            logging.exception("Error occurred while loading data from database")
            return []
        else:
            data = {}
            for row in rows:
                ts, datum = row[0], row[1]
                data[ts] = json.loads(datum)
                #data[ts.strftime("%Y-%m-%d %H:%M:%S.%f")] = json.loads(datum)
            return data
        
    def map_handler(self, url):
        if url.path.startswith("/descr/"):
            if url.depth == 1:
                return self.handle_descr_index
            else:
                return self.handle_descr
        elif url.path.startswith("/meta/"):
            if url.depth == 1:
                return self.handle_meta_index
            else:
                return self.handle_meta
        elif url.path.startswith("/data/"):
            if url.depth == 1:
                return self.handle_data_index
            else:
                return self.handle_data
        else:
            return self.handle_static
    
    @allow('GET')
    def handle_descr_index(self, request):
        resp = HTTPResponse(200, content_type="text/html")
        resp.body.write(dedent("""\
        <html>
            <head>
                <title>CAN Node Documentation</title>
            </head>
            <body>
                <p>The following is a list of links to metadata about the format of
                   <acronym title="Controller Area Network">CAN</acronym> packets
                   sent by each CAN node on Impulse:
                </p>
                <ul>"""))
        
        for node in sorted(self.server.descriptor_sets.keys()):
            resp.body.write(8*' ' + '<li><a href="%s/">%s</li>\n' % (node, node))

        resp.body.write(dedent("""\
                </ul>
            </body>
        </html>"""))
        return resp

    @allow('GET')
    def handle_static(self, request):
        segments = [segment for segment in request.url.segments if not segment.startswith(".")]
        if request.url.is_index:
            segments.append("index.html")
        path = os.path.join("html", *segments)
        if not path.startswith("html"):
            return self.handle_404(request)

        if not os.path.exists(path):
            return self.handle_404(request)
        
        return StaticHTTPResponse(path)

    @allow('GET')#, 'HEAD', 'PUT', 'DELETE')
    def handle_descr(self, request):
        if request.url.depth != 2:
            return self.handle_404(request)
        group = request.url.segments[1]
        if group in self.server.descriptor_sets:
            descr = self.server.descriptor_sets[group]
        elif group in self.server.can_descriptors:
            descr = self.server.can_descriptors[group]
        else:
            return self.handle_404(request)

        return HTTPResponse(200,
                            content_type="text/plain",
                            body=json.dumps(descr, indent=4))
    
    @allow('GET')
    def handle_data_index(self, request):
        return StaticHTTPResponse("html/historical-viewer.html")
        #return StaticHTTPResponse("html/highcharts-test.html")

    @allow('GET')
    def handle_data(self, request):
        if request.url.depth != 2:
            return self.handle_404(request)

        if request.url.segments[1] not in ("history", "since"):
            return self.handle_404(request)

        if request.url.segments[1] == "history":
            print request.headers
            if request.headers.get("Upgrade") == "websocket":
                print "Got websocket upgrade request"
                return self.serve_data_websocket(request)
            
            if not request.query.get("m"):
                return HTTPResponse(500,
                                    content_type="text/plain",
                                    body="No messages were specified")
            
            try:
                start, end = parse_timespan(request.query_first("ts"))
            except ValueError as e:
                return HTTPResponse(500, content_type="text/plain",
                                    body=str(e))
            
            if not (start or end):
                return HTTPResponse(500, content_type="text/plain",
                                    body="No timespan specified")
            
            if request.headers.get("Upgrade") == "websocket":
                print "Got websocket upgrade request"
                return self.serve_data_websocket(request)

            
            messages = request.query["m"]
            idents = []
            for msg in messages:
                try:
                    id_, name = msg.split(":")
                    id_ = normalize(id_)
                except ValueError as e:
                    return HTTPResponse(500, content_type="text/plain",
                                        body=str(e))
                if id_ not in self.server.can_descriptors:
                    return HTTPResponse(500, content_type="text/plain",
                                        body=("Unknown packet %s" % id_))
                descr = self.server.can_descriptors[id_]
                if name not in [msg[0] for msg in descr["messages"]]:
                    return HTTPResponse(500, content_type="text/plain",
                                        body=("Unknown message '%s' in packet %s" % (name, id_)))
                idents.append((id_, name))
                series = []
                for (id_, name) in idents:
                    data = self.load_can_messages(id_, name, start, end)
                    polyline = list(transform_to_screen(simplify_by_extrema(sorted(data.items())),
                                                        (start, end), (0.0, 140.0), 1600.0, 400.0))
                    simplified = simplify_ramer_douglas_peucker(polyline, 2.0)
                    simple_data = list(transform_to_sample(simplified,
                                                      (start, end), (0.0, 140.0), 1600.0, 400.0))
                    series.append([(dt.strftime("%Y-%m-%d %H:%M:%S.%f"), value) for (dt, value) in simple_data])
                if not any(series):
                    resource = {
                        "data": None,
                        "tmin": None,
                        "tmax": None
                    }
                else:
                    resource = {
                        "data": series,
                        "tmin": min(map(min, series))[0],
                        "tmax": max(map(max, series))[0]
                    }

            return HTTPResponse(200, content_type="application/json",
                                body=json.dumps(resource))

    def serve_data_websocket(self, request):
        
        policy_map = {
            "version": SingleOptionPolicy("8"),
            "protocol": SingleOptionPolicy("telemetry.calsol.berkeley.edu")
        }
        try:
            WebSocket.validate(request, policy_map)
        except ValueError as e:
            print e
            return HTTPResponse(400, content_type="text/plain",
                               body=str(e))
        websocket = WebSocket(request, self)
        websocket.negotiate(policy_map)

        while True:
            message = websocket.read_message()
            print "Received:", message.payload_string
            s = "Hello, client!"
            import StringIO
            reply = Message.make_text(StringIO.StringIO(s), len(s))
            websocket.send_message(reply)

if __name__ == "__main__":
    logging.basicConfig(filename="server.log")
    config = {"database": "test2.db"}
    httpd = TelemetryServer(('', 8000), TelemetryHTTPHandler)
    httpd.setup()
    print "Starting up"
    httpd.serve_forever()
