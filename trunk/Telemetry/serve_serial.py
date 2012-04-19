import select
import serial
import socket

WRITE_CHUNK_SIZE = 4096
READ_CHUNK_SIZE = 4096

class RemoteSerialClient(object):
    """
    Represents a remote client reading serial data over a socket. This class
    handles buffering data and writing data to the client over its socket.
    """
    def __init__(self, socket):
        self.buffer = ""
        self.socket = socket

    def write(self):
        if self.buffer:
            sent = self.socket.send(self.buffer[:WRITE_CHUNK_SIZE])
            if sent:
                print "I sent some data: ", self.buffer[:sent]
                self.buffer = self.buffer[sent:]
            else:
                print "I tried to send some data, but failed"

    def push(self, data):
        self.buffer += data

    def flush(self):
        while self.buffer:
            try:
                sent = self.socket.send(self.buffer)
            except socket.error:
                return
            if sent:
                self.buffer = self.buffer[sent:]

    def close(self):
        self.socket.shutdown(socket.SHUT_RDWR)
        self.socket.close()

    def fileno(self):
        return self.socket.fileno()

    def has_unsent_data(self):
        return len(self.buffer) > 0

class FileClient(object):
    """
    Represents a file 'client' to write serial data to. This is only usable if
    the operating system allows polling on actual file descriptors.
    """
    def __init__(self, fname):
        self.file = open(fname, "ab+", READ_CHUNK_SIZE)
        self.buffer = ""

    def write(self):
        self.file.write(self.buffer)
        self.buffer = ""

    def push(self, data):
        self.buffer += data

    def flush(self):
        if self.buffer:
            self.file.write(self.buffer)
        self.file.flush()

    def close(self):
        self.file.close()

    def fileno(self):
        return self.file.fileno()

    def has_unsent_data(self):
        return len(self.buffer) > 0

