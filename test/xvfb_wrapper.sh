#!/bin/bash

XVFB_DISPLAY=:9345

XVFB=$1
shift

"$XVFB" $XVFB_DISPLAY &

XVFB_PID=$!
export DISPLAY=$XVFB_DISPLAY

function finish()
{
    if [ -n "$XVFB_PID" ]; then
        kill $XVFB_PID >/dev/null 2>&1
        wait $XVFB_PID
    fi
}

trap finish EXIT

"$@"
RET=$?
exit $RET
