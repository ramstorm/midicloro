MIDIcloro - MIDI clock generator and router

By David Ramström

INTRODUCTION
MIDIcloro is a 4-to-1 MIDI merger which adds MIDI clock, polyphonic chords and routing of channels to class-compliant USB MIDI devices. The clock, chord and routing settings can be controlled in real-time via MIDI CC. MIDIcloro is a Linux console application intended to be used on the Raspberry Pi/Raspbian to provide a small hardware solution, entirely controlled via MIDI, for connecting and improving the capabilities of USB MIDI devices.


EXAMPLE USE-CASES:
Game Boy as a sequencer with clock - The Nintendo Game Boy can be used as a MIDI sequencer by running LSDJ in MIDI out mode together with an ArduinoBoy. No MIDI clock is generated however, making it hard to sync with e.g. a drum machine. Add a Raspberry Pi running MIDIcloro and a USB MIDI interface to your rig to generate MIDI clock controllable via MIDI CC from the Game Boy. Also, all 16 MIDI channels and simple polyphonic chords can now be used in your LSDJ songs. See detailed instructions below on how to use MIDIcloro with your Game Boy.

Improved sequencer - Add clock, chords and the capability to switch channels using MIDI CC to a simple MIDI sequencer. The sequencer can be connected via USB if available, or via DIN MIDI by using a USB MIDI interface.

USB to DIN MIDI - Use MIDIcloro with USB MIDI devices as inputs and a MIDI interface as output to merge and convert MIDI data from USB to DIN MIDI (if this functionality is all you need, consider the ALSA aconnect utility for a more light-weight solution).

Clock only - Use a Raspberry Pi running MIDIcloro as a master clock with a fixed tempo, or use a USB MIDI controller as a single input to set the tempo with a knob.


MIDI CLOCK
MIDI clock messages are sent immediately after starting MIDIcloro. An incoming start message resets the clock and the start message is forwarded to the output. Stop messages are also forwarded to the output. Clock signals are sent even after a stop message is received, so that connected equipment relying on MIDI clock can still be played using a keyboard. Any incoming clock messages are filtered out. If MIDI clock is disabled, no clock messages are generated and all incoming start, stop and clock messages are forwarded to the output.

There are two ways of setting the clock tempo. It can be set directly using the configured tempo MIDI CC message: new tempo = configured offset + MIDI CC value (0-127). The tempo can also be set by tapping the tempo MIDI CC message every beat. The first tap will set the tempo directly, and the each of the following taps will recalculate and set the tempo according to the tap interval.


CHORD MODE
Each device and MIDI channel has its individual chord mode setting. By setting a chord mode, every incoming note will generate other notes, creating a chord. Chord mode is disabled by default, but can be enabled using the configured chord mode MIDI CC message. Available modes (with CC value): OFF (0-7), MINOR (8-15), MAJOR (16-23), MINOR_LOW (24-31), MAJOR_LOW (32-39). Other CC values turn the chord mode off. The LOW modes transpose the highest note in the chord one octave down. More chord modes may be added in the future.


CHANNEL ROUTING
Each device and MIDI channel also has its own routing setting. By changing the channel routing, the current channel will be changed to another of the 16 MIDI channels. Notes, CC messages and other MIDI data with channel will be routed to the new target channel. The routing is set to the current channel by default, and is controlled via the configured routing MIDI CC message. The MIDI CC value range (0-127) is divided into steps of 8, where 0-7 sets channel 1, 8-15 channel 2 and so on.


INSTALLATION
Install dependencies:
sudo apt-get update
sudo apt-get install libasound2-dev
sudo apt-get install libboost-system-dev
sudo apt-get install libboost-program-options-dev
sudo apt-get install libboost-regex-dev

Download the latest binary and make it executable:
wget https://github.com/ledfyr/midicloro/releases/download/v1.2/midicloro
chmod +x midicloro

Connect your devices and start the interactive configuration:
./midicloro -c
Available input and output ports will be listed and you will be prompted to select which ones to use. You will then be prompted for the rest of the parameters listed below. When the configuration is completed, MIDIcloro will start.

Run MIDIcloro with the current settings:
./midicloro

Automatically start midicloro on boot:
Download the latest version of the auto-start script startm.sh and make it executable.
wget https://github.com/ledfyr/midicloro/releases/download/v1.2/startm.sh
chmod +x startm.sh

Configure midicloro with the inputs/output you wish to use and verify that it works.
In the file /etc/rc.local, add a call to the startm.sh script with the path to midicloro as parameter. Place it before the last exit command. IMPORTANT - add a '&' to let startm.sh run as a background process, otherwise your system will hang on boot.

Example (midicloro and startm.sh are downloaded to /home/pi):
sudo nano /etc/rc.local
Add the following line above "exit 0" in the bottom of the file. Do not forget the '&'.
/home/pi/startm.sh /home/pi &

MIDIcloro will now start when booting up, without the need of logging in (this makes it possible to run MIDIcloro on a headless Raspberry Pi).

