#!/usr/bin/env bash

# Author: Jarrod Cameron (z5210220)
# Date:   14/10/19 11:05

# Tmux window names
SESSION="Networks"
WINNAME="client_login"

# Args for client/server
SERVER_PORT="1024"
BLOCK_DURATION="0"
TIMEOUT="0"
IP_ADDR="127.0.0.1"

set -e

function get_free_portnum () {
    t=$SERVER_PORT
    while [ -n "$(netstat -an | grep "$t")" ]
    do
        t=$((t+1))
    done
    printf $t
}

port="$(get_free_portnum)"

# Command to run in the tmux session
server_cmd="./server $port $BLOCK_DURATION $TIMEOUT"
client_cmd="./client $IP_ADDR $port"

clear
make

if [ "$1" = "comp" ]; then
    exit 1
fi

echo
echo '  Stats:'
echo "    PORT: $port"
echo "    BLOCK_DURATION: $BLOCK_DURATION"
echo "    TIMEOUT: $TIMEOUT"
echo "    IP_ADDR: $IP_ADDR"
echo
echo "  server:"
echo "    $server_cmd"
echo
echo "  client:"
echo "    $client_cmd"
echo

tmux new-session -d -s "$SESSION" -n "$WINNAME" # Create session, don't attach, set names
tmux split-window -v -t "$SESSION"              # Create horizontal split
tmux send-keys -t 1 "clear && echo '[SERVER]' && $server_cmd" C-m  # Command for top panel
tmux send-keys -t 2 "clear && echo '[CLIENT]' && $client_cmd" C-m  # Command for bot panel
tmux attach-session -t "$SESSION"               # Attach to session
