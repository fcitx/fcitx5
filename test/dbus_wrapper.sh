#!/bin/sh

finish()
{
    if [ -n "$DBUS_SESSION_BUS_PID" ]; then
        kill "$DBUS_SESSION_BUS_PID" || exit 1
    fi
    if [ -n "$ADDRESS_FILE" ]; then
        rm -f -- "$ADDRESS_FILE"
    fi
    if [ -n "$PID_FILE" ]; then
        rm -f -- "$PID_FILE"
    fi
}

ADDRESS_FILE=
PID_FILE=

trap finish EXIT

ADDRESS_FILE=$(mktemp -p "$PWD" dbus-session-bus-address.XXXXXX)
PID_FILE=$(mktemp -p "$PWD" dbus-session-bus-pid.XXXXXX)

DBUS_DAEMON=$1
shift

"$DBUS_DAEMON" --fork --session --print-address=3 --print-pid=4 \
  3> "$ADDRESS_FILE" 4> "$PID_FILE" || exit 1

DBUS_SESSION_BUS_ADDRESS=$(cat "$ADDRESS_FILE")
DBUS_SESSION_BUS_PID=$(cat "$PID_FILE")

export DBUS_SESSION_BUS_ADDRESS

"$@"
RET=$?
exit $RET
