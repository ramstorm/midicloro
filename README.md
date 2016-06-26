# MIDIcloro - MIDI clock generator and router
By David Ramström

## Introduction
MIDIcloro turns a Raspberry Pi into a little box of MIDI handling goodness! This is what it does:
* Listens to MIDI data from up to 4 USB input devices (e.g. sequencers, keyboards, interfaces) and merges it into 1 output.
* Sends MIDI clock (tempo is controlled via MIDI CC).
* Adds effects to MIDI notes (polyphonic chords, velocity and changing of channels - all controlled via MIDI CC).
* Runs stand-alone on the Raspberry Pi. No need for a mouse, keyboard or monitor. Auto start script is included.
* Plain old MIDI connectors (5-pin DIN) are supported by using a USB MIDI interface.


## Example use-cases
* **Sequencer/keyboard rig**: Add clock, chords, velocity and channel routing to the data sent from a MIDI sequencer. The settings are controlled via knobs on the sequencer. Connect a USB MIDI keyboard as well to mix notes from the sequencer with some live jamming on the keys.
* **Gameboy as sequencer**: Same scenario as above, but using a Gameboy + Arduinoboy as MIDI sequencer.
  * Use LSDJ MIDI out mode and connect Gameboy - Arduinoboy - MIDIcloro input port.
  * Clock and other functions can easily be controlled from LSDJ. See *Gameboy examples*.
  * Improve note stability by using a special Arduinoboy version together with MIDIcloro. See *Mono mode*.
* **Clock only**: Use MIDIcloro as a master clock. You can connect a MIDI controller to set the tempo with a knob.


## Settings
The config file below gives a good overview of the functions of MIDIcloro. The file is called *midicloro.cfg* and is created when running the initial configuration `./midicloro -c` (see *Initial configuration*).

```
input1 = (inputs and output are detected automatically when running the configuration)
input1mono = true (mono mode: when notes overlap, note off is sent leaving only the last note playing)
input2 =
input3 =
output =
enableClock = true (enable or disable clock)
ignoreProgramChanges = true (ignore or allow incoming program change messages)
initialBpm = 142 (this is the clock tempo used when starting MIDIcloro)
tapTempoMinBpm = 80 (lower limit for tempoMidiCC tapping)
tapTempoMaxBpm = 200 (upper limit for tempoMidiCC tapping)
bpmOffsetForMidiCC = 70 (this offset is added to the tempoMidiCC value to set the tempo)
velocityRandomOffset = -40 (in random velocity mode, notes get a random velocity between the velocityMidiCC value and this offset - set to 0 to get random velocity between 0-127)
velocityMultiDeviceCtrl = true (mirrors the velocity setting of the current input to inputs with lower number)
velocityMidiCC = 7 (MIDI CC number for setting the velocity mode)
tempoMidiCC = 10 (MIDI CC number for setting the tempo)
chordMidiCC = 11 (MIDI CC number for setting the chord mode)
routeMidiCC = 12 (MIDI CC number for setting the channel routing)
```


## MIDI clock
There are two ways of setting the clock tempo:
1. Send a single *tempo MIDI CC* message on one of the inputs: new tempo = configured offset + MIDI CC value (0-127).
2. Tap the *tempo MIDI CC* message every beat. The first tap will set the tempo instantly, and the each of the following taps will recalculate and set the tempo according to the tap interval.


## Chord mode
Each device and MIDI channel has its individual chord mode setting. By setting a chord mode, every incoming note will generate other notes, creating a chord. Chord mode is set via the *chord mode MIDI CC*. The CC value range 0-127 is divided into intervals of 8 to set the following modes:
* OFF
* MINOR3
* MAJOR3
* MINOR3_LO
* MAJOR3_LO
* MINOR2
* MAJOR2
* M7
* MAJ7
* M9
* MAJ9
* SUS4
* POWER2
* POWER3
* OCTAVE2
* OCTAVE3


## Channel routing
Each device and MIDI channel has its own routing setting. Set channel routing via the *routing MIDI CC*. The MIDI CC value range (0-127) is divided into intervals of 8, where 0-7 sets channel 1, 8-15 channel 2 and so on. By changing the channel routing, the current channel is changed to another of the 16 MIDI channels. Notes, CC messages and other MIDI data with channel will be routed to the new target channel.


## Velocity mode
Each device and MIDI channel has a velocity setting set via the *velocity MIDI CC* as follows:
* CC value=0 => OFF (velocity from the input device is unchanged)
* CC value=1-126 (all notes get velocity=value)
* CC value=127 (toggles random velocity mode on/off)

**Random velocity mode**: Send CC value=127 once to activate, and again to disable. When active, all notes get a random velocity between the CC value and the *velocityRandomOffset*.
Velocity is scaled to squeeze the whole 0-127 range in between CC values 8-120.
The velocity setting is mirrored to all other input ports with a number lower than the current port. The mirroring can be turned off in the settings by setting *velocityMultiDeviceCtrl* to false.

**Gameboy users**: To enable random velocity mode, you need to send a CC value of 127 from the Gameboy/Arduinoboy. The Arduinoboy sends CC values of maximum 120, and not 127 by default. A solution is to use the custom Arduinoboy software described in *Mono mode* - it scales CC values to span the whole range 0-127 (the scaling is done in such a way to still work as expected when controlling MIDIcloro via CC as in *Gameboy examples*). There might also exist a setting in the original Arduinoboy software to scale CC values to reach 127, or it can be modified to allow that.


