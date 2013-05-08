#!/bin/sh

pwd=`pwd`
osascript -e "tell application \"Terminal\"" \
    -e "tell application \"System Events\" to keystroke \"t\" using {command down}" \
    -e "do script \"cd $pwd; ./connect.sh $1; clear\" in front window" \
    -e "end tell"
    > /dev/null