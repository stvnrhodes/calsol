import base64
import collections
import hashlib
import random
import struct

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

from httplite import HTTPResponse

__all__ = ["WebSocket", "SingleOptionPolicy", "UnrestrictedPolicy"]

def split_field(s):
    return [e.strip() for e in s.split(",")]

def validate_key(client_key):
    try:
        decoded = base64.standard_b64decode(client_key)
        if len(decoded) == 16:
            return True
    except:
        pass

    return False

def compute_key(client_key):
    GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    hash_ = hashlib.sha1()
    hash_.update(client_key)
    hash_.update(GUID)
    return base64.standard_b64encode(hash_.digest())

class Policy(object):
    def __init__(self):
        pass

    def set_acceptor(self, func):
        self.accept = func

    def set_chooser(self, func):
        self.choose = func

    @classmethod
    def make_policy(cls, accept, choose):
        policy = cls()
        policy.set_acceptor(accept)
        policy.set_chooser(choose)
        return policy

class SingleOptionPolicy(Policy):
    def __init__(self, option):
        self.option = option

    def accept(self, options):
        return self.option in options

    def choose(self, options):
        return self.option

class UnrestrictedPolicy(Policy):
    def accept(self, options):
        return True

    def choose(self, options):
        return options[0]

class PolicyError(ValueError):
    pass

class BitProperty(object):
    def __init__(self, field, position):
        #Position is the position where the LSB is at position 0
        self.field = field
        self.position = position
        self.mask = 1 << position

    def __get__(self, obj, type_):
        return int(getattr(obj, self.field) & self.mask != 0)
    
    def __set__(self, obj, value):
        if not (0 <= value <= 1):
            raise ValueError("Value %s cannot fit in a 1-bit field" % (value,))
        v = getattr(obj, self.field, 0)
        v = (v | self.mask) ^ self.mask
        setattr(obj, self.field, v | (value << self.position))

class MultiBitProperty(object):
    def __init__(self, field, start, end):
        self.field = field
        if start > end:
            start, end = end, start
        self.start = start
        self.end = end

        mask = 0
        for i in xrange(start, end + 1):
            mask |= 1 << i

        self.mask = mask

    def __get__(self, obj, type_):
        value = getattr(obj, self.field)
        return (value & self.mask) >> self.start

    def __set__(self, obj, value):
        if value != (value & (self.mask >> self.start)):
            raise ValueError("Value %s can't fit in a %d-bit field" % (value, width))
            
        v = getattr(obj, self.field, 0)
        #clear the bits occupied by the mask, taking into account variable widths
        v = (v | self.mask) ^ self.mask
        v |= value << self.start

        setattr(obj, self.field, v)

class ByteProperty(object):
    def __init__(self, field):
        self.field = field
        
    def __get__(self, obj, type_):
        return getattr(obj, self.field)

    def __set__(self, obj, value):
        if value != value & 0xff:
            raise ValueError("Value %s can't fit in a 1-byte field" % (value,))
        setattr(obj, self.field, value)

    def __delete__(self, obj, value):
        if hasattr(obj, self.field):
            delattr(obj, self.field)

class MultiByteProperty(object):
    def __init__(self, *fields):
        self.fields = fields
        self.width = len(fields)

    def __get__(self, obj, type_):
        value = 0
        for field in self.fields:
            value = (value << 8) | getattr(obj, field)
        return value

    def __set__(self, obj, value):
        if value >> (self.width * 8) != 0:
            raise ValueError("Value %s can't fit in a %d-byte field" % (value, self.width))
        for field in reversed(self.fields):
            setattr(obj, field, value & 0xff)
            value = value >> 8

    def __delete__(self, obj):
        for field in self.fields:
            if hasattr(obj, field):
                delattr(obj, field)

