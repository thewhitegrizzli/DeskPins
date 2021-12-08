"""Manage arbitrary payload data at the end of binary files.

Payload is stored as:
    <binary data string> <32-bit data size> <32-bit magic id> EOF
"""

import os
import struct
 

def get(fn, magic):
    """Get payload string or None."""
    with open(fn, 'rb') as f:
        f.seek(0, os.SEEK_END)
        if f.tell() < 8:
            return None
        f.seek(-8, os.SEEK_CUR)
        size, id_ = struct.unpack('LL', f.read(8))
        if magic != id_:
            return None
        f.seek(-8 - size, os.SEEK_CUR)
        return f.read(size)


def set(fn, magic, data):
    """Set or replace payload string."""
    clear(fn, magic)
    with open(fn, 'ab') as f:
        f.write(data + struct.pack('LL', len(data), magic))


def clear(fn, magic):
    """Remove payload data if it exists."""
    with open(fn, 'r+b') as f:
        f.seek(0, os.SEEK_END)
        if f.tell() < 8:
            return
        f.seek(-8, os.SEEK_CUR)
        size, id_ = struct.unpack('LL', f.read(8))
        if magic == id_:
            f.seek(-8 - size, os.SEEK_CUR)
            f.truncate()