If the program does not start automatically on boot, try to run it as:
sudo ./midicloro
If you get errors regarding missing libraries, make sure that the libasound2-dev and libboost 1.55.0 libraries exist in /usr/lib/.
The startm.sh script will print debug information to a file named startm.log placed in the directory you used as parameter to startm.sh. See startm.log for help solving the problem.


SETTINGS
The settings are stored in midicloro.cfg. The interactive configuration (see above) is recommended the first time you run MIDIcloro since it detects all connected MIDI devices. The configuration file midicloro.cfg can also be edited manually (a restart of MIDIcloro is needed for the changes to take effect). All parameters are displayed below with default values (and explanations) where applicable:

input1 =
input2 =
input3 =
input4 =
output =
enableClock = true (enable or disable clock)
ignoreProgramChanges = true (ignore or allow incoming program change MIDI messages)
initialBpm = 142 (this is the clock tempo used when starting MIDIcloro)
tapTempoMinBpm = 80 (lower limit for tempoMidiCC tapping - the tempo will be set using the tempoMidiCC value for taps slower than this)
tapTempoMaxBpm = 200 (upper limit for tempoMidiCC tapping - the tempo will be set using the tempoMidiCC value for taps faster than this)
bpmOffsetForMidiCC = 70 (this offset is added to the tempoMidiCC value to set the tempo)
tempoMidiCC = 10 (MIDI CC number for setting the tempo)
chordMidiCC = 11 (MIDI CC number for setting the chord mode)
routeMidiCC = 12 (MIDI CC number for setting the channel routing)


BUILD AND COMPILE
Install the dependencies (see INSTALLATION). Also make sure gcc/g++ is installed.

Compile MIDIcloro:
g++ -Wall -D__LINUX_ALSA__ -o midicloro midicloro.cpp rtmidi/RtMidi.cpp -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG -lasound -lpthread -lboost_system -lboost_program_options -lboost_regex


GAME BOY USAGE
To use MIDIcloro with the Nintendo Game Boy, you need the following: Game Boy, flash cart with LSDJ, Arduinoboy, Raspberry Pi, USB MIDI interface.
Start by installing MIDIcloro on the Raspberry Pi by following the instructions above. Connect the Game Boy to the Arduinoboy and connect the MIDI out port on the Arduinoboy to a MIDI in port on the USB MIDI interface (this port must be configured as an input in MIDIcloro). Set LSDJ and the Arduinoboy to MIDI out mode and start MIDIcloro on the Raspberry Pi.

To control the clock tempo, chord mode and channel routing, send MIDI CC messages from LSDJ using the X command.
The high byte of the X command sets the CC number as follows (high byte value = CC number): 0=1,1=2,2=3,3=7,4=10,5=11,6=12.
The low byte of the X command sets the CC value from 0 - 120 in steps of 8 (low byte value = CC value): 0=0,1=8,2=16, ... ,E=112,F=120.
The default CC numbers are 10 (X4_) for clock tempo, 11 (X5_) for chord mode and 12 (X6_) for channel routing. The default clock tempo offset is 70.

Examples (using default values):

X4A - If sent once: sets clock tempo to 150 (offset 70 + value 80 = 150).
X4A - If sent every beat: sets clock tempo to 150 the first time and calculates the tempo according to the interval between the messages the following times.

X51 - Every note sent on the current channel will generate three MIDI notes creating a minor chord.
X52 - Same as above but generating a major chord.
X53 - Same as above but generating a minor-low chord.
X54 - Same as above but generating a major-low chord.
X50 - Disables the chord mode.

X60 - Route MIDI data sent on the current channel to MIDI channel 1.
X64 - Route MIDI data sent on the current channel to MIDI channel 5.
X6F - Route MIDI data sent on the current channel to MIDI channel 16.


LIST OF SUPPORTED USB MIDI DEVICES
Rule of thumb: devices stated to be class compliant or known to be working in Linux will probably work with MIDIcloro on a Raspberry Pi.
Please contact me if you find a working/non-working device not listed here and I will update the list.

Known working devices:
IK Multimedia iRig Keys
E-MU XMidi 1X1

Known problematic devices (not working out-of-the-box):
M-Audio Midisport Uno


OTHER INFO
MIDIcloro is built and tested on a Raspberry Pi Model B+ running Raspbian. It may work without modifications on other Linux/Unix systems with ALSA support. MIDIcloro can also probably be built from source for Mac OS X and Windows without too much effort (since Boost and RtMidi are cross-platform software).

MIDIcloro uses RtMidi to handle the MIDI communication. Many thanks go to the author of RtMidi, Gary P. Scavone, for creating this great MIDI API. See rtmidi/readme for license and other information regarding RtMidi.


CONTACT
Ledfyr at chipmusic.org
david.ramstrom (at) gmail _dot_ com


LICENSE
MIDIcloro is licensed under the MIT license, see LICENSE.txt for details.

