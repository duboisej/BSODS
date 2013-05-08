# connect.sh - runs and connects the game client, passing in command line argument of server 
# and port number.

#!/bin/bash
./client "connect 128.197.11.31:$1"