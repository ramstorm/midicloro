import time
import rtmidi
import logging
import sys
import threading
import mconfig
try:
    import Queue as queue
except ImportError:  # Python 3
    import queue
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
    def __init__(self, mono):
        self.modes = [Mode(x) for x in range(0, 16)]
        self.mono = mono


class MidiDispatcher(threading.Thread):
    def __init__(self, midiin, midiout):
        super(MidiDispatcher, self).__init__()
        self.midiin = midiin
        self.midiout = midiout
        self._wallclock = time.time()
        self.queue = queue.Queue()

    def __call__(self, event, data=None):
        message, deltatime = event
        self._wallclock += deltatime
        log.debug("IN: @%0.6f %r", self._wallclock, message)
        self.queue.put((message, self._wallclock))

    def run(self):
        log.debug("Attaching MIDI input callback handler.")
        self.midiin.set_callback(self)

        while True:
            event = self.queue.get()

            if event is None:
                break

            log.debug("Out: @%0.6f %r", event[1], event[0])
            self.midiout.send_message(event[0])

    def stop(self):
        self.queue.put(None)



def main(args=None):
    logging.basicConfig(format="%(name)s: %(levelname)s - %(message)s", level=logging.DEBUG)

    try:
        midiin, inport_name = open_midiport(mconfig.input1, "input")
        midiout, outport_name = open_midiport(mconfig.output, "output")
    except IOError as exc:
        print(exc)
        return 1
    except (EOFError, KeyboardInterrupt):
        return 0

    dispatcher = MidiDispatcher(midiin, midiout)

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

        midiin.close_port()
        midiout.close_port()

        del midiin
        del midiout

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]) or 0)

#midiin = rtmidi.MidiIn()
#inports = midiin.get_ports()

#midiin.open_port(mconfig.input1)

#print(inports[mconfig.input1])
#print(mconfig.input1)

#if available_ports:
#    midiout.open_port(0)
#else:
#    midiout.open_virtual_port("My virtual output")
#
#note_on = [0x90, 60, 112] # channel 1, middle C, velocity 112
#note_off = [0x80, 60, 0]
#midiout.send_message(note_on)
#time.sleep(0.5)
#midiout.send_message(note_off)

#del midiin

