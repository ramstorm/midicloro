#!/bin/bash

cd $1
while true; do
  if ! ps aux | grep -v 'grep' | grep -v 'startm' | grep 'midicloro' ; then
    ./midicloro &
    sleep 5s
  else
    sleep 25s
  fi
done
