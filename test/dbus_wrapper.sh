#!/bin/bash

function finish()
{
    if [ -n "$DBUS_SESSION_BUS_PID" ]; then
        kill $DBUS_SESSION_BUS_PID || exit 1
    fi
    rm -f dbus-session-bus-pid
    rm -f dbus-session-bus-address
}

trap finish EXIT
dbus-daemon --fork --session --print-address=3 --print-pid=4 \
  3> dbus-session-bus-address 4> dbus-session-bus-pid

if [ $? != 0 ]; then
    exit 1
fi

export DBUS_SESSION_BUS_ADDRESS="$(cat dbus-session-bus-address)"
DBUS_SESSION_BUS_PID="$(cat dbus-session-bus-pid)"

"$@"
RET=$?
exit $RET
