#!/bin/bash

if [ ! -d "$1" ]; then
  echo "Couldn't find directory: $1"
  echo "Usage: startm.sh /path/to/midicloro"
  exit 0
fi

logfile="$1/startm.log"
if [ ! -f "$1/midicloro" ]; then
  echo "Couldn't find midicloro in: $1" > $logfile
  echo "Usage: startm.sh /path/to/midicloro" >> $logfile
  exit 0
fi

while true; do
  d=`date '+%Y-%m-%d %H:%M:%S'`
  echo "$d - Trying to start midicloro" > $logfile
  cd $1
  echo "pwd:" >> $logfile
  pwd >> $logfile
  echo "ls:" >> $logfile
  ls -la >> $logfile
  ./midicloro & >> $logfile
  sleep 10s
  echo "ps:" >> $logfile
  if ps aux | grep -v 'grep' | grep -v 'startm' | grep 'midicloro' >> $logfile ; then
    echo "Successfully started midicloro, exiting startm" >> $logfile
    exit 0
  fi
done

