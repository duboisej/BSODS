# one_client.sh - when called, runs an applescript that creates a new terminal tab and runs
# a new instance of the game client. Passes command line argument of port number into connect.sh

#!/bin/sh

pwd=`pwd`
osascript -e "tell application \"Terminal\"" \
    -e "tell application \"System Events\" to keystroke \"t\" using {command down}" \
    -e "do script \"cd $pwd; ./connect.sh $1; clear\" in front window" \
    -e "end tell"
    > /dev/null