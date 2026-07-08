#!/usr/bin/env bash
#
# End-to-end test: sends MIDI from the real sendmidi binary through a virtual
# MIDI port and checks what actually arrives on the other end, received by
# receivemidi. This exercises the whole path - argument parsing, message
# encoding and the OS MIDI stack - which the in-process unit tests can't.
#
# Usage: e2e-test.sh <path-to-sendmidi> [path-to-receivemidi]
#
# Needs virtual MIDI ports (macOS and Linux only) and a receivemidi binary
# (defaults to one on the PATH). It skips cleanly - exit 0 - when those aren't
# available, so it's safe to call from CI on any platform.

set -u

SM="${1:?usage: e2e-test.sh <path-to-sendmidi> [path-to-receivemidi]}"
RM="${2:-receivemidi}"

case "$(uname -s)" in
    Darwin|Linux) ;;
    *) echo "skip: virtual MIDI ports are not supported on this platform"; exit 0 ;;
esac

if ! command -v "$RM" >/dev/null 2>&1 && [ ! -x "$RM" ]; then
    echo "skip: receivemidi not found (needed to receive the sent messages)"
    exit 0
fi

# One long-lived virtual port for the whole run: repeatedly creating and tearing
# down virtual ports is racy, a single receiver is reliable. receivemidi prints
# notes as numbers (nn) and is asked for every message type sent below.
PORT="SM-E2E-$$"
OUT="$(mktemp)"
"$RM" virt "$PORT" nn on off pp cc pc cp pb clock start stop > "$OUT" 2>&1 &
RPID=$!
cleanup() { kill "$RPID" 2>/dev/null; wait "$RPID" 2>/dev/null; rm -f "$OUT"; }
trap cleanup EXIT

# wait for the virtual port to show up as a sendmidi output destination
for _ in $(seq 1 100); do "$SM" list | grep -q "$PORT" && break; sleep 0.1; done
if ! "$SM" list | grep -q "$PORT"; then
    echo "skip: the virtual MIDI port never appeared (no MIDI on this host?)"
    exit 0
fi
sleep 0.5

"$SM" dev "$PORT" ch 1 on 60 100 off 60 0 pp 62 40 cc 74 55 pc 5 cp 90 pb 8192
"$SM" dev "$PORT" ch 2 nrpn 1000 200
"$SM" dev "$PORT" mc start stop
sleep 1

kill "$RPID" 2>/dev/null; wait "$RPID" 2>/dev/null; trap - EXIT
# squeeze the column padding down to single spaces so the lines are easy to match
GOT="$(tr -s ' ' < "$OUT" | sed 's/[[:space:]]*$//')"
rm -f "$OUT"

failures=0
check() {   # <name> <expected whole line>
    if grep -qxF "$2" <<< "$GOT"; then
        echo "ok   $1"
    else
        echo "FAIL $1"
        echo "  expected line: $2"
        failures=$((failures + 1))
    fi
}

check "note on"                "channel 1 note-on 60 100"
check "note off"               "channel 1 note-off 60 0"
check "poly pressure"          "channel 1 poly-pressure 62 40"
check "control change"         "channel 1 control-change 74 55"
check "program change"         "channel 1 program-change 5"
check "channel pressure"       "channel 1 channel-pressure 90"
check "pitch bend"             "channel 1 pitch-bend 8192"
check "nrpn param select MSB"  "channel 2 control-change 99 7"
check "nrpn param select LSB"  "channel 2 control-change 98 104"
check "nrpn data entry MSB"    "channel 2 control-change 6 1"
check "nrpn data entry LSB"    "channel 2 control-change 38 72"
check "timing clock"           "midi-clock"
check "start"                  "start"
check "stop"                   "stop"

if [ "$failures" -eq 0 ]; then
    echo "all end-to-end tests passed"
else
    echo "$failures end-to-end test(s) failed"
    echo "--- full received transcript ---"
    echo "$GOT"
    exit 1
fi
