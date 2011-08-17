from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
#from SocketServer import ThreadingMixIn

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

class TelemetryServer(HTTPServer):
    DB_ERROR_LIMIT = 5
    def __init__(self, server_address, handler_class):
        HTTPServer.__init__(self, server_address, handler_class)
        self.timeout = 0

        self.db_connection = None
        self.can_descriptors = {}
        self.descriptor_sets = {}
        
        self.db_error_count = 0

        self.shutdown = False

    def connect_db(self, config):
        if self.db_connection is not None:
            self.disconnect_db()

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
    
    def load_can_messages(self, id_, name, lower=None, upper=None):
        try:
            descr = self.can_descriptors[id_]
        except KeyError:
            logging.error("Loading data from database failed - no descriptor found for packet with id %s", id_)
            return []

        if self.db_connection is None:
            logging.warning("Trying to access database without an active connection")
            return []
        if not lower and not upper:
            logging.warning("Trying to execute unbounded query - returning empty list instead")
            return []
        base = "SELECT time, data FROM messages WHERE id = ? AND name = ? AND "
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
            messages = []
            for row in rows:
                ts, datum = row[0], row[1]
                messages.append((ts.strftime("%Y-%m-%d %H:%M:%S.%f"), json.loads(datum)))
            return messages

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
        if not self.connect_db({'database': 'telemetry.db'}):
            logging.critical("Failed to connect to database")
        
class HTTPRequest(object):
    def __init__(self, url, command, headers, body):
        self.url = url
        self.command = command
        self.headers = headers
        self.body = body
        self._query = None
        self._query_list = None

    @property
    def query(self):
        if self._query is None:
            self._query = urlparse.parse_qs(self.url.query)
        return self._query

    @property
    def query_list(self):
        if self._query_list is None:
            self._query_list = urlparse.parse_qsl(self.url.query, keep_blank_values=True)
        return self._query_list
    
    @property
    def fragment(self):
        return self.url.fragment

class HTTPResponse(object):
    def __init__(self, status=200, headers=None, body=None, content_type=None):
        self.status = status
        self.headers = headers if headers is not None else OrderedDict()
        if content_type is not None:
            self.headers["Content-Type"] = content_type
            
        if hasattr(body, 'getvalue'):
            self.body = body
        elif isinstance(body, basestring):
            self.body = StringIO(body)
        else:
            self.body = StringIO()

class StaticHTTPResponse(HTTPResponse):
    def __init__(self, path, content_type=None):
        status = 200
        body = None
        try:
            f = open(path, 'r')
            body = f.read()
        except IOError:
            logging.exception("Unable to open static content")
            status = 500
        else:
            f.close()

        if not content_type:
            if path.endswith(".js"):
                content_type = "text/javascript"
            elif path.endswith(".css"):
                content_type = "text/css"
            elif path.endswith(".txt"):
                content_type = "text/plain"
            elif path.endswith(".jpg") or path.endswith(".jpeg"):
                content_type = "image/jpeg"
            elif path.endswith(".png"):
                content_type = "image/png"
            elif path.endswith(".ico"):
                content_type = "image/x-icon"
        HTTPResponse.__init__(self, status, {}, body, content_type)
        if body is not None:
            self.headers['Content-Length'] = len(body)
        
def allow(*commands):
    allowed = frozenset(commands)
    def decorator(handler):        
        def wrapped_handler(self, request):
            if request.command not in allowed:
                resp = HTTPResponse(405, headers={"Allow": ', '.join(allowed)})
                return resp
            else:
                return handler(self, request)
        return wrapped_handler
    return decorator

delta_units = ['weeks', 'days', 'hours', 'mins', 'secs']
delta_pattern = ''.join((r'(?:(?P<%s>0|[1-9]\d*)%s(?:%ss?)?)?' %
                         (unit, unit[0], unit[1:-1])) for unit in delta_units)
delta_regex = re.compile(delta_pattern, re.I)
def parse_timestamp(ts):
    if ts.startswith('-'):
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
            raise ValueError("Invalid timestamp doesn't match compact ISO format")

