#!/usr/bin/env bash

# Author: Jarrod Cameron (z5210220)
# Date:   14/10/19 11:05

# Tmux window names
SESSION="client_init"
WINNAME="CS"

# Args for client/server
SERVER_PORT="2000"
BLOCK_DURATION="0"
TIMEOUT="0"
IP_ADDR="127.0.0.1"

# Command to run in the tmux session
SERVER_CMD="clear && ./server $SERVER_PORT $BLOCK_DURATION $TIMEOUT"
CLIENT_CMD="clear && ./client $IP_ADDR $SERVER_PORT"

set -e

clear
make

tmux new-session -d -s "$SESSION" -n "$WINNAME" # Create session, don't attach, set names
tmux split-window -v -t "$SESSION"              # Create horizontal split
tmux send-keys -t 1 "$SERVER_CMD" C-m           # Command for top panel
tmux send-keys -t 2 "$CLIENT_CMD" C-m           # Command for bot panel
tmux attach-session -t "$SESSION"               # Attach to session
