#!/bin/bash

cd $1
while true; do
  if ! ps aux | grep -v 'grep' | grep -v 'startm' | grep 'midicloro' ; then
    ./midicloro &
  fi
  sleep 10s
done
