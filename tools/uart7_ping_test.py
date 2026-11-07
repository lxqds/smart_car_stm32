#!/usr/bin/env python3
"""Linux test client for the isolated UART7 TEST1 firmware; no vehicle commands."""
import argparse
import fcntl
import json
import os
import select
import termios
import time


def transact(fd, payload, expected, timeout=1.0):
    start = time.monotonic()
    deadline = start + timeout
    offset = 0
    reply = bytearray()
    while time.monotonic() < deadline:
        readable, writable, _ = select.select(
            [fd], [fd] if offset < len(payload) else [], [],
            max(0, deadline - time.monotonic()))
        if writable:
            offset += os.write(fd, payload[offset:])
        if readable:
            chunk = os.read(fd, 256)
            if not chunk:
                raise OSError('Serial device disconnected')
            reply.extend(chunk)
            if len(reply) > 256:
                raise ValueError('Unexpected oversized response')
            if b'\n' in reply:
                if bytes(reply) != expected:
                    raise ValueError('Expected %r, received %r' % (expected, bytes(reply)))
                return (time.monotonic() - start) * 1000
    raise TimeoutError('No matching response: tx=%r rx=%r' % (payload, bytes(reply)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    parser.add_argument('--count', type=int, default=100)
    args = parser.parse_args()
    if not 1 <= args.count <= 10000:
        parser.error('count must be 1..10000')
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        fcntl.ioctl(fd, termios.TIOCEXCL)
        attrs = termios.tcgetattr(fd)
        attrs[0] = attrs[1] = attrs[3] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[4] = attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        termios.tcflush(fd, termios.TCIOFLUSH)
        transact(fd, b'HELLO\n', b'HELLO UART7 TEST1 PWM_OFF\n')
        print('HELLO verified: UART7 TEST1 PWM_OFF', flush=True)
        times = []
        for sequence in range(args.count):
            token = ('%08X' % sequence).encode('ascii')
            elapsed = transact(fd, b'PING ' + token + b'\n', b'PONG ' + token + b'\n')
            times.append(elapsed)
            print(json.dumps({'seq': sequence, 'pong': token.decode(), 'rtt_ms': round(elapsed, 3)}), flush=True)
            time.sleep(0.05)
        print(json.dumps({'pass': len(times), 'mean_ms': sum(times) / len(times), 'max_ms': max(times)}))
    finally:
        os.close(fd)


if __name__ == '__main__':
    main()
