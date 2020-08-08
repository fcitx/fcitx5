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

if which xprop >/dev/null 2>&1; then
    for((i=1;i<=5;i++)); do
        if xprop -root >/dev/null 2>&1; then
            break
        else
            echo "Can't connect to Xvfb, retrying..."
            sleep $i
        fi
    done
    if [[ $i == 6 ]]; then
        exit 1
    fi
else
    sleep 1
fi

"$@"
RET=$?
exit $RET
