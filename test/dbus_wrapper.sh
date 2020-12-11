#!/bin/sh

finish()
{
    if [ -n "$DBUS_SESSION_BUS_PID" ]; then
        kill "$DBUS_SESSION_BUS_PID" || exit 1
    fi
    rm -f dbus-session-bus-pid
    rm -f dbus-session-bus-address
}

trap finish EXIT
DBUS_DAEMON=$1
shift

"$DBUS_DAEMON" --fork --session --print-address=3 --print-pid=4 \
  3> dbus-session-bus-address 4> dbus-session-bus-pid || exit 1

DBUS_SESSION_BUS_ADDRESS="$(cat dbus-session-bus-address)"
DBUS_SESSION_BUS_PID="$(cat dbus-session-bus-pid)"

export DBUS_SESSION_BUS_ADDRESS

"$@"
RET=$?
exit $RET
