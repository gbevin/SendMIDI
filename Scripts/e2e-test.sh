#!/usr/bin/env bash
#
# End-to-end tests that drive sendmidi through real MIDI ports and check what
# arrives with receivemidi, so port enumeration, port matching and the MIDI
# backend are exercised on the real binaries. On macOS and Linux receivemidi
# creates a virtual port per case; on Windows E2E_PORT names an existing
# loopback port (loopMIDI) that both tools open.
#
# Usage: e2e-test.sh <path-to-sendmidi> <path-to-receivemidi>
#
# Every case brackets its traffic with CC 119 marker messages: the start marker
# is repeated until it shows up in the receiver's output, which proves both ends
# are open, and the end marker tells when everything before it has arrived.

set -u
if [ -n "${E2E_TRACE:-}" ]; then
    set -x
fi

SENDMIDI="$1"
RECEIVEMIDI="$2"
PORT="${E2E_PORT:-}"
WORK="$(mktemp -d)"
MARK_START='control-change +(119 +1|77 +01)$'
MARK_END='control-change +(119 +2|77 +02)$'
failures=0
receiver_pid=""
port=""
received=""

virtual_ports() { [ -z "$PORT" ]; }

pass() { echo "ok   $1"; }

fail() {
    echo "FAIL $1"
    shift
    printf '     %s\n' "$@"
    failures=$((failures+1))
}

# compares two multi-line strings and reports the difference
check() {
    local name="$1" expected="$2" actual="$3"
    if [ "$expected" = "$actual" ]; then
        pass "$name"
    else
        fail "$name"
        echo "--- expected -------"; printf '%s\n' "$expected"
        echo "--- actual ---------"; printf '%s\n' "$actual"
        echo "--------------------"
    fi
}

# a fresh port name per case, so a port lingering from an earlier case can't
# satisfy a later one
new_port() {
    if virtual_ports; then
        port="E2E sendmidi $$ $RANDOM"
    else
        port="$PORT"
    fi
}

# starts a tool in the background with its output in the given file; the MIDI
# backend may refuse a virtual port created right after one vanished, so a
# refused start is tried again
start_background() {
    local out="$1"
    shift
    local attempt
    for attempt in 1 2 3; do
        "$@" > "$out" 2>&1 &
        started_pid=$!
        sleep 1
        if ! grep -q "Couldn't create virtual MIDI" "$out"; then
            return 0
        fi
        kill "$started_pid" 2>/dev/null
        wait "$started_pid" 2>/dev/null
    done
    return 1
}

stop_receiver() {
    if [ -n "$receiver_pid" ]; then
        kill "$receiver_pid" 2>/dev/null
        wait "$receiver_pid" 2>/dev/null
        receiver_pid=""
        # the MIDI backend may refuse a port created right after one vanished
        sleep 0.5
    fi
}

# starts receivemidi on the case's port with the given arguments and waits for
# the start marker to come through; filters passed in must let CC 119 pass
start_receiver() {
    local out="$1"
    shift
    local attempt i
    for attempt in 1 2 3; do
        new_port
        if virtual_ports; then
            "$RECEIVEMIDI" virt "$port" "$@" > "$out" 2>&1 &
        else
            "$RECEIVEMIDI" dev "$port" "$@" > "$out" 2>&1 &
        fi
        receiver_pid=$!
        for i in $(seq 1 40); do
            sleep 0.25
            "$SENDMIDI" dev "$port" cc 119 1 > /dev/null 2>&1
            if grep -qE "$MARK_START" "$out"; then
                return 0
            fi
        done
        if ! grep -q "Couldn't create virtual MIDI" "$out"; then
            break
        fi
        stop_receiver
    done
    echo "     receiver output:"
    sed 's/^/     | /' "$out"
    return 1
}

# sends the end marker, waits for it, stops the receiver and leaves the lines
# between the markers in $received
finish_receiver() {
    local out="$1"
    local i
    for i in $(seq 1 40); do
        "$SENDMIDI" dev "$port" cc 119 2 > /dev/null 2>&1
        if grep -qE "$MARK_END" "$out"; then
            break
        fi
        sleep 0.25
    done
    stop_receiver
    received="$(tr -d '\r' < "$out" | awk -v s="$MARK_START" -v e="$MARK_END" \
        '$0 ~ s { buf = ""; next } $0 ~ e { printf "%s", buf; exit } { buf = buf $0 "\n" }')"
}

