//************** MIDIcloro **************
//
// MIDI clock generator and router
//
// Copyright (c) 2015 David Ramstrom
// This project is licensed under the terms of the MIT license
//
// Run:
// ./midicloro [-c to start interactive configuration]
//
//***************************************

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sstream>
#include <string>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/utility/binary.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/random.hpp>
#include "rtmidi/RtMidi.h"

using namespace std;

namespace po = boost::program_options;
namespace convert {
    template<typename T> string to_string(const T& n) {
        ostringstream stm;
        stm << n;
        return stm.str();
    }
}

enum Chord {
  CHORD_OFF = 0,
  MINOR3,
  MAJOR3,
  MINOR3_LO,
  MAJOR3_LO,
  MINOR2,
  MAJOR2,
  M7,
  MAJ7,
  M9,
  MAJ9,
  SUS4,
  POWER2,
  POWER3,
  OCTAVE2,
  OCTAVE3
};

enum Velo {
  VEL_OFF,
  VEL_ON,
  VEL_RDM
};

RtMidiIn *midiin1 = 0;
RtMidiIn *midiin2 = 0;
RtMidiIn *midiin3 = 0;
RtMidiIn *midiin4 = 0;
RtMidiOut *midiout = 0;
bool enableClock;
bool ignoreProgramChanges;
int tempoMidiCC;
int chordMidiCC;
int routeMidiCC;
int velocityMidiCC;
int bpmOffsetForMidiCC;
long clockInterval; // Clock interval in ns
long tapTempoMinInterval; // Tap-tempo min interval in ns
long tapTempoMaxInterval; // Tap-tempo max interval in ns
int velocityRandomOffset;
bool velocityMultiDeviceCtrl;
boost::mt19937 *randomGenerator;
boost::asio::deadline_timer *clockTimer = 0;
vector<unsigned char> *clockMessage;
vector<unsigned char> *noteOffMessage;
int lastNote[4][16] = {{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
                       {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
                       {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
                       {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
int channelRouting[4][16] = {{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
                             {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
                             {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
                             {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}};
int chordModes[4][16] = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
int velocityModes[4][16] = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
int velocity[4][16] = {{100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100},
                       {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100},
                       {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100},
                       {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100}};
bool noteOffMode[4] = {false, false, false, false};
bool noteOffLegato[4][16] = {{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false},
                             {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false},
                             {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false},
                             {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}};
boost::circular_buffer<boost::posix_time::ptime> *tapTempoTimes;
const char *CONFIG_FILE = "midicloro.cfg";

void usage(void);
double random01();
bool ignoreMessage(unsigned char msgByte);
void transposeAndSend(vector<unsigned char> *message, int semiNotes);
void sendNoteOrChord(vector<unsigned char> *message, int source);
void sendNoteOffAndNote(vector<unsigned char> *message, int source);
void setChordMode(int source, int channel, int value);
void routeChannel(vector<unsigned char> *message, int source);
void setChannelRouting(int source, int channel, int newChannel);
void applyVelocity(vector<unsigned char> *message, int source);
void setVelocityMode(int source, int channel, int value);
void setVelocityModeMulti(int source, int channel, int value);
int scaleUp(int value);
long tapTempo();
void handleMessage(vector<unsigned char> *message, int source);
void messageAtIn1(double deltatime, vector<unsigned char> *message, void */*userData*/);
void messageAtIn2(double deltatime, vector<unsigned char> *message, void */*userData*/);
void messageAtIn3(double deltatime, vector<unsigned char> *message, void */*userData*/);
void messageAtIn4(double deltatime, vector<unsigned char> *message, void */*userData*/);
string trimPort(bool doTrim, const string& str);
bool openInputPort(RtMidiIn *in, string port);
bool openOutputPort(RtMidiIn *in, string port);
bool openPorts(string i1, string i2, string i3, string i4, string o);
void clock(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t);
void signalHandler(const boost::system::error_code& error, int signal_number);
void cleanUp();
void runInteractiveConfiguration();

int main(int argc, char *argv[]) {
  try {
    if (argc == 2 && string(argv[1]) == "-c")
      runInteractiveConfiguration();
    else if (argc > 1)
      usage();

    // Handle configuration
    string input1, input2, input3, input4, output;
    bool input1noteOffMode, input2noteOffMode, input3noteOffMode, input4noteOffMode;
    int initialBpm, tapTempoMinBpm, tapTempoMaxBpm;

    po::options_description desc("Options");
    desc.add_options()
      ("input1", po::value<string>(&input1), "input1")
      ("input1noteOffMode", po::value<bool>(&input1noteOffMode)->default_value(false), "input1noteOffMode")
      ("input2", po::value<string>(&input2), "input2")
      ("input2noteOffMode", po::value<bool>(&input2noteOffMode)->default_value(false), "input2noteOffMode")
      ("input3", po::value<string>(&input3), "input3")
      ("input3noteOffMode", po::value<bool>(&input3noteOffMode)->default_value(false), "input3noteOffMode")
      ("input4", po::value<string>(&input4), "input4")
      ("input4noteOffMode", po::value<bool>(&input4noteOffMode)->default_value(false), "input4noteOffMode")
      ("output", po::value<string>(&output), "output")
      ("enableClock", po::value<bool>(&enableClock)->default_value(true), "enableClock")
      ("ignoreProgramChanges", po::value<bool>(&ignoreProgramChanges)->default_value(true), "ignoreProgramChanges")
      ("initialBpm", po::value<int>(&initialBpm)->default_value(142), "initialBpm")
      ("tapTempoMinBpm", po::value<int>(&tapTempoMinBpm)->default_value(80), "tapTempoMinBpm")
      ("tapTempoMaxBpm", po::value<int>(&tapTempoMaxBpm)->default_value(200), "tapTempoMaxBpm")
      ("bpmOffsetForMidiCC", po::value<int>(&bpmOffsetForMidiCC)->default_value(70), "bpmOffsetForMidiCC")
      ("velocityRandomOffset", po::value<int>(&velocityRandomOffset)->default_value(-40), "velocityRandomOffset")
      ("velocityMultiDeviceCtrl", po::value<bool>(&velocityMultiDeviceCtrl)->default_value(true), "velocityMultiDeviceCtrl")
      ("tempoMidiCC", po::value<int>(&tempoMidiCC)->default_value(10), "tempoMidiCC")
      ("chordMidiCC", po::value<int>(&chordMidiCC)->default_value(11), "chordMidiCC")
      ("routeMidiCC", po::value<int>(&routeMidiCC)->default_value(12), "routeMidiCC")
      ("velocityMidiCC", po::value<int>(&velocityMidiCC)->default_value(7), "velocityMidiCC");
    po::variables_map vm;

    ifstream file(CONFIG_FILE);
    po::store(po::parse_config_file(file, desc), vm);
    po::notify(vm);
    file.close();

    clockInterval = 60000000000/(initialBpm*24);
    tapTempoMaxInterval = 60000000000/tapTempoMinBpm;
    tapTempoMinInterval = 60000000000/tapTempoMaxBpm;

    noteOffMode[0] = input1noteOffMode;
    noteOffMode[1] = input2noteOffMode;
    noteOffMode[2] = input3noteOffMode;
    noteOffMode[3] = input4noteOffMode;

    randomGenerator = new boost::mt19937(time(0));

    midiin1 = new RtMidiIn();
    midiin2 = new RtMidiIn();
    midiin3 = new RtMidiIn();
    midiin4 = new RtMidiIn();
    midiout = new RtMidiOut();

    // Assign MIDI ports
    if (!openPorts(input1, input2, input3, input4, output)) {
      cout << "Exiting" << endl;
      cleanUp();
      exit(0);
    }

    midiin1->setCallback(&messageAtIn1);
    midiin2->setCallback(&messageAtIn2);
    midiin3->setCallback(&messageAtIn3);
    midiin4->setCallback(&messageAtIn4);

    // Don't ignore sysex, timing, or active sensing messages
    midiin1->ignoreTypes(false, false, false);
    midiin2->ignoreTypes(false, false, false);
    midiin3->ignoreTypes(false, false, false);
    midiin4->ignoreTypes(false, false, false);

    // Note off message
    vector<unsigned char> offMsg;
    offMsg.push_back(BOOST_BINARY(10000000));
    offMsg.push_back(42);
    offMsg.push_back(100);
    noteOffMessage = &offMsg;

    // Clock message
    vector<unsigned char> clkMsg;
    clkMsg.push_back(BOOST_BINARY(11111000));
    clockMessage = &clkMsg;

    // Tap-tempo
    boost::circular_buffer<boost::posix_time::ptime> taps(4);
    taps.push_front(boost::posix_time::microsec_clock::local_time());
    tapTempoTimes = &taps;

    cout << "Starting" << endl;
    if (enableClock) {
      // Start recurring timer generating MIDI clock
      cout << "Press ctrl+c to exit" << endl;
      boost::asio::io_service io;
      boost::asio::deadline_timer t(io, boost::posix_time::nanoseconds(clockInterval));
      t.async_wait(boost::bind(clock, boost::asio::placeholders::error, &t));
      clockTimer = &t; 

      // Register signals for process termination
      boost::asio::signal_set signals(io, SIGINT, SIGTERM);
      signals.async_wait(signalHandler);

      io.run();
    }

    cout << "Press enter to exit" << endl;
    char exitIn;
    cin.get(exitIn);
  }
  catch (RtMidiError &error) {
    error.printMessage();
  }
  catch (exception& e) {
    cout << "Error occurred while reading configuration: " << e.what() << endl;
    return 1;
  }

  cleanUp();
  return 0;
}

void usage(void) {
  cout << "Usage: ./midicloro [-c to start interactive configuration]" << endl;
  exit(0);
}

double random01() {
  static boost::uniform_01<boost::mt19937> dist(*randomGenerator);
  return dist();
}

bool ignoreMessage(unsigned char msgByte) {
  if ((enableClock && (msgByte == BOOST_BINARY(11111000))) || // MIDI clock
      (ignoreProgramChanges && ((msgByte & BOOST_BINARY(11110000)) == BOOST_BINARY(11000000)))) // Program change
    return true;
  return false;
}

void transposeAndSend(vector<unsigned char> *message, int semiNotes) {
  // Verify that the note will end up withing the permitted range
  int note = (int)(*message)[1] + semiNotes;
  if (note >= 0 && note <= 127){
    // This changes the message - keep in mind for the next note in the chord
    (*message)[1] = note;
    midiout->sendMessage(message);
  }
}

void sendNoteOrChord(vector<unsigned char> *message, int source) {
  int channel = (int)((*message)[0] & BOOST_BINARY(00001111));
  // Handle chord mode
  switch(chordModes[source][channel]) {
    case CHORD_OFF:
      midiout->sendMessage(message);
      break;
    case MINOR3:
      midiout->sendMessage(message);
      transposeAndSend(message, 3);
      transposeAndSend(message, 4);
      break;
    case MAJOR3:
      midiout->sendMessage(message);
      transposeAndSend(message, 4);
      transposeAndSend(message, 3);
      break;
    case MINOR3_LO:
      transposeAndSend(message, -5);
      transposeAndSend(message, 5);
      transposeAndSend(message, 3);
      break;
    case MAJOR3_LO:
      transposeAndSend(message, -5);
      transposeAndSend(message, 5);
      transposeAndSend(message, 4);
      break;
    case MINOR2:
      midiout->sendMessage(message);
      transposeAndSend(message, 3);
      break;
    case MAJOR2:
      midiout->sendMessage(message);
      transposeAndSend(message, 4);
      break;
    case M7:
      midiout->sendMessage(message);
      transposeAndSend(message, 3);
      transposeAndSend(message, 4);
      transposeAndSend(message, 3);
      break;
    case MAJ7:
      midiout->sendMessage(message);
      transposeAndSend(message, 4);
      transposeAndSend(message, 3);
      transposeAndSend(message, 4);
      break;
    case M9:
      midiout->sendMessage(message);
      transposeAndSend(message, 3);
      transposeAndSend(message, 4);
      transposeAndSend(message, 3);
      transposeAndSend(message, 4);
      break;
    case MAJ9:
      midiout->sendMessage(message);
      transposeAndSend(message, 4);
      transposeAndSend(message, 3);
      transposeAndSend(message, 4);
      transposeAndSend(message, 3);
      break;
    case SUS4:
      midiout->sendMessage(message);
      transposeAndSend(message, 5);
      transposeAndSend(message, 2);
      break;
    case POWER2:
      midiout->sendMessage(message);
      transposeAndSend(message, 7);
      break;
    case POWER3:
      midiout->sendMessage(message);
      transposeAndSend(message, 7);
      transposeAndSend(message, 5);
      break;
    case OCTAVE2:
      midiout->sendMessage(message);
      transposeAndSend(message, 12);
      break;
    case OCTAVE3:
      midiout->sendMessage(message);
      transposeAndSend(message, 12);
      transposeAndSend(message, 12);
      break;
    default:
      midiout->sendMessage(message);
      break;
  }
}

void sendNoteOffAndNote(vector<unsigned char> *message, int source) {
  int channel = (int)((*message)[0] & BOOST_BINARY(00001111));
  bool thisIsNoteOn = ((*message)[0] & BOOST_BINARY(10010000)) == BOOST_BINARY(10010000);
  if (!noteOffLegato[source][channel]) {
    if (thisIsNoteOn && lastNote[source][channel] != -1) {
      (*noteOffMessage)[0] = 128 + channel;
      (*noteOffMessage)[1] = lastNote[source][channel];
      sendNoteOrChord(noteOffMessage, source);
    }
    lastNote[source][channel] = thisIsNoteOn ? (*message)[1] : -1;
    sendNoteOrChord(message, source);
  }
  else {
    unsigned char currNote = (*message)[1];
    sendNoteOrChord(message, source);
    if (thisIsNoteOn && lastNote[source][channel] != -1) {
      (*noteOffMessage)[0] = 128 + channel;
      (*noteOffMessage)[1] = lastNote[source][channel];
      sendNoteOrChord(noteOffMessage, source);
    }
    lastNote[source][channel] = thisIsNoteOn ? currNote : -1;
  }
}


void setChordMode(int source, int channel, int value) {
  if (chordModes[source][channel] == 0 && value == 0)
    noteOffLegato[source][channel] = !noteOffLegato[source][channel];

  chordModes[source][channel] = value/8;
}

void routeChannel(vector<unsigned char> *message, int source) {
  int channel = (int)((*message)[0] & BOOST_BINARY(00001111));
  (*message)[0] = ((*message)[0] & BOOST_BINARY(11110000)) + channelRouting[source][channel];
}

void setChannelRouting(int source, int channel, int newChannel) {
  if (newChannel >= 0 && newChannel <= 127)
    channelRouting[source][channel] = newChannel/8;
}

void applyVelocity(vector<unsigned char> *message, int source) {
  int channel = (int)((*message)[0] & BOOST_BINARY(00001111));
  if (velocityModes[source][channel] == VEL_OFF || message->size() < 3)
    return;

  if (velocityModes[source][channel] == VEL_RDM) {
    if (velocityRandomOffset < 0)
      (*message)[2] = max(velocity[source][channel]+(int)(velocityRandomOffset*random01()), 0);
    else if (velocityRandomOffset > 0)
      (*message)[2] = min(velocity[source][channel]+(int)(velocityRandomOffset*random01()), 127);
    else
      (*message)[2] = (int)(random01()*127);
  }
  else {
    (*message)[2] = velocity[source][channel];
  }
}

void setVelocityMode(int source, int channel, int value) {
  if (value == 127) {
    velocityModes[source][channel] = (velocityModes[source][channel] == VEL_RDM) ? VEL_ON : VEL_RDM;
  }
  else if (value == 0) {
    velocityModes[source][channel] = VEL_OFF;
  }
  else {
    value = scaleUp(value);
    velocity[source][channel] = value;
    if (velocityModes[source][channel] == VEL_OFF)
      velocityModes[source][channel] = VEL_ON;
  }
}

void setVelocityModeMulti(int source, int channel, int value) {
  if (value == 127) {
    int newMode = (velocityModes[source][channel] == VEL_RDM) ? VEL_ON : VEL_RDM;
    for (int i=source; i>=0; i--)
      velocityModes[i][channel] = newMode;
  }
  else if (value == 0) {
    for (int i=source; i>=0; i--)
      velocityModes[i][channel] = VEL_OFF;
  }
  else {
    value = scaleUp(value);
    for (int i=source; i>=0; i--)
      velocity[i][channel] = value;
    if (velocityModes[source][channel] == VEL_OFF)
      for (int i=source; i>=0; i--)
        velocityModes[i][channel] = VEL_ON;
  }
}

int scaleUp(int value) {
  // Scale value to let 8-120 contain the whole range 0-127
  if (value > 64) {
    value += 8*(value - 64)/56;
    value = min(value, 127);
  }
  else if (value < 64) {
    value -= 8*(64 - value)/56;
    value = max(value, 0);
  }
  return value;
}

long tapTempo() {
  long diff = 0;
  long accumulatedDiffs = 0;
  unsigned int i = 0;
  tapTempoTimes->push_front(boost::posix_time::microsec_clock::local_time());
  do {
    boost::posix_time::time_duration td = (*tapTempoTimes)[i] - (*tapTempoTimes)[i+1];
    diff = td.total_nanoseconds();
    accumulatedDiffs += diff;
    i++;
  }
  while (diff >= tapTempoMinInterval && diff <= tapTempoMaxInterval && i < tapTempoTimes->size()-1);
  if (i > 1)
    return accumulatedDiffs/i; // Interval in ns
  else
    return 0;
}

void handleMessage(vector<unsigned char> *message, int source) {
  // Note-off mode: send note off for last note
  if (noteOffMode[source] && ((*message)[0] & BOOST_BINARY(11100000)) == BOOST_BINARY(10000000)) {
    routeChannel(message, source);
    applyVelocity(message, source);
    sendNoteOffAndNote(message, source);
  }
  // Note on/off: send note or chord
  else if (((*message)[0] & BOOST_BINARY(11100000)) == BOOST_BINARY(10000000)) {
    routeChannel(message, source);
    applyVelocity(message, source);
    sendNoteOrChord(message, source);
  }
  // Start message: pass it through and reset clock
  else if (enableClock && ((*message)[0] == BOOST_BINARY(11111010))) {
    midiout->sendMessage(message);
    clockTimer->cancel();
  }
  // Tap-tempo MIDI CC: use tap-tempo or tempo from MIDI message
  else if (((*message)[0] & BOOST_BINARY(11110000)) == BOOST_BINARY(10110000) && message->size() > 2 && (*message)[1] == tempoMidiCC) {
    long tapInterval = tapTempo();
    if (tapInterval != 0)
      clockInterval = tapInterval/24;
    else
      clockInterval = 60000000000/((bpmOffsetForMidiCC+(*message)[2])*24);

    clockTimer->cancel();
  }
  // Chord mode MIDI CC: set chord mode
  else if (((*message)[0] & BOOST_BINARY(11110000)) == BOOST_BINARY(10110000) && message->size() > 2 && (*message)[1] == chordMidiCC) {
    routeChannel(message, source);
    setChordMode(source, (*message)[0] & BOOST_BINARY(00001111), (*message)[2]);
  }
  // Channel routing MIDI CC: set channel routing
  else if (((*message)[0] & BOOST_BINARY(11110000)) == BOOST_BINARY(10110000) && message->size() > 2 && (*message)[1] == routeMidiCC) {
    setChannelRouting(source, (*message)[0] & BOOST_BINARY(00001111), (*message)[2]);
  }
  // Velocity MIDI CC: set velocity mode
  else if (((*message)[0] & BOOST_BINARY(11110000)) == BOOST_BINARY(10110000) && message->size() > 2 && (*message)[1] == velocityMidiCC) {
    routeChannel(message, source);
    if (velocityMultiDeviceCtrl)
      setVelocityModeMulti(source, (*message)[0] & BOOST_BINARY(00001111), (*message)[2]);
    else
      setVelocityMode(source, (*message)[0] & BOOST_BINARY(00001111), (*message)[2]);
  }
  // Other MIDI messages
  else if (!ignoreMessage((*message)[0])) {
    if ((((*message)[0] & BOOST_BINARY(11110000)) >= BOOST_BINARY(10000000)) &&
        (((*message)[0] & BOOST_BINARY(11110000)) <= BOOST_BINARY(11100000))) {
      routeChannel(message, source);
    }
    midiout->sendMessage(message);
  }
}

void messageAtIn1(double deltatime, vector<unsigned char> *message, void */*userData*/) {
  handleMessage(message, 0);
}

void messageAtIn2(double deltatime, vector<unsigned char> *message, void */*userData*/) {
  handleMessage(message, 1);
}

void messageAtIn3(double deltatime, vector<unsigned char> *message, void */*userData*/) {
  handleMessage(message, 2);
}

void messageAtIn4(double deltatime, vector<unsigned char> *message, void */*userData*/) {
  handleMessage(message, 3);
}

string trimPort(bool doTrim, const string& str) {
  if (doTrim)
    return str.substr(0,str.find_last_of(" "));
  return str;
}

bool openInputPort(RtMidiIn *in, string port) {
  if (port.empty())
    return false;

  // Match full name if port contains hardware id (example: 11:0), otherwise remove the hardware id before matching
  bool doTrim = !boost::regex_match(port, boost::regex("(.+)\\s([0-9]+):([0-9]+)"));
  string portName;
  unsigned int i = 0, nPorts = in->getPortCount();
  for (i=0; i<nPorts; i++ ) {
    portName = in->getPortName(i);
    if (trimPort(doTrim, portName) == port) {
      cout << "Opening input port: " << portName << endl;
      in->openPort(i);
      return true;
    }
  }
  cout << "Couldn't find input port: " << port << endl;
  return false;
}

bool openOutputPort(RtMidiOut *out, string port) {
  // Match full name if port contains hardware id (example: 11:0), otherwise remove the hardware id before matching
  bool doTrim = !boost::regex_match(port, boost::regex("(.+)\\s([0-9]+):([0-9]+)"));
  string portName;
  unsigned int i = 0, nPorts = out->getPortCount();
  for (i=0; i<nPorts; i++ ) {
    portName = out->getPortName(i);
    if (trimPort(doTrim, portName) == port) {
      cout << "Opening output port: " << portName << endl;
      out->openPort(i);
      return true;
    }
  }
  cout << "Couldn't find output port: " << port << endl;
  return false;
}

bool openPorts(string i1, string i2, string i3, string i4, string o) {
  openInputPort(midiin1, i1);
  openInputPort(midiin2, i2);
  openInputPort(midiin3, i3);
  openInputPort(midiin4, i4);
  return openOutputPort(midiout, o);
}

void clock(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t) {
  midiout->sendMessage(clockMessage);
  t->expires_from_now(boost::posix_time::nanoseconds(clockInterval));
  t->async_wait(boost::bind(clock, boost::asio::placeholders::error, t));
}

void signalHandler(const boost::system::error_code& error, int signal_number) {
  if (!error) {
    cout << endl << "Exiting" << endl;
    cleanUp();
    exit(0);
  }
}

void cleanUp() {
  delete midiin1;
  delete midiin2;
  delete midiin3;
  delete midiin4;
  delete midiout;
}

void runInteractiveConfiguration() {
  cout << "This will clear and reconfigure the settings. Continue? (y/N): ";
  string keyHit;
  getline(cin, keyHit);
  if (keyHit != "y") {
    cout << "Exiting" << endl;
    exit(0);
  }
  RtMidiIn *cfgMidiIn = new RtMidiIn();
  RtMidiOut *cfgMidiOut = new RtMidiOut();
  string cfg = "";
  vector<string> inputs;
  vector<string> outputs;
  string portName;
  int userIn;
  int addedIns = 0;
  cout << endl <<
    "Note about hardware id (HWid, example: 11:0). "
    "The HWid for a device might change if you connect it to another USB port."
    << endl <<
    "Recommendation: Do not store HWid for single devices. "
    "Store HWid for devices sharing the same name."
    << endl << endl;
  // Input
  int nPorts = cfgMidiIn->getPortCount();
  cout << "Available input ports:" << endl;
  for (int i=0; i<nPorts; i++) {
    portName = cfgMidiIn->getPortName(i);
    inputs.push_back(portName);
    cout << i << ". " << portName << endl;
  }
  for (int i=0; i<4; i++) {
    cout << "Enter port number for input " << i+1 << " (press enter to disable)" << ": ";
    if (addedIns>=nPorts) {
      cout << endl << "Disabling input" << i+1 << endl;
      cfg += string("input") + convert::to_string(i+1) + string(" =\n");
      continue;
    }
    else if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0 || userIn>=nPorts || inputs[userIn]=="") {
      cout << "Disabling input " << i+1 << endl;
      cfg += string("input") + convert::to_string(i+1) + string(" =\n");
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      continue;
    }
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Store hardware id for input " << i+1 << "? (y/N): ";
    getline(cin, keyHit);
    if (keyHit == "y")
      cfg += string("input") + convert::to_string(i+1) + string(" = ") + inputs[userIn] + "\n";
    else
      cfg += string("input") + convert::to_string(i+1) + string(" = ") + trimPort(true, inputs[userIn]) + "\n";

    cout << "Activate note-off mode for input " << i+1 << "? (y/N): ";
    getline(cin, keyHit);
    if (keyHit == "y")
      cfg += string("input") + convert::to_string(i+1) + string("noteOffMode = true") + "\n";

    addedIns++;
    inputs[userIn] = "";
  }
  // Output
  nPorts = cfgMidiOut->getPortCount();
  cout << endl << "Available output ports:" << endl;
  for (int i=0; i<nPorts; i++) {
    portName = cfgMidiOut->getPortName(i);
    outputs.push_back(portName);
    cout << i << ". " << portName << endl;
  }
  cout << "Enter port number for output: ";
  while (!(cin >> userIn) || userIn < 0 || userIn >= nPorts) {
    cout << "Incorrect port number, try again: ";
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
  }
  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
  cout << "Store hardware id? (y/N): ";
  getline(cin, keyHit);
  if (keyHit == "y")
    cfg += string("output = ") + outputs[userIn] + "\n";
  else
    cfg += string("output = ") + trimPort(true, outputs[userIn]) + "\n";

  cout << endl << "Enable MIDI clock? (Y/n): ";
  getline(cin, keyHit);
  if (keyHit == "n")
    cfg += string("enableClock = false") + "\n";
  else
    cfg += string("enableClock = true") + "\n";

  cout << "Ignore incoming program change messages? (Y/n): ";
  getline(cin, keyHit);
  if (keyHit == "n")
    cfg += string("ignoreProgramChanges = false") + "\n";
  else
    cfg += string("ignoreProgramChanges = true") + "\n";

  cout << "Enter initial MIDI clock BPM (1-300, default 142): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<1 || userIn>300)
    cfg += string("initialBpm = 142") + "\n";
  else
    cfg += string("initialBpm = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enter tap-tempo minimum BPM (default 80): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0)
    cfg += string("tapTempoMinBpm = 80") + "\n";
  else
    cfg += string("tapTempoMinBpm = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enter tap-tempo maximum BPM (default 200): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0)
    cfg += string("tapTempoMaxBpm = 200") + "\n";
  else
    cfg += string("tapTempoMaxBpm = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enter BPM offset (offset + MIDI CC value [0-127] = BPM, default 70): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0)
    cfg += string("bpmOffsetForMidiCC = 70") + "\n";
  else
    cfg += string("bpmOffsetForMidiCC = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enter velocity random offset (default -40): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn < -127 || userIn > 127)
    cfg += string("velocityRandomOffset = -40") + "\n";
  else
    cfg += string("velocityRandomOffset = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enable velocity multi device control (e.g. input 2 controls the velocity setting for input 2, 3 and 4)? (Y/n): ";
  getline(cin, keyHit);
  if (keyHit == "n")
    cfg += string("velocityMultiDeviceCtrl = false") + "\n";
  else
    cfg += string("velocityMultiDeviceCtrl = true") + "\n";

  cout << "Enter tempo MIDI CC number (default 10): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0 || userIn>127)
    cfg += string("tempoMidiCC = 10") + "\n";
  else
    cfg += string("tempoMidiCC = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enter chord mode MIDI CC number (default 11): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0 || userIn>127)
    cfg += string("chordMidiCC = 11") + "\n";
  else
    cfg += string("chordMidiCC = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enter channel routing MIDI CC number (default 12): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0 || userIn>127)
    cfg += string("routeMidiCC = 12") + "\n";
  else
    cfg += string("routeMidiCC = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << "Enter velocity MIDI CC number (default 7): ";
  if (cin.peek()=='\n' || !(cin >> userIn) || userIn<0 || userIn>127)
    cfg += string("velocityMidiCC = 7") + "\n";
  else
    cfg += string("velocityMidiCC = ") + convert::to_string(userIn) + "\n";

  cin.clear();
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  cout << endl;

  ofstream file(CONFIG_FILE);
  file << cfg;

  delete cfgMidiIn;
  delete cfgMidiOut;
}
