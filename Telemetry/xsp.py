import codecs
import json
import logging
import struct

class ConnectionProblem(IOError):
    "Indicates that the connection in use is broken and should be disconnected."

class XSPDemultiplexer:
    """
    Implements buffering input from the serial stream and demultiplexing the
    serial stream back into individual packets. Each packet is preceded by
    a start byte, which is 0xE7 for XSP.
    """
    START_BYTE = 0xE7
    START_CHAR = chr(START_BYTE)

    def __init__(self):
        self.buffer = bytes()

    def reset(self):
        self.buffer = bytes()

    @classmethod
    def _demux(cls, data):
        """
        Demultiplex the packets in data and return the rest of the unconsumed
        bytes. Data is returned as a tuple of (packets, remaining_data)
        """
        #For simplicity, just defer parsing a packet until the next one
        #arrives
        if data.count(cls.START_CHAR) < 2:
            return [], data

        packets = []

        #Trim bad packets from the head of the data stream
        data = data[data.index(cls.START_CHAR):]

        #While demuxing the stream, we maintain the invariant that
        #the head of the stream is always a start byte and the first
        #un-parsed packet in the stream is inbetween the start byte
        #at the head and the next start byte in the stream

        while data.count(cls.START_CHAR) >= 2:
            #Find the position of the second start byte
            index = data.index(cls.START_CHAR, 1)

            packets.append(data[1:index])

            #Trim the data that we just parsed into a packet
            data = data[index:]

        return packets, data

    def demux(self, data):
        """
        Demultiplex the packets in the demultiplexer's internal buffer with data
        appended to it. Returns a list of the packets demultiplexed, if any.
        """
        if not data:
            return []
        
        packets, self.buffer = self._demux(self.buffer + data)

class XORStreamDecoder(codecs.IncrementalDecoder):
    """
    Implements decoding data streams where the data is escaped by XORing
    significant bytes with an escape byte.
    """
    def __init__(self, escaped_bytes, escape_byte, errors):
        codecs.IncrementalDecoder.__init__(self, errors)
        self.escaped_bytes = frozenset(escaped_bytes)
        self.escape_byte   = escape_byte
        self.escape_value  = ord(escape_byte)
        self.escape_next   = False

    def decode(self, obj, final=False):
        output = ""
        for byte in obj:
            if self.escape_next:
                unescaped = bytes(chr(ord(byte) ^ self.escape_value))
                if unescaped not in self.escaped_bytes:
                    if self.errors == 'strict':
                        raise ValueError("Got an unexpected value while unescaping. %#2x %#2x -> %#2x is not a required escape sequence"
                                         % (ord(self.escape_byte), ord(byte), ord(unescaped)))
                    elif self.errors == 'ignore':
                        pass
                    elif self.errors == 'replace':
                        pass
                output += unescaped
                self.escape_next = False
            elif byte == self.escape_byte:
                self.escape_next = True
            else:
                output += byte

        if final and self.escape_next:
            if self.errors == 'strict':
                raise ValueError("Bad data stream - got EOF before finding the escaped byte after an indicator escape byte")
            elif self.errors == 'ignore':
                pass
            elif self.errors == 'replace':
                pass

        return output

    def reset(self):
        self.escape_next = False

    def getstate(self):
        return ("", self.escape_next)

    def setstate(self, state):
        buf, self.escape_next = state

class XSPStreamDecoder(XORStreamDecoder):
    """
    Implements decoding the XOR escaping scheme used by XSP where the start byte
    must be escaped by XORing it with the escape byte (and escape bytes are also
    escaped).
    """
    START_BYTE = 0xE7
    ESCAPE_BYTE = 0x75
    START_CHAR = chr(START_BYTE)
    ESCAPE_CHAR = chr(ESCAPE_BYTE)
    def __init__(self, errors='strict'):
        escaped_bytes = [self.START_CHAR, self.ESCAPE_CHAR]
        XORStreamDecoder.__init__(self, escaped_bytes, self.ESCAPE_BYTE, errors)
        
class XSPCANParser:
    """
    Implements parsing CAN packets embedded in XSP packets
    """
    ID_MASK  = 0xFFF0
    LEN_MASK = 0x000F
    MAX_PAYLOAD_SIZE = 8
    def __init__(self, lookup_format):
        self.lookup_format = lookup_format

    def parse(self, packet):
        """
        Parses a single XSP packet and returns a 3-tuple of the datetime
        timestamp of when the packet was parsed, the CAN packet id, and
        a list of the packed values from the payload.
        """
        if len(packet) < 2:
            raise ValueError("Packet is too short to have a preamble")

        (preamble,) = struct.unpack("<H", packet[:2])
        id_  = (preamble & self.ID_MASK) >> 4
        len_ = preamble & self.LEN_MASK

        if len_ > self.MAX_PAYLOAD_SIZE:
            raise ValueError("Invalid CAN payload size %d is greater than the maximum size %d"
                             % (len_, self.MAX_PAYLOAD_SIZE))
        elif len(packet[2:]) != len_:
            raise ValueError("Stated payload size %d mismatches actual size %d for packet w/id=%#x, data=%r"
                             % (len_, len(packet[2:]), id_, packet[2:]))
        time = datetime.datetime.utcnow()
        try:
            format_ = self.lookup_format(id_)
        except KeyError:
            raise ValueError("Unknown packet id=%#x, len=%d, data=%r" %
                             (id_, len_, packet[2:]))
        if format_ is None or struct.calcsize(format_) == 0:
            if len_ != 0 or len(packet[2:]) != 0:
                raise ValueError("Expected packet w/id=%#x to have no payload, but got len=%d and data=%r"
                                 % (id_, len_, packet[2:]))
            data = []
        else:
            expected_len = struct.calcsize(format_)
            if len_ != expected_len:
                raise ValueError("Wrong payload size=%d for id=%#x data=%r; expected size %d"
                                 % (len_, id_, packet[2:], expected_len))
            data = struct.unpack(format_, packet)
        return time, id_, data

class XSPHandler:
    """
    Implements the full XSP stack to produce fully parsed messages from a
    serial stream.
    """

    def __init__(self, lookup_format, lookup_descr):
        self.port = None
        self.demuxer = XSPDemultiplexer()
        self.decoder = XSPStreamDecoder()
        self.parser  = XSPCanParser(lookup_format)
        self.lookup_descr = lookup_descr

    def bind(self, port):
        if self.port:
            self.unbind()
        self.port = port

    def unbind(self):
        self.port = None
        self.demuxer.reset()
        self.decoder.reset()

    def poll(self, num_bytes):
        if not self.port:
            return []
        
        try:
            data = self.port.read(num_bytes)
        except ValueError:
            logging.exception("Error reading from serial port")
            raise ConnectionProblem("Error while reading from serial port")

        try:
            xsp_packets = self.demuxer.demux(data)
        except:
            logging.exception("Error while demultiplexing stream")
            return
        
        for xsp_packet in xsp_packets:
            try:
                can_packet = self.parser.parse(self.decoder.decode(xsp_packet, final=True))
            except ValueError:
                logging.exception("Read bad CAN packet")
                continue
            except:
                logging.exception("Error while reading CAN packet")
                continue

            time, id_, data = can_packet
            descr = self.lookup_descr(id_)

            if not descr['messages']:
                return [(time, id_, descr['name'], None)]
            else:
                messages = []
                for msg_name, datum in zip(descr['messages'], datum):
                    messages.append((time, id_, msg_name, datum))
                return messages