send() {
    "$SENDMIDI" dev "$port" "$@"
}

trap 'stop_receiver; rm -rf "$WORK"' EXIT

# --- the receiver's port is listed and every message type arrives intact -----
if start_receiver "$WORK/battery.txt"; then
    if send list | grep -qF "$port"; then
        pass "list shows the port"
    else
        fail "list shows the port" "$(send list)"
    fi

    send on 60 100 off 60 0 pp C3 90 cc 74 64 cc14 1 8192 pc 5 cp 77 pb 8192 pb 0 \
         rpn 0 2 nrpn 300 1000 mpe 1 7 \
         mc start stop cont as tun rst tc 1 5 spp 100 ss 3 \
         hex syx 7E 7F 09 01 on 3C 7F dec ch 16 on 127 1 omc 4 on C4 1
    finish_receiver "$WORK/battery.txt"
    EXPECTED='channel  1   note-on           C3 100
channel  1   note-off          C3   0
channel  1   poly-pressure     C3  90
channel  1   control-change    74    64
channel  1   control-change     1    64
channel  1   control-change    33     0
channel  1   program-change         5
channel  1   channel-pressure      77
channel  1   pitch-bend          8192
channel  1   pitch-bend             0
channel  1   control-change   101     0
channel  1   control-change   100     0
channel  1   control-change     6     0
channel  1   control-change    38     2
channel  1   control-change   101   127
channel  1   control-change   100   127
channel  1   control-change    99     2
channel  1   control-change    98    44
channel  1   control-change     6     7
channel  1   control-change    38   104
channel  1   control-change   101   127
channel  1   control-change   100   127
channel  1   control-change   101     0
channel  1   control-change   100     6
channel  1   control-change     6     7
channel  1   control-change    38     0
channel  1   control-change   101   127
channel  1   control-change   100   127
midi-clock
start
stop
continue
active-sensing
tune-request
reset
time-code  1 5
song-position   100
song-select   3
system-exclusive hex 7E 7F 09 01 dec
channel  1   note-on           C3 127
channel 16   note-on           G8   1
channel 16   note-on           C3   1'
    check "every message type round-trips through a real port" "$EXPECTED" "$received"
else
    fail "every message type round-trips through a real port" "the receiver never saw the start marker" "$(cat "$WORK/battery.txt")"
    stop_receiver
fi

# --- a SysEx file is sent complete, with and without the worst-case pacing ----
python3 - "$WORK/big.syx" <<'PY' 2>/dev/null || printf '\xF0\x7D\x01\x02\x03\x7F\xF7' > "$WORK/big.syx"
import sys
open(sys.argv[1], "wb").write(bytes([0xF0, 0x7D] + [i % 128 for i in range(200)] + [0xF7]))
PY
if [ ! -s "$WORK/big.syx" ]; then
    fail "a SysEx file arrives complete" "could not generate the SysEx file"
else
    EXPECTED="system-exclusive hex $(od -An -tx1 -v "$WORK/big.syx" | tr -s ' \n' ' ' | sed 's/^ //; s/ $//' | tr 'a-f' 'A-F' | sed 's/^F0 //; s/ F7$//') dec"
    for mode in "" nowait; do
        if start_receiver "$WORK/syx.txt"; then
            send $mode syf "$WORK/big.syx" > /dev/null
            finish_receiver "$WORK/syx.txt"
            check "a 203-byte SysEx file arrives complete${mode:+ with $mode}" "$EXPECTED" "$received"
        else
            fail "a 203-byte SysEx file arrives complete${mode:+ with $mode}" "the receiver never saw the start marker"
            stop_receiver
        fi
    done
fi

# --- commands from a program file and from standard input -------------------
printf 'on 60 100\noff 60 0\n' > "$WORK/prog.txt"
if start_receiver "$WORK/file.txt"; then
    send file "$WORK/prog.txt"
    printf 'on 61 100\noff 61 0\n' | send --
    finish_receiver "$WORK/file.txt"
    EXPECTED='channel  1   note-on           C3 100
channel  1   note-off          C3   0
channel  1   note-on          C#3 100
channel  1   note-off         C#3   0'
    check "a program file and standard input both drive the port" "$EXPECTED" "$received"
else
    fail "a program file and standard input both drive the port" "the receiver never saw the start marker"
    stop_receiver
