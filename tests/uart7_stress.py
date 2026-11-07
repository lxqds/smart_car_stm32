#!/usr/bin/env python3
"""Diagnostic newline flood followed by exact PING recovery; no vehicle commands."""
import os, termios, fcntl, select, time, json

fd=os.open('/dev/ttyUSB0',os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
def receive_for(seconds):
    end=time.monotonic()+seconds; data=b''
    while time.monotonic()<end:
        if select.select([fd],[],[],max(0,end-time.monotonic()))[0]:
            part=os.read(fd,4096)
            if not part: raise OSError('Disconnected')
            data+=part
    return data
def ping(token):
    os.write(fd,b'PING '+token+b'\n')
    actual=receive_for(.3)
    assert actual==b'PONG '+token+b'\n',repr(actual[-100:])
try:
    fcntl.ioctl(fd,termios.TIOCEXCL)
    a=termios.tcgetattr(fd);a[0]=a[1]=a[3]=0
    a[2]=termios.CS8|termios.CREAD|termios.CLOCAL
    a[4]=a[5]=termios.B115200;a[6][termios.VMIN]=a[6][termios.VTIME]=0
    termios.tcsetattr(fd,termios.TCSANOW,a);termios.tcflush(fd,termios.TCIOFLUSH)
    ping(b'C0000000')
    end=time.monotonic()+5;sent=received=0
    while time.monotonic()<end:
        r,w,_=select.select([fd],[fd],[],.01)
        if r:received+=len(os.read(fd,4096))
        if w:
            try:sent+=os.write(fd,b'\n'*64)
            except BlockingIOError:pass
    # Drain both the adapter's outgoing backlog and MCU replies before recovery.
    termios.tcdrain(fd)
    received+=len(receive_for(3))
    termios.tcflush(fd,termios.TCIOFLUSH)
    ping(b'C0000001')
    print(json.dumps(dict(test='newline_flood_5s',sent=sent,received=received,recovered=True)))
finally:os.close(fd)
