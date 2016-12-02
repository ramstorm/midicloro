import time
import rtmidi
import logging
import sys
import threading
import mconfig
from rtmidi.midiutil import open_midiport


log = logging.getLogger("midicloro")

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


class MidiDispatcher(threading.Thread):
    def __init__(self, input_ports, midiout):
        super(MidiDispatcher, self).__init__()
        self.input_ports = input_ports
        self.midiout = midiout
        self._wallclock = time.time()
        self.running = True

    def run(self):

        while True:
            if not self.running:
                break
            for ip in self.input_ports:
                data = ip.midiin.get_message()
                if data is None:
                    continue

                msg, deltatime = data
                log.debug("Input%s: @%0.6f %r", str(ip.number), deltatime, msg)
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

    print("Entering main loop. Press Control-C to exit.")
    try:
        dispatcher.start()
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
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