class Frame(object):
    OP_CONTINUE = 0x0
    OP_TEXT = 0x1
    OP_BINARY = 0x2
    OP_CLOSE = 0x8
    OP_PING = 0x9
    OP_PONG = 0xA

    CONTROL_OPCODES = frozenset(xrange(0x8, 0xF+1))
    
    @classmethod
    def read_from(cls, stream):
        frame = cls()
        stream_read = stream.read
        def next_byte():
            char = stream_read(1)
            if not char:
                raise EOFError()
            return ord(char)
        frame.control_byte = next_byte()
        frame.len_byte = next_byte()
        if frame.LEN8 >= 126:
            frame.len_ext16_byte1 = next_byte()
            frame.len_ext16_byte2 = next_byte()
        if frame.LEN8 == 127:
            frame.len_ext64_byte3 = next_byte()
            frame.len_ext64_byte4 = next_byte()
            frame.len_ext64_byte5 = next_byte()
            frame.len_ext64_byte6 = next_byte()
            frame.len_ext64_byte7 = next_byte()
            frame.len_ext64_byte8 = next_byte()

        if frame.MASK:
            frame.mask_byte1 = next_byte()
            frame.mask_byte2 = next_byte()
            frame.mask_byte3 = next_byte()
            frame.mask_byte4 = next_byte()
            
            frame.payload = StringIO()

            xor_payload(stream, frame.payload, frame.KEY, frame.LEN)
            frame.payload.seek(0)
        else:
            frame.payload = StringIO(stream_read(frame.LEN))
        return frame

    def __init__(self, **kwargs):
        self.payload = None
    
    def configure(self, OPCODE, LEN, payload, FIN=1, MASK_KEY=None):
        self.FIN = FIN
        self.OPCODE = OPCODE
        self.LEN = LEN

        if MASK_KEY is not None:
            self.KEY = MASK_KEY

        if hasattr(payload, "read"):
            self.payload = StringIO(payload.read(LEN))
        else:
            self.payload = StringIO(payload)

    FIN    = BitProperty("control_byte", 7)
    RSV1   = BitProperty("control_byte", 6)
    RSV2   = BitProperty("control_byte", 5)
    RSV3   = BitProperty("control_byte", 4)
    OPCODE = MultiBitProperty("control_byte", 0, 3)
    MASK   = BitProperty("len_byte", 7)
    LEN8   = MultiBitProperty("len_byte", 0, 6)
    LENx16 = MultiByteProperty("len_ext16_byte1", "len_ext16_byte2")
    LENx64 = MultiByteProperty("len_ext16_byte1", "len_ext16_byte2",
                               "len_ext64_byte3", "len_ext64_byte4",
                               "len_ext64_byte5", "len_ext64_byte6"
                               "len_ext64_byte7", "len_ext64_byte8")
    KEY    = MultiByteProperty("mask_byte1", "mask_byte2",
                               "mask_byte3", "mask_byte4")
    
    def _get_len(self):
        if self.LEN8 == 126:
            return self.LENx16
        elif self.LEN8 == 127:
            return self.LENx64
        else:
            return self.LEN8

    def _set_len(self, value):
        if 0 <= value <= 125:
            del self.LENx64
            self.LEN8 = value
        elif value < 2**16:
            del self.LENx64
            self.LEN8 = 126
            self.LENx16 = value
        elif value < 2**64:
            self.LEN8 = 127
            self.LENx64 = value
        else:
            raise ValueError("Payload size can't fit in a 64-bit integer")

    LEN = property(_get_len, _set_len)

    def is_control(self):
        return self.OPCODE in self.CONTROL_OPCODES

    def write_to(self, stream):
        stream.write(chr(self.control_byte))
        stream.write(chr(self.len_byte))
        if self.LEN8 >= 126:
            stream.write(chr(self.len_ext16_byte1))
            stream.write(chr(self.len_ext16_byte2))
        if self.LEN8 == 127:
            stream.write(chr(self.len_ext64_byte3))
            stream.write(chr(self.len_ext64_byte4))
            stream.write(chr(self.len_ext64_byte5))
            stream.write(chr(self.len_ext64_byte6))
            stream.write(chr(self.len_ext64_byte7))
            stream.write(chr(self.len_ext64_byte8))
        if self.MASK:
            stream.write(chr(self.mask_byte1))
            stream.write(chr(self.mask_byte2))
            stream.write(chr(self.mask_byte3))
            stream.write(chr(self.mask_byte4))
        if self.payload:
            if self.MASK:
                xor_payload(self.payload, stream, self.KEY, self.LEN)
            else:
                stream.write(self.payload.read(self.LEN))

