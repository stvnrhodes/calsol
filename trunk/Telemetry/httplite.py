import urlparse
import logging
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
import traceback

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

from odict import OrderedDict

__all__ = ["HTTPRequest", "HTTPResponse", "StaticHTTPResponse",
           "LiteHTTPHandler",
           "allow"]

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

    def query_first(self, name):
        return self.query.get(name, [None])[0]
    
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

class LiteHTTPHandler(BaseHTTPRequestHandler):
    def _respond(self, handler, request, disable_500=False):
        try:
            response = handler(request)
            if response is None:
                return
            content = response.body.getvalue() if response.body else ""
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
