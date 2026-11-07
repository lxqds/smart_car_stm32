"""Non-actuating UART7 TEST1 parser checks, Linux stdlib only."""
import os, termios, fcntl, select, time, json

fd = os.open('/dev/ttyUSB0', os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
def read_for(seconds):
    end = time.monotonic() + seconds
    result = b''
    while time.monotonic() < end:
        if select.select([fd], [], [], max(0, end-time.monotonic()))[0]:
            chunk = os.read(fd, 512)
            if not chunk:
                raise OSError('Disconnected')
            result += chunk
    return result

def check(name, parts, expected):
    for data, delay in parts:
        os.write(fd, data)
        time.sleep(delay)
    actual = read_for(0.4)
    ok = actual == expected
    print(json.dumps(dict(case=name, passed=ok, expected=repr(expected), actual=repr(actual))), flush=True)
    return ok

try:
    fcntl.ioctl(fd, termios.TIOCEXCL)
    a = termios.tcgetattr(fd)
    a[0] = a[1] = a[3] = 0
    a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    a[4] = a[5] = termios.B115200
    a[6][termios.VMIN] = a[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, a)
    termios.tcflush(fd, termios.TCIOFLUSH)
    results = []
    results.append(check('fragmented', [(b'PING ', .05), (b'A0000001\n', 0)], b'PONG A0000001\n'))
    results.append(check('back_to_back', [(b'PING A0000002\nPING A0000003\n', 0)], b'PONG A0000002\nPONG A0000003\n'))
    burst = [('%08X' % (0xB0000000+i)).encode() for i in range(64)]
    results.append(check('burst_64', [(b''.join(b'PING '+v+b'\n' for v in burst), 0)],
                         b''.join(b'PONG '+v+b'\n' for v in burst)))
    results.append(check('unknown_command', [(b'BAD\n', 0)], b'ERR\n'))
    results.append(check('invalid_hex', [(b'PING Z0000004\n', 0)], b'ERR\n'))
    results.append(check('oversized_discard', [(b'X'*40+b'\n', 0)], b''))
    results.append(check('binary_discard', [(b'PING \x00BAD\n', 0)], b''))
    results.append(check('partial_timeout_recovery', [(b'PING ', .3), (b'PING A0000005\n', 0)], b'PONG A0000005\n'))
    results.append(check('final_recovery', [(b'PING A0000006\n', 0)], b'PONG A0000006\n'))
    print(json.dumps(dict(passed=sum(results), total=len(results))), flush=True)
    raise SystemExit(0 if all(results) else 1)
finally:
    os.close(fd)
