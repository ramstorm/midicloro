#!/bin/bash

cd $1
#Initial start. We wait 15s to start the loop in order not to start different midicloro instances in parallel
./midicloro &
sleep 15s
while true; do
  if ! ps aux | grep -v 'grep' | grep -v 'startm' | grep 'midicloro' ; then
    ./midicloro &
#Awful trick to shutdown a headless RPi when a Ralink USB WiFi dongle is not detected. 
#Someone should get jailed for that.
  elif ! lsusb | grep 'Ralink' ; then
    halt
  else
    sleep 5s
  fi
done
