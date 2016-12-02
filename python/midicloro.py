import time
import rtmidi
import logging
import sys
import threading
import collections
import mconfig
from rtmidi.midiutil import open_midiport


log = logging.getLogger("midicloro")

send_clock = False
clock_interval = 60 / (mconfig.initial_bpm * 24)

class Mode():
    def __init__(self, channel):
        self.last_note = -1
        self.channel_routing = channel
        self.chord = 0
        self.velocity_mode = 0
        self.velocity = 100
        self.mono_legato = False

class InputPort():
    def __init__(self, midiin, number, mono):
        self.midiin = midiin
        self.number = number
        self.mono = mono
        self.modes = [Mode(x) for x in range(0, 16)]

class ClockTimer(threading.Thread):
    def __init__(self):
        super(ClockTimer, self).__init__()
        self.running = True

    def run(self):
        global send_clock
        while True:
            if not self.running:
                break
            send_clock = True
            time.sleep(clock_interval)

    def stop(self):
        global send_clock
        send_clock = False
        self.running = False


class MidiDispatcher(threading.Thread):
    def __init__(self, input_ports, midiout):
        super(MidiDispatcher, self).__init__()
        self.input_ports = input_ports
        self.midiout = midiout
        #self.last_clock = 0
        self.tap_tempo_times = collections.deque(maxlen=4)
        self.running = True

    def run(self):
        global send_clock
        while True:
            if not self.running:
                break
            if send_clock:
                self.midiout.send_message([248])
                send_clock = False
                #now = time.time()
                #log.debug("Clock: delta=%0.6f", now - self.last_clock)
                #self.last_clock = now
            for ip in self.input_ports:
                data = ip.midiin.get_message()
                if not data:
                    continue

                msg, deltatime = data
                log.debug("Input%s: @%0.6f %r", str(ip.number), deltatime, msg)

                # Tap-tempo
                if (msg[0] & 0b11110000) == 0b10110000 and msg[1] == mconfig.tempo_midi_cc:
                    log.debug("TAP")

                self.midiout.send_message(msg)

    def stop(self):
        self.running = False



def main(args=None):
    logging.basicConfig(format="%(name)s: %(levelname)s - %(message)s", level=logging.DEBUG)

    try:
        input_ports = []
        for ip in mconfig.inputs:
            midiin, inport_name = open_midiport(ip["name"], "input")
            input_ports.append(InputPort(midiin, ip["number"], ip["mono"]))

        midiout, outport_name = open_midiport(mconfig.output, "output")
    except IOError as exc:
        print(exc)
        return 1
    except (EOFError, KeyboardInterrupt):
        return 0

    dispatcher = MidiDispatcher(input_ports, midiout)
    clock = ClockTimer()

    print("Entering main loop. Press Control-C to exit.")
    try:
        dispatcher.start()
        clock.start()
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        clock.stop()
        clock.join()
        dispatcher.stop()
        dispatcher.join()
        print('')
    finally:
        print("Exit.")

        for ip in input_ports:
            ip.midiin.close_port()
            del ip.midiin

        midiout.close_port()
        del midiout

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]) or 0)