## Mono mode
By enabling mono mode for an input port (see *Settings*) only 1 note can play at the same time. MIDIcloro sends note-off when notes overlap.
* Retrig is the default mono mode behavior (note-off first, then note-on).
* Legato (note-on first, then note-off for the old note) can be enabled by sending *chord mode MIDI CC* value 0-7 when chord mode already is OFF (another value 0-7 toggles back to retrig). The *chord mode CC* is used here to spare another CC from being occupied by MIDIcloro.

**Improved Gameboy and Arduinoboy note stability**: Mono mode together with a custom version of the Arduinoboy software can solve problems with missing notes which you might notice when sending MIDI on multiple channels from the Gameboy.
* Upload this to your Arduinoboy: https://github.com/ledfyr/ab-midiout-lite
* Enable mono mode on the MIDIcloro input port connected to the Arduinoboy.
* Note stability is improved mainly by relieving the Arduino from sending note-off when notes overlap. Other modes and LED flashing etc are also stripped away.


## Gameboy examples

MIDIcloro is controlled from LSDJ via CC (the X command). Examples:

```
X32 - Activates velocity mode: all MIDI notes on the current channel will get a low velocity.
X36 - Increases the velocity.
X3F - Enables random velocity mode: every note gets a random velocity between the last velocity setting (X36 above) and the configured offset (-40 by default).
X3D - High velocity, random velocity mode still active.
X31 - Low velocity, random velocity mode still active.
X3F - Disables random velocity mode.
X30 - Disables velocity mode: velocity is not changed by MIDIcloro anymore (velocity from the Gameboy/Arduinoboy is 100 by default).

X4A - If sent once: sets clock tempo to 150 (offset 70 + value 80 = 150).
X4A - If sent for e.g. 4 beats in a row (RECOMMENDED): sets clock tempo to 150 the first time and calculates the tempo according to the interval between the messages the following times.

X51 - Every note sent on the current channel will generate three MIDI notes creating a minor chord.
X52 - Same as above but generating a major chord.
X53 - Same as above but generating a minor-low chord.
X54 - Same as above but generating a major-low chord.
X50 - Disables the chord mode.

X60 - Route MIDI data sent on the current channel to MIDI channel 1.
X64 - Route MIDI data sent on the current channel to MIDI channel 5.
X6F - Route MIDI data sent on the current channel to MIDI channel 16.
```


## Installation
Prepare Raspberry Pi:
* Flash a *Lite* version of Raspbian to an SD card (no desktop or GUI functions are needed).
* Start the Raspberry Pi and log in as the default user "pi" (use SSH or monitor+keyboard).


Install dependencies:

```
sudo apt-get install libasound2-dev
sudo apt-get install libboost-system-dev
sudo apt-get install libboost-program-options-dev
sudo apt-get install libboost-regex-dev
```

Download the MIDIcloro binary and make it executable:

```
wget https://github.com/ledfyr/midicloro/releases/download/v1.5/midicloro
chmod +x midicloro
```


## Initial configuration
Connect your devices and start the configuration:

`./midicloro -c`

Available input and output ports will be listed and you will be prompted to select which ones to use. You will then be prompted for the rest of the parameters listed under *Settings*.

When the configuration is finished, a config file is created (see *Settings*). This file can be edited manually (e.g. using nano). Changes take effect when restarting MIDIcloro.

It is also possible to change settings by running the configuration again using `./midicloro -c`. However, if the MIDIcloro process is running in the background, shut it down before running the configuration. Find the process ID with `ps aux | grep midicloro` and shut it down with `sudo kill process_id_goes_here`.


## Run
To run MIDIcloro with the current settings:

`./midicloro`


## Auto start
Follow these instructions if you want to start MIDIcloro automatically when the Raspberry Pi starts up.

Download the auto-start script and make it executable:

```
wget https://github.com/ledfyr/midicloro/releases/download/v1.5/startm.sh
chmod +x startm.sh
```

If you haven't done so already, configure MIDIcloro with the inputs/output and settings you want to use and verify that it works (see *Initial configuration* and *Run*).

In the file `/etc/rc.local`, add a call to the startm.sh script with the path to midicloro as parameter. Place it before the last exit command. IMPORTANT - add a `&` to let startm.sh run as a background process, otherwise your system may hang on boot.

Example (midicloro and startm.sh are downloaded to /home/pi):

`sudo nano /etc/rc.local`

Add the following line above `exit 0` in the bottom of the file. Do not forget the `&`.

`/home/pi/startm.sh /home/pi &`

MIDIcloro will now start when booting up, without the need of logging in. You are now running MIDIcloro stand-alone!

If the program fails to start automatically, try to run it as:

`sudo ./midicloro`

If you get errors regarding missing libraries, make sure that the `libasound2-dev` and `libboost` libraries exist in `/usr/lib/`.


## Build and compile
Install the dependencies (see *Installation*). Also make sure g++ is installed.

Compile MIDIcloro with `make` or the following command:

`g++ -Wall -D__LINUX_ALSA__ -o midicloro midicloro.cpp rtmidi/RtMidi.cpp -lasound -lpthread -lboost_system -lboost_program_options -lboost_regex`


## Supported USB MIDI devices
Any class compliant device should work. Please contact me if you find any working/non-working device not listed here and I will update the list.

Known working devices:
* IK Multimedia iRig Keys
* E-MU XMidi 1X1

Known problematic devices (not working out-of-the-box):
* M-Audio Midisport Uno


## Other info
MIDIcloro is built and tested on a Raspberry Pi 1 Model B+ running Raspbian Jessie Lite.

MIDIcloro uses RtMidi to handle the MIDI communication. Many thanks to Gary P. Scavone for creating this great framework. See rtmidi/readme for license and other info regarding RtMidi.


## Contact
Ledfyr at chipmusic.org

david.ramstrom (at) gmail \_dot\_ com


## License
MIDIcloro is licensed under the MIT license, see the LICENSE file for details.

