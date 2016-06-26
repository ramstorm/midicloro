#!/bin/bash

cd $1
while true; do
  if ! ps aux | grep -v 'grep' | grep -v 'startm' | grep 'midicloro' ; then
    ./midicloro &
  else
    exit 0
  fi
  sleep 5s
done
