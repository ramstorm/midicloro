all:
	g++ -Wall -D__LINUX_ALSA__ -o midicloro midicloro.cpp rtmidi/RtMidi.cpp -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG -lasound -lpthread -lboost_system -lboost_program_options -lboost_regex

run: all
	./midicloro
