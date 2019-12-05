# Date:   14/10/19 11:05
# Author: Jarrod Cameron (z5210220)

# Tmux window names
SESSION="Networks"
WINNAME="Author: Jarrod Cameron (z521022)"

# Args for client/server
SERVER_PORT="1024"
BLOCK_DURATION="0"
TIMEOUT="600"
HOST_NAME="localhost"

# Delay between each connection to ensure the right order
DELAY=0.3

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

# If there is an already running session kill it
function kill_open_session () {
    if tmux ls | grep "^${SESSION}: "; then
        tmux kill-session -t "$SESSION"
    fi
}

port="$(get_free_portnum)"

# Command to run in the tmux session
server_cmd="./server $port $BLOCK_DURATION $TIMEOUT"
client_cmd="./client $HOST_NAME $port"

if [ "$1" = "ip" ]; then
    printf "IPv4: "
    hostname -i
    printf "Hostname: "
    hostname
    exit 0
elif [ -n "$1" ]; then
    echo "Unknown argument: $1"
    echo "Usage: bash ./run.sh [ip]"
fi

clear
make

echo
echo '  Stats:'
echo "    PORT: $port"
echo "    BLOCK_DURATION: $BLOCK_DURATION"
echo "    TIMEOUT: $TIMEOUT"
echo "    HOST_NAME: $HOST_NAME"
echo
echo "  SERVER:"
echo "    $server_cmd"
echo
echo "  CLIENT:"
echo "    $client_cmd"
echo
echo "  Author:"
echo "    Jarrod Cameron (z521022)"
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

kill_open_session &>/dev/null

tmux new-session -d -s "$SESSION" -n "$WINNAME" # Create session, don't attach, set names

# Depending on .tmux.config the first pane will be different
pane1="$(tmux list-panes -t Networks | awk -F':' '{print $1}')"
pane2="$((pane1+1))"
pane3="$((pane1+2))"
pane4="$((pane1+3))"

tmux split-window -v -t "$SESSION"              # Create horizontal split
tmux split-window -h -t "$pane2"              # Create horizontal split
tmux split-window -h -t "$pane1"              # Create horizontal split
tmux send-keys -t "$pane1" "clear && echo '[SERVER]' && $server_cmd" C-m  # Command for top panel
sleep "$DELAY"
tmux send-keys -t "$pane2" "clear && echo '[CLIENT 1]' && $client_cmd" C-m  # Command for bot panel
sleep "$DELAY"
tmux send-keys -t "$pane3" "clear && echo '[CLIENT 2]' && $client_cmd" C-m  # Command for bot panel
sleep "$DELAY"
tmux send-keys -t "$pane4" "clear && echo '[CLIENT 3]' && $client_cmd" C-m  # Command for bot panel
sleep "$DELAY"
tmux attach-session -t "$SESSION"               # Attach to session
