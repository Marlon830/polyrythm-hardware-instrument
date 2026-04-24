#!/usr/bin/env python3

"""
Live ASCII waveform viewer.
Usage:
  chmod +x tools/ascii_wave_viewer.py
  ./TinyFirmware | ./tools/ascii_wave_viewer.py
"""
import sys
import re
import collections
import curses
import signal
import time

LINE_RE = re.compile(r':\s*([+-]?\d*\.?\d+(?:[eE][+-]?\d+)?)')

def draw(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(True)
    max_y, max_x = stdscr.getmaxyx()
    width = max_x - 1
    height = max_y - 4  # leave room for header/footer
    if height < 4:
        height = 4

    buffer = collections.deque(maxlen=width)
    # sliding min/max for autoscale
    min_v = -1.0
    max_v = 1.0
    last_update = time.time()

    def map_value_to_row(v):
        # clamp
        if max_v == min_v:
            return 0
        norm = (v - min_v) / (max_v - min_v)
        # invert so top row is max
        row = int((1.0 - norm) * (height - 1))
        return max(0, min(height - 1, row))

    # read stdin lines in non-blocking way by checking for available input
    # We'll block on readline (pipe provides data), but still update screen periodically.
    while True:
        try:
            line = sys.stdin.readline()
            if line == '' and sys.stdin.closed:
                break
            if line == '':
                # no data right now; update screen and continue
                time.sleep(0.01)
            else:
                m = LINE_RE.search(line)
                if m:
                    try:
                        v = float(m.group(1))
                        buffer.append(v)
                        # update min/max from buffer for autoscale
                        if len(buffer) > 0:
                            min_v = min(buffer)
                            max_v = max(buffer)
                            if min_v == max_v:
                                # small range fallback
                                min_v -= 0.5
                                max_v += 0.5
                    except Exception:
                        pass

            # redraw at most 30 fps
            now = time.time()
            if now - last_update < 1/30:
                continue
            last_update = now

            stdscr.erase()
            max_y, max_x = stdscr.getmaxyx()
            width = max_x - 1
            height = max_y - 4
            if height < 4:
                height = 4

            # draw header
            stdscr.addstr(0, 0, "Live Waveform (press Ctrl-C to quit)".ljust(max_x - 1))
            stdscr.addstr(1, 0, f"samples: {len(buffer):4d}  min:{min_v: .6f}  max:{max_v: .6f}".ljust(max_x - 1))

            # prepare rows
            rows = [[' ' for _ in range(width)] for _ in range(height)]
            # draw center line if 0 in range
            if min_v < 0 < max_v:
                zero_row = map_value_to_row(0.0)
                for x in range(width):
                    rows[zero_row][x] = '-'

            # plot samples from right to left
            start_x = max(0, width - len(buffer))
            for i, v in enumerate(buffer):
                x = start_x + i
                if x >= width:
                    break
                y = map_value_to_row(v)
                rows[y][x] = '*'

            # render rows
            for r in range(height):
                stdscr.addstr(2 + r, 0, ''.join(rows[r]))

            stdscr.refresh()

            # handle resize or keypress
            try:
                k = stdscr.getch()
                if k == ord('q'):
                    break
            except Exception:
                pass

        except KeyboardInterrupt:
            break
        except BrokenPipeError:
            break
        except Exception:
            # ignore transient errors, continue
            pass

def main():
    # handle SIGWINCH gracefully (resize)
    signal.signal(signal.SIGWINCH, lambda n, f: None)
    curses.wrapper(draw)

if __name__ == '__main__':
    main()
# ...existing code...