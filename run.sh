# Date:   14/10/19 11:05

# Tmux window names
SESSION="Networks"
WINNAME="Author: Jarrod Cameron (z5210220)"

# Args for client/server
SERVER_PORT="1024"
BLOCK_DURATION="0"
TIMEOUT="600"
IP_ADDR="127.0.0.1"

# Flags for valgrind
VAL="valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all"

# Exit if non-zero reutrn status
set -e

# Scans list of open ports and returns port# that is free for our program.
# Saves messing with a port number each time
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
server_cmd="$VAL ./server $port $BLOCK_DURATION $TIMEOUT"
client_cmd="sleep 1 && $VAL ./client $IP_ADDR $port"

if [ "$1" = "comp" ]; then
    clear
    make
    exit 0
fi

clear
make

echo
echo '  Stats:'
echo "    PORT: $port"
echo "    BLOCK_DURATION: $BLOCK_DURATION"
echo "    TIMEOUT: $TIMEOUT"
echo "    IP_ADDR: $IP_ADDR"
echo
echo "  SERVER:"
echo "    $server_cmd"
echo
echo "  CLIENT:"
echo "    $client_cmd"
echo

# /-------------------------\
# | [SERVER]   | [CLIENT 1] |
# |            |            |
# |            |            |
# |-------------------------|
# | [CLIENT 2] | [CLIENT 3] |
# |            |            |
# |            |            |
# \-------------------------/

tmux new-session -d -s "$SESSION" -n "$WINNAME" # Create session, don't attach, set names
tmux split-window -v -t "$SESSION"              # Create horizontal split
tmux split-window -h -t 2              # Create horizontal split
tmux split-window -h -t 1              # Create horizontal split
tmux send-keys -t 1 "clear && echo '[SERVER]' && $server_cmd" C-m  # Command for top panel
sleep 0.05
tmux send-keys -t 2 "clear && echo '[CLIENT 1]' && $client_cmd" C-m  # Command for bot panel
sleep 0.05
tmux send-keys -t 3 "clear && echo '[CLIENT 2]' && $client_cmd" C-m  # Command for bot panel
sleep 0.05
tmux send-keys -t 4 "clear && echo '[CLIENT 3]' && $client_cmd" C-m  # Command for bot panel
tmux attach-session -t "$SESSION"               # Attach to session
