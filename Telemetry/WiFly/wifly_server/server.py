# server.py: Wifly server for CalSol.  Sets up a HTTP server to accept POST
#            data coming from a WiFly module, and mirrors it out via JSON

import cStringIO
import json
import telnetlib
import SimpleHTTPServer
import SocketServer
import sys


class WiflyData():
  """An object used to store the current state of wifly data"""
  def __init__(self):
    # Stores unused data
    self.buf = ''
    self.can_data = {'0x1': 'ASDF'}
    
  def write(self, data):
    """Entry point for new data"""
    self.buf += data
  
  def process(self):
    """Processes data in buffer, leaves unprocessed data in buffer.
    Format is expected as follows:
    Bytes 0-3: CAN ID in HEX
    Byte 4: DLC (Length)
    Bytes 5-end: Data
    Deliminator: Newline
    """
    # If we have an empty buffer, return
    if not self.buf: return
    packets = self.buf.split('\n')
    # If the very last item is not an empty string (representing a newline),
    # Leave it in the buffer
    if not packets[-1]:
      self.buf = packets[-1]
      packets = packets[:-1]
    else:
      # All packets are full packets, empty the buffer
      self.buf = ''
    
    # Verify all packets, and store them
    for packet in packets:
      if len(packet) < 4:
        # Invalid packet, missing header
        continue
      can_id = packet[0:3]
      can_id_hex = int(can_id, 16)
      if can_id_hex > 0x7FF:
        # Invalid CAN ID
        continue
      dlc = int(packet[3])
      if dlc > 8:
        # Invalid DLC
        continue
      data = packet[4:]
      if len(data) != (dlc * 2):
        # Data does not match DLC
        continue
      self.can_data['0x%X' % can_id_hex] = data
      

data = WiflyData()

class WiflyServer(SimpleHTTPServer.SimpleHTTPRequestHandler):
  """A instance of a WiFly server"""
  
  def do_GET(self):
    """Access point for web server.  Can receive data or output data."""
    global data
    # Get essential data
    req_path = self.path.split('?')[0]
    req_list = (''.join(self.path.split('?')[1:])).split('&')
    request = {}
    for req in req_list:
      if req:
        k, v = req.split('=')
        request[k] = v
    
    if req_path == '/post':
      # Data coming from WiFly, store 'data' into WiflyData
      self.send_response(200)
      self.send_header('Content-type', 'text/html')
      self.end_headers()
      if 'data' not in req:
        return  # Invalid request, fail gracefully
      data.write(req['data'])
      data.process()
    elif req_path == '/get':
      # Request from client, send out wifly data as JSON object
      self.send_response(200)
      self.send_header('Content-type', 'text/json')
      self.end_headers()
      self.wfile.write(json.dumps(data.can_data))
    elif req_path == '/' or req_path == '/index.html':
      try:
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(open('index.html').read())
      except IOError:
        self.send_error(404, 'index.html not found')
    elif req_path == '/jquery.js':
      try:
        self.send_response(200)
        self.send_header('Content-type', 'text/javascript')
        self.end_headers()
        self.wfile.write(open('jquery.js').read())
      except IOError:
        self.send_error(404, 'jquery.js not found')
    else:
      # Unknown path
      self.send_error(404, 'Reqeust not recognized')
      
      
if __name__ == '__main__':
  try:
    server = SocketServer.TCPServer(('', 8000), WiflyServer)
    print 'Started WiFly server...'
    server.serve_forever()
  except KeyboardInterrupt:
    print '^C received, shutting down...'
    server.socket.close()