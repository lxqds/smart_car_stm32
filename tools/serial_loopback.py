#!/usr/bin/env python3
"""Linux USB-UART loopback check. Disconnect MCU; bridge adapter TX and RX.

This is a wiring test, NOT the smart-car protocol or an MCU communication test.
No third-party Python packages required.
"""
import argparse
import fcntl
import os
import select
import termios
import time


def configure(fd):
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)


def exchange(fd, payload, timeout=1.0):
    deadline = time.monotonic() + timeout
    sent = 0
    received = bytearray()
    while sent < len(payload) or len(received) < len(payload):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError('sent=%d received=%d expected=%d' %
                               (sent, len(received), len(payload)))
        readable, writable, _ = select.select(
            [fd], [fd] if sent < len(payload) else [], [], remaining)
        if writable:
            sent += os.write(fd, payload[sent:])
        if readable:
            data = os.read(fd, 4096)
            if not data:
                raise OSError('Serial device disconnected')
            received.extend(data)
    if received != payload:
        raise ValueError('Loopback data mismatch')


def self_test():
    import pty
    import threading
    master, slave = pty.openpty()
    configure(slave)
    payload = bytes(range(256)) * 4
    errors = []

    def fragmented_echo():
        try:
            total = 0
            while total < len(payload):
                if not select.select([master], [], [], 2)[0]:
                    raise TimeoutError('PTY echo timeout')
                data = os.read(master, 17)
                total += len(data)
                offset = 0
                while offset < len(data):
                    offset += os.write(master, data[offset:])
        except Exception as exc:
            errors.append(exc)

    worker = threading.Thread(target=fragmented_echo, daemon=True)
    try:
        worker.start()
        exchange(slave, payload, 3)
        worker.join(3)
        if worker.is_alive() or errors:
            raise RuntimeError('PTY worker failed: %r' % errors)
        try:
            exchange(slave, b'no-echo', 0.1)
        except TimeoutError:
            pass
        else:
            raise AssertionError('Missing response must time out')
        print('PASS: PTY fragmented binary echo and missing-response timeout')
    finally:
        os.close(master)
        os.close(slave)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--port')
    parser.add_argument('--adapter-loopback-only', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if not args.port or not args.adapter_loopback_only:
        parser.error('Provide --port and --adapter-loopback-only; disconnect MCU first')
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        fcntl.ioctl(fd, termios.TIOCEXCL)
        configure(fd)
        for sequence in range(100):
            exchange(fd, sequence.to_bytes(2, 'little') + bytes(range(256)))
        print('PASS: 100 exchanges / 25800 bytes; 115200 8N1 adapter loopback ONLY')
    finally:
        os.close(fd)


if __name__ == '__main__':
    main()