class SerialServer(object):
    """
    Implements a (mostly) asynchronous Serial server that reads in data from
    a serial port and broadcasts it to any listening clients over TCP,
    optionally logging data to a file.
    """
    def __init__(self, address, log_filename=None, logger=None):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setblocking(False)
        #self.socket.setsockopt()
        self.socket.bind(address)
        self.running = False
        self.shutting_down = False

        self.clients = {}
        self.serial_port = None

    def connect(self, port, **settings):
        """
        Connect to a serial port named `port` with the same settings as the
        serial.Serial object and open it for serial broadcast. Timeout will
        be set to 0, if it is not specified
        """
        
        if "timeout" not in settings:
            settings["timeout"] = 0
        self.serial_port = serial.Serial(port, **settings)

    def log_to(self, fname):
        """
        Opens the file at path `fname` in binary append mode and records all
        serial data to it.
        """
        
        self.log_file = open(fname, "ab+", READ_CHUNK_SIZE)

    def set_serial_port(self, serial_port):
        """
        Set an already opened port as the port for serial broadcast. To
        avoid blocking when no data is being read, the timeout should be set.
        """
        
        self.serial_port = serial_port

    def set_log_file(self, f):
        """
        Set an already opened file as the log file to record serial data to.
        """
        
        if self.log_file is not None:
            self.log_file.flush()
            self.log_file.close()
        self.log_file = f

    def select_setup(self):
        "Setup level trigger state for select(). Currently a nop"
        pass

    def select_add(self, fd):
        "Add an fd to level trigger state for select(). Currently a nop"
        pass

    def select_remove(self, fd):
        "Remove an fd from level trigger state for select(). Currently a nop"
        pass

    def select(self):
        """
        Uses select() to check if there are incoming connections or writeable
        sockets and returns a tuple of (connections_waiting, writeable_sockets)
        """
        
        rlist, wlist, xlist = select.select([self.socket], self.clients.keys(), [], 0)
        return (rlist != [], wlist)

    def poll_setup(self):
        "Setup level trigger state for poll()"
        
        #Create the poll object that tracks which fds and events we care about.
        self.poll_obj = select.poll()

        #Register our server socket to check for incoming connections
        self.poll_obj.register(self.socket, select.POLLIN)

    def poll_add(self, fd):
        "Add an fd to level trigger state for poll()"
        
        #Register the fd for write availability events
        self.poll_obj.register(fd, select.POLLOUT)

    def poll_remove(self, fd):
        "Remove an fd from level trigger state for poll()"

        #Unregister the fd
        self.poll_obj.unregister(fd)

    def poll(self):
        """
        Uses poll() to check if there are incoming connections or writeable
        sockets and returns a tuple of (connections_waiting, writeable_sockets)
        """

        connections_waiting = False
        writeable = []
        
        ready = self.poll_obj.poll(0)
        for (fd, event) in ready:
            if fd == self.socket.fileno():
                connections_waiting = bool(event & select.POLLIN)
            elif event & select.POLLOUT:
                writeable.append(fd)
        
        return (connections_waiting, writeable)

    def accept(self):
        "Accept a socket and return the client to handle it, or None on failure"
        pair = self.socket.accept()
        if pair is None:
            return None

        sock, addr = pair
        print "Accepted client from %s" % (addr,)
        client = RemoteSerialClient(sock)
        return client
    
    def run(self):
        "Start listening for connections and broadcast serial data"
        if self.serial_port is None:
            raise ValueError("No serial port is connected - nothing to broadcast")
        
        if hasattr(select, "poll"):
            get_ready_io = self.poll
            add_client = self.poll_add
            remove_client = self.poll_remove
            self.poll_setup()
        else:
            get_ready_io = self.select
            add_client = self.select_add
            remove_client = self.select_remove
            self.select_setup()

        self.socket.listen(1)
        print "Broadcasting data from serial port '%s'" % self.serial_port.name
        print "SerialServer listening on %s" % (self.socket.getsockname(),)
        self.running = True
        
        while self.running:
            try:
                #No way to check for this on windows, so just poll always.
                data_waiting = not self.shutting_down
                
                #Do level checking to see which sockets to service
                connections_waiting, writeable = get_ready_io()

                if not writeable:
                    print "Nobody available to write"
                #Push data to clients
                for fd in writeable:
                    client = self.clients[fd]
                    should_cleanup = False
                    try:
                        client.write()
                    except socket.error:
                        should_cleanup = True

                    if self.shutting_down and not client.has_unsent_data():
                        should_cleanup = True

                    if should_cleanup:
                        client.close()
                        remove_client(fd)
                        del self.clients[fd]

                #Handle incoming connections from new clients
                if connections_waiting and not self.shutting_down:
                    client = self.accept()
                    if client is not None:
                        self.clients[client.fileno()] = client
                        add_client(client)

                #Poll for serial data to push to clients
                if data_waiting and not self.shutting_down:
                    try:
                        data = self.read_serial_data()
                        if data:
                            print "Got data:", data
                    except serial.SerialException:
                        #If the serial port breaks, stop accepting clients,
                        #flush existing data
                        print "ohnoes"
                        import traceback
                        traceback.print_exc()
                        data_waiting = False
                        self.shutting_down = True
                        self.socket.close()
                    else:
                        for client in self.clients.values():
                            client.push(data)

                #Check if we're done flushing data and should finish shutting down
                if self.shutting_down and not self.clients:
                    self.running = False

            except KeyboardInterrupt:
                #If we're already shutting down, just close down everything
                if self.shutting_down:
                    for fd, client in self.clients.items():
                        client.close()
                        remove_client(fd)
                        del self.clients[fd]
                    self.running = False
                #Otherwise just start a soft shutdown
                else:
                    self.shutting_down = True

        print "SerialServer shutdown"

    def read_serial_data(self):
        "Read in some serial data, if possible"
        return self.serial_port.read(READ_CHUNK_SIZE)
        

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Broadcast serial data over TCP")
    parser.add_argument('port_name', nargs='+', metavar="port-name",
                        help="the serial port device name(s) to listen on")
    parser.add_argument('--baud-rate', type=int, nargs='?', default=115200,
                        help="the baud rate at which the serial port communicates")
    parser.add_argument('--host-name', nargs='?', default="localhost",
                        help="the hostname for the server to bind to")
    parser.add_argument('--host-port', type=int, nargs='?', default=6543,
                        help="the port for the server to listen on")
    parser.add_argument('--log-file', nargs='?',
                        help="the file to append all serial data to")

    args = parser.parse_args()
    
    server = SerialServer((args.host_name, args.host_port), args.log_file)
    server.connect(args.port_name[0], baudrate=args.baud_rate, timeout=0.1)
    server.run()
