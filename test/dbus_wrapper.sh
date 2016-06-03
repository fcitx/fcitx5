#!/bin/sh

function finish()
{
    if [ -n "$DBUS_SESSION_BUS_PID" ]; then
        kill $DBUS_SESSION_BUS_PID || exit 1
    fi
}

trap finish EXIT
toeval=`dbus-launch --sh-syntax`

if [ $? != 0 ]; then
    exit 1
fi

eval $toeval

"$@"
RET=$?
exit $RET
