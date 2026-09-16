#!/usr/bin/env bash
#
# Runs a command inside a short-lived virtual machine whose kernel has the ALSA
# sequencer. The CI runner kernels are built without sound support, so virtual
# MIDI ports cannot be created there at all. The repository is shared into the
# guest, the command runs from the repository root, and its exit status becomes
# this script's.
#
# Usage: linux-guest-test.sh <command> [arguments...]
#
# Needs KVM, qemu-system-x86 and cloud-image-utils.

set -u

IMAGE="https://cloud-images.ubuntu.com/minimal/releases/noble/release/ubuntu-24.04-minimal-cloudimg-amd64.img"
WORK="$(mktemp -d)"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
trap 'rm -rf "$WORK"; rm -f "$ROOT/guest-output" "$ROOT/guest-status"' EXIT

if [ ! -w /dev/kvm ]; then
    echo "/dev/kvm is not writable, so the guest cannot start"
    exit 1
fi

wget -q "$IMAGE" -O "$WORK/disk.img" || { echo "could not download the guest image"; exit 1; }
qemu-img resize -q "$WORK/disk.img" +2G

# the guest mounts the repository, loads the sequencer, installs the ALSA
# runtime the binaries link against, runs the command and shuts down
cat > "$WORK/user-data" <<EOF
#cloud-config
output: { all: "| tee -a /dev/console" }
runcmd:
  - [ mkdir, -p, /mnt/host ]
  - [ mount, -t, 9p, -o, "trans=virtio,version=9p2000.L,msize=512000", host, /mnt/host ]
  - modprobe snd-seq
  - apt-get update -qq && apt-get install -y -qq libasound2t64
  - cd /mnt/host && $* > /mnt/host/guest-output 2>&1; echo \$? > /mnt/host/guest-status
  - poweroff
EOF
cloud-localds "$WORK/seed.iso" "$WORK/user-data"

timeout 1200 qemu-system-x86_64 \
    -enable-kvm -cpu host -m 4096 -smp "$(nproc)" -nographic \
    -drive file="$WORK/disk.img",format=qcow2,if=virtio \
    -drive file="$WORK/seed.iso",format=raw,if=virtio \
    -netdev user,id=n0 -device virtio-net-pci,netdev=n0 \
    -virtfs local,path="$ROOT",mount_tag=host,security_model=mapped-xattr,id=host \
    > "$WORK/console.log" 2>&1

if [ -f "$ROOT/guest-output" ]; then
    cat "$ROOT/guest-output"
else
    echo "the guest produced no output, the end of its console was:"
    tail -40 "$WORK/console.log"
fi

exit "$(cat "$ROOT/guest-status" 2>/dev/null || echo 1)"