class Message(object):
    
    def __init__(self, opcode, payload=None):
        self.opcode = opcode
        self.payload = payload if payload is not None else StringIO()
        self.complete = False
        self._cached_string = None
        self.frames = []

    def extend(self, frame):
        self.frames.append(frame)
        self.payload.write(frame.payload.read())
        frame.payload.seek(0)

    def finish(self):
        self.complete = True

    @property
    def payload_string(self):
        if self._cached_string == None:
            self._cached_string = self.payload.getvalue()
        return self._cached_string

    @property
    def is_close(self):
        return self.opcode == Frame.OP_CLOSE

    @property
    def is_ping(self):
        return self.opcode == Frame.OP_PING

    @property
    def is_pong(self):
        return self.opcode == Frame.OP_PONG

    @property
    def is_text(self):
        return self.opcode == Frame.OP_TEXT

    @property
    def is_binary(self):
        return self.opcode == Frame.OP_BINARY

    @classmethod
    def make_text(cls, payload, LEN, mask=False):
        message = cls(Frame.OP_TEXT, payload)
        message.frames.extend(cls.fragment_message(Frame.OP_TEXT, LEN, payload, mask))
        return message

    @staticmethod
    def fragment_message(opcode, LEN, payload, mask=False, FRAGMENT_SIZE=1452):
        """
        Fragments a message with the given opcode into multiple frames, if necessary.
        If mask is true, a mask key is randomly generated and included, even if the
        payload is empty.

        Returns a list of frames suitable for sending over the wire
        """
        #There's not really any obvious way to fragment messages
        #But the simplest is just to fragment at the MTU size, going by
        #Ethernet, which is 1500, less 20 bytes for an IPv4 header and
        #20 bytes for a TCP header and 4 bytes for a 16-bit payload and
        #possibly 4 bytes for a masking key, for a total of 1452 bytes free
        frames = []
        first = True
        key = None
        if LEN == 0:
            frame = Frame()
            frame.configure(opcode, 0, None, FIN=1)
            if mask:
                frame.KEY = gen_mask_key()
                frame.MASK = 1
            return [frame]
        
        while LEN >= FRAGMENT_SIZE:
            frame = Frame()
            frame.OPCODE = opcode if first else Frame.OP_CONTINUE
            frame.LEN = FRAGMENT_SIZE
            frame.payload = StringIO(payload.read(FRAGMENT_SIZE))
            if mask:
                frame.KEY = gen_mask_key()
                frame.MASK = 1
            first = False
            LEN -= FRAGMENT_SIZE
            frames.append(frame)

        if LEN > 0:
            frame = Frame()
            frame.OPCODE = opcode if first else Frame.OP_CONTINUE
            frame.LEN = LEN
            frame.payload = StringIO(payload.read(LEN))
            if mask:
                frame.KEY = gen_mask_key()
                frame.MASK = 1
            frames.append(frame)
        frames[-1].FIN = True
        return frames

def gen_mask_key():
    return random.randint(-2**15, 2**15-1)

def xor_payload(in_stream, out_stream, mask_key, num_bytes=-1):
    "Takes in a host-order masking key and xors data from in_stream and writes to out_stream"
    if mask_key & 0xffffffff != mask_key:
        raise ValueError("Mask Key %s doesn't fit in a 32-bit integer" % (mask_key,))
    if num_bytes == -1:
        chunk = in_stream.read(4)
        while len(chunk) == 4:
            (stuff,) = struct.unpack("!L", chunk)
            out_stream.write(struct.pack("!L", stuff ^ mask_key))
            chunk = in_stream.read(4)
    else:
        bytes_left = num_bytes
        chunk = in_stream.read(min(4, bytes_left))
        while len(chunk) == 4:
            bytes_left -= len(chunk)
            (stuff,) = struct.unpack("!L", chunk)
            out_stream.write(struct.pack("!L", stuff ^ mask_key))
            chunk = in_stream.read(min(4, bytes_left))
    for i, char in enumerate(chunk):
        octet = ord(char)
        key_octet = (mask_key >> ((3-i)*8)) & 0xff
        out_stream.write(chr(octet ^ key_octet))