fi

# --- the port name matches case-insensitively -------------------------------
if start_receiver "$WORK/match.txt"; then
    lower="$(printf '%s' "$port" | tr '[:upper:]' '[:lower:]')"
    "$SENDMIDI" dev "$lower" on 62 100
    finish_receiver "$WORK/match.txt"
    check "the port name matches case-insensitively" 'channel  1   note-on           D3 100' "$received"
else
    fail "the port name matches case-insensitively" "the receiver never saw the start marker"
    stop_receiver
fi

if virtual_ports; then
    # --- ports sharing a name are numbered and can be picked apart -----------
    name="E2E sendmidi twin $$ $RANDOM"
    start_background "$WORK/twin1.txt" "$RECEIVEMIDI" virt "$name"
    twin1=$started_pid
    start_background "$WORK/twin2.txt" "$RECEIVEMIDI" virt "$name"
    twin2=$started_pid
    sleep 2
    "$SENDMIDI" dev "$name (2)" on 63 100
    "$SENDMIDI" dev "$name (1)" on 64 100
    sleep 1
    listing="$("$SENDMIDI" list)"
    kill $twin1 $twin2 2>/dev/null
    wait $twin1 $twin2 2>/dev/null
    if printf '%s\n' "$listing" | grep -qF "$name (1)" && printf '%s\n' "$listing" | grep -qF "$name (2)"; then
        pass "ports sharing a name are listed numbered"
    else
        fail "ports sharing a name are listed numbered" "$listing"
    fi
    # the numbering follows the order the system lists the ports, which is not
    # the order they were created, so each numbered name has to reach one port
    # and one note each, whichever way round that is
    first=$(grep -c . "$WORK/twin1.txt")
    second=$(grep -c . "$WORK/twin2.txt")
    if [ "$first" = "1" ] && [ "$second" = "1" ]; then
        pass "each numbered name reaches exactly one of the ports"
    else
        fail "each numbered name reaches exactly one of the ports" "$first line(s) and $second line(s)"
    fi
    check "the numbered names deliver both notes" \
        "$(printf 'channel  1   note-on          D#3 100\nchannel  1   note-on           E3 100' | sort)" \
        "$( { tr -d '\r' < "$WORK/twin1.txt"; tr -d '\r' < "$WORK/twin2.txt"; } | sort)"

    # --- the MPE Profile negotiates through MIDI-CI, including the details ---
    name="E2E sendmidi mpe $$ $RANDOM"
    start_background "$WORK/mpe-responder.txt" "$RECEIVEMIDI" mpp "$name" 1 7 mpb 1 mcp 2 m3d 1
    responder=$started_pid
    sleep 2
    "$SENDMIDI" dev "$name" mpp "$name" 1 7 > "$WORK/mpe-initiator.txt" 2>&1
    sleep 1
    kill $responder 2>/dev/null
    wait $responder 2>/dev/null
    EXPECTED='Initiator MUID negotating MPE Profile with manager channel 1 and 7 member channels
MUID : Discovered
MUID : Requesting MPE Profile enablement with manager channel 1 and 7 member channels
MUID : MPE Profile enabled with manager channel 1 and 7 member channels
MUID : Inquiring MPE Profile details for optional features
MUID : MPE Profile details received for optional features
MUID   channel response : not supported
MUID   pitch bend       : supported
MUID   channel pressure : alternate bipolar controller
MUID   3rd dimension    : standard controller'
    check "the MPE Profile initiator negotiates and reads the optional features" \
        "$EXPECTED" "$(tr -d '\r' < "$WORK/mpe-initiator.txt" | sed -E 's/MUID 0x[0-9a-f]+/MUID/')"
    EXPECTED='Responder MUID waiting for MPE Profile negotiation on channel 1
MUID : MPE Profile enabled with manager channel 1 and 7 member channels
MUID : MPE Profile details inquired for optional features'
    check "the MPE Profile responder enables the profile and answers the inquiry" \
        "$EXPECTED" "$(tr -d '\r' < "$WORK/mpe-responder.txt" | sed -E 's/MUID 0x[0-9a-f]+/MUID/')"
fi

echo
if [ "$failures" -eq 0 ]; then
    echo "all end-to-end tests passed"
else
    echo "$failures end-to-end test(s) failed"
    exit 1
fi