class TelemetryHTTPHandler(BaseHTTPRequestHandler):
    def _respond(self, handler, request, disable_500=False):
        try:
            response = handler(request)
            content = response.body.getvalue()
            self.send_response(response.status)
            for header, value in response.headers.items():
                self.send_header(header, value)
            if "Content-Length" not in response.headers:
                self.send_header("Content-Length", len(content))
            self.end_headers()
            self.wfile.write(content)
            response.body.close()
        except:
            if not disable_500:
                self._respond(self.handle_500, request, disable_500=True)
    
    def do(self, action):
        url = urlparse.urlparse(self.path, 'http')
        url.is_index = url.path.endswith('/')
        url.depth = url.path.rstrip('/').count('/')
        url.segments = url.path.strip('/').split('/')
        request = HTTPRequest(url, action, self.headers, self.rfile)
        
        handler = self.map_handler(url)
        if handler is None:
            self._respond(self.handle_404, request)
        else:
            self._respond(handler, request)

    def do_GET(self):
        self.do('GET')

    def do_POST(self):
        self.do('POST')

    def do_PUT(self):
        self.do('PUT')

    def do_HEAD(self):
        self.do('HEAD')

    def do_DELETE(self):
        self.do('DELETE')

    def handle_404(self, request):
        resp = HTTPResponse(404, content_type="text/html")
        resp.body.write("<html>\n")
        resp.body.write("    <head><title>Page not found</title></head>\n")
        resp.body.write("    <body><p>The url you attempted to access, <code>%s</code>, could not be found.</p></body>\n" % request.url.path)
        resp.body.write("</html>\n")

        return resp

    def handle_500(self, request):
        resp = HTTPResponse(500, content_type="text/html")
        traceback.print_exc()
        resp.body.write("<html>\n")
        resp.body.write("    <head><title>Server Error</title></head>\n")
        resp.body.write("    <body>\n")
        resp.body.write("        <p>While rendering the resource at <code>%s</code> an error occurred:</p>\n" % request.url.path)
        resp.body.write("        <pre>%s</pre>\n" % traceback.format_exc())
        resp.body.write("    </body>\n")
        resp.body.write("</html>\n")

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

    @allow('GET')
    def handle_descr_index(self, request):
        resp = HTTPResponse(200, content_type="text/html")
        resp.body.write("""\
<html>
    <head>
        <title>CAN Node Documentation</title>
    </head>
    <body>
        <p>The following is a list of links to metadata about the format of
           <acronym title="Controller Area Network">CAN</acronym> packets
           sent by each CAN node on Impulse:
        </p>
        <ul>
""")
        for node in sorted(self.server.descriptor_sets.keys()):
            resp.body.write('            <li><a href="%s/">%s</li>\n' % (node, node))
        resp.body.write("""\
        </ul>
    </body>
</html>""")
        return resp

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
        return HTTPResponse(200, content_type="text/plain", body="Hello, world!")

    @allow('GET')
    def handle_data(self, request):
        if request.url.depth not in (2, 3):
            return self.handle_404(request)

        packet_id = request.url.segments[1]
        if packet_id not in self.server.can_descriptors:
            return self.handle_404(request)

        descr = self.server.can_descriptors[packet_id]

        message_name = None
        if request.url.depth == 3:
            message_name = unquote(request.url.segments[2])
            if message_name not in set(d[0] for d in descr["messages"]):
                return self.handle_404(request)
        
        filter_ = request.query.get('filter')[0] if request.query.get('filter') else 'latest'
        lower = request.query.get('after')[0] if request.query.get('after') else None
        upper = request.query.get('before')[0] if request.query.get('before') else None

        if lower is not None:
            try:
                lower = parse_timestamp(lower)
            except ValueError:
                return HTTPResponse(400, {"error": "timestamp after=%s is not in the relative timestamp format 1h5m or compact ISO timestamp" % lower})
        if upper is not None:
            try:
                upper = parse_timestamp(upper)
            except ValueError:
                return HTTPResponse(400, {"error": "timestamp before=%s is not in the relative timestamp format 1h5m or compact ISO timestamp" % upper})
        
        if filter_ == 'latest':
            now = datetime.datetime.utcnow()
            #By default, return the most recent packet that is no older than
            #ten minutes. Otherwise, return None.
            packet = self.server.handler.packet_cache.get(packet_id)
            if packet is None:
                resource = None
            elif (now - packet["time"]) > datetime.timedelta(minutes=10):
                resource = None
            else:
                resource = packet.copy()
                resource["time"] = packet["time"].strftime("%Y-%m-%d %H:%M:%S.%f")
        elif filter_ == "after":
            if not lower:
                return HTTPResponse(400, content_type="application/json",
                                    body=json.dumps({"error": "Missing parameter 'after' for filter mode 'after'"}))
            resource = {}
            if message_name:
                resource = self.server.load_can_messages(packet_id, message_name, lower=lower)
            else:
                for message_descr in descr["messages"]:
                    message_name = message_descr[0]
                    messages = self.server.load_can_messages(packet_id, message_name, lower=lower)
                    resource[message_name] = messages
        elif filter_ == "before":
            if not upper:
                return HTTPResponse(400, content_type="application/json",
                                    body=json.dumps({"error": "Missing parameter 'before' for filter mode 'before'"}))
            resource = {}
            if message_name:
                resource = self.server.load_can_messages(packet_id, message_name, upper=upper)
            else:
                for message_descr in descr["messages"]:
                    message_name = message_descr[0]
                    messages = self.server.load_can_messages(packet_id, message_name, upper=upper)
                    resource[message_name] = messages
        elif filter_ == "between":
            if not lower:
                return HTTPResponse(400, content_type="application/json",
                                    body=json.dumps({"error": "Missing parameter 'after' for filter mode 'after'"}))
            if not upper:
                return HTTPResponse(400, content_type="application/json",
                                    body=json.dumps({"error": "Missing parameter 'before' for filter mode 'before'"}))
            resource = {}
            if message_name:
                resource = self.server.load_can_messages(packet_id, message_name, lower=lower, upper=upper)
            else:
                for message_descr in descr["messages"]:
                    message_name = message_descr[0]
                    messages = self.server.load_can_messages(packet_id, message_name, lower=lower, upper=upper)
                    resource[message_name] = messages
        else:
            return []
        return HTTPResponse(200, content_type="application/json",
                            body=json.dumps(resource))
        
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

if __name__ == "__main__":
    logging.basicConfig(filename="server.log")
    httpd = TelemetryServer(('', 8000), TelemetryHTTPHandler)
    httpd.setup()
    print "Starting up"
    httpd.serve_forever()