class WebSocket(object):
    def __init__(self, request, handler):
        self.request = request
        self.handler = handler
        self.buffered_message = None
        self.frame_queue = collections.deque()

    @staticmethod
    def validate(request, policies):
        #Check if we support their WebSocket version
        versions = split_field(request.headers.get("Sec-WebSocket-Version"))
        if not policies["version"].accept(versions):
            raise PolicyError("Unsupported version")
        
        #Check that the upgrade request looks sane
        if request.headers.get("Connection") != "Upgrade":
            raise ValueError("Invalid upgrade request")

        #Do a sanity check on the client key
        client_key = request.headers.get("Sec-WebSocket-Key", "").strip()
        if not validate_key(client_key):
            raise ValueError("Invalid client key")

        origin_policy = policies.get("origin", UnrestrictedPolicy())
        host_policy = policies.get("host", UnrestrictedPolicy())

        #Check if we accept their origin
        if not origin_policy.accept([request.headers.get("Origin")]):
            raise PolicyError("Origin not accepted")

        #Check if we accept the host
        if not host_policy.accept([request.headers.get("Host")]):
            raise PolicyError("Host not accepted")
        
    def negotiate(self, policies):
        version_string = self.request.headers.get("Sec-WebSocket-Version")
        client_versions = split_field(version_string)
        self.version = policies["version"].choose(client_versions)

        protocol_string = self.request.headers.get("Sec-WebSocket-Protocol", "")
        client_protocols = split_field(protocol_string)
        self.protocol = policies["protocol"].choose(client_protocols)

        client_key = self.request.headers.get("Sec-WebSocket-Key", "").strip()
        self.accept_key = compute_key(client_key)
        
        self.handler.send_response(101)
        self.handler.send_header("Upgrade", "websocket")
        self.handler.send_header("Connection", "Upgrade")
        self.handler.send_header("Sec-WebSocket-Accept", self.accept_key)
        self.handler.send_header("Sec-WebSocket-Protocol", self.protocol)
        self.handler.send_header("Sec-WebSocket-Version", self.version)
        self.handler.end_headers()

    def process_frame(self, frame):
        if frame.is_control:
            if not frame.FIN:
                raise ValueError("Got a fragmented control message")
            else:
                message = Message(frame.OPCODE)
                message.extend(frame)
                message.finish()
                return message
        
        if self.buffered_message is not None:
            if not frame.is_continuation:
                raise ValueError("Got a new data frame before fragmented data frame was finished")
        elif frame.is_continuation:
            raise ValueError("Got a data frame continuation before receiving first data frame")
        else:
            self.buffered_message = Message(frame.OPCODE)

        self.buffered_message.extend(frame)

        if frame.FIN:
            message = self.buffered_message
            message.finish()

            self.buffered_message = None
            return message
        else:
            return None
    def read_message(self):
        message = self.process_frame(self.read_frame())
        while message is None:
            message = self.process_frame(self.read_frame())

        return message
        
    def read_frame(self):
        frame = Frame.read_from(self.handler.rfile)
##        print " FIN:", frame.FIN
##        print " RSV:", "%d%d%d" % (frame.RSV1, frame.RSV2, frame.RSV3)
##        print "  OP:", hex(frame.OPCODE)
##        print " LEN:", frame.LEN
##        print " KEY:", hex(frame.KEY)
##        print "DATA:", frame.payload.getvalue()
####        import sys
####        xor_payload(frame.masked_payload, sys.stdout, frame.KEY)
####        print
####        frame.masked_payload.seek(0)

        return frame

    def send_message(self, message, override=False):
        """
        Enqueue a message's frames to be sent over the wire and block until
        the frame queue is empty. Control frames may be sent in-between.
        If override is set to true, the frame is enqueued at the head of the
        frame queue. This should only be used for control frames.
        """
        if override:
            self.frame_queue.appendleft(message.frames[0])
        else:
            self.frame_queue.extend(message.frames)
            
        while self.frame_queue:
            self.send_frame(self.frame_queue.popleft())
        
    
    def send_frame(self, frame):
        "Sends a frame over the wire"
        frame.write_to(self.handler.wfile)
        self.handler.wfile.flush()
