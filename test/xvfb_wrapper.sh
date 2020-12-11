#!/bin/sh

XVFB_DISPLAY=:9345

XVFB=$1
shift

"$XVFB" $XVFB_DISPLAY &

XVFB_PID=$!
export DISPLAY=$XVFB_DISPLAY

finish()
{
    if [ -n "$XVFB_PID" ]; then
        kill $XVFB_PID >/dev/null 2>&1
        wait $XVFB_PID
    fi
}

trap finish EXIT

if which xprop >/dev/null 2>&1; then
    i=1
    while [ "$i" -lt 5 ]; do
        if xprop -root >/dev/null 2>&1; then
            break
        else
            echo "Can't connect to Xvfb, retrying..."
            sleep "$i"
        fi
        i=$((i + 1))
    done
    if [ "$i" -eq 6 ]; then
        exit 1
    fi
else
    sleep 1
fi

"$@"
RET=$?
exit $RET
