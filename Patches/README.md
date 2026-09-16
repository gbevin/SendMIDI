# Vendored JUCE patches

The JUCE modules vendored under `JuceLibraryCode/modules` are based on JUCE
9.0.2 with three local patches, kept as the patch files in this folder.

Two of them are in `juce_midi_ci`, because stock JUCE (verified absent through
JUCE 9.0.2 as of September 2026) doesn't support what the MPE Profile
negotiation needs:

- **`juce_midi_ci-profile-inquiry-inactive.patch`**: a Profile Inquiry Reply
  must list a profile as either enabled or disabled, but stock
  `ChannelProfileStates::getInactive()` returns every supported profile,
  including the active ones, so active profiles were also reported as disabled.
  The fix excludes active profiles from the inactive list.
- **`juce_midi_ci-profile-details-inquiry.patch`**: stock JUCE unconditionally
  NAKs a Profile Details Inquiry with a non-zero target. This adds a
  `profileDetailsInquired()` hook to `ci::ProfileDelegate` (default: empty, which
  still NAKs) and makes the profile host answer with the delegate's data, so a
  responder can serve the MPE Profile optional-features inquiry.

The third is in `juce_audio_devices`, and fixes MIDI message loss on Linux:

- **`juce_alsa-midi-1-bytestream.patch`**: JUCE 9 registers every ALSA client as
  MIDI 2.0, which makes the sequencer carry UMP. Sending then combines the
  controllers of an RPN or NRPN into one MIDI 2.0 message, and receiving decodes
  that message to nothing, so RPN and NRPN traffic disappears between MIDI 1.0
  applications on Linux. The fix registers as a MIDI 1.0 client and always sends
  a bytestream.

The same patches are applied in SendMIDI, ReceiveMIDI and RouteMIDI, keeping
their vendored modules identical.

## Re-applying

`Projucer --resave` re-copies the modules from the external JUCE and **silently
overwrites these patches**. After a resave, restore the vendored code before
committing:

```
git checkout -- JuceLibraryCode/modules JuceLibraryCode/AppConfig.h
```

To apply the patches onto a fresh stock module copy instead (for example after
deliberately updating the vendored JUCE), from the repository root:

```
git apply Patches/juce_midi_ci-profile-inquiry-inactive.patch
git apply Patches/juce_midi_ci-profile-details-inquiry.patch
```

Note that JUCE ships these sources with CRLF line endings while the vendored
copies are LF; if a fresh copy still has CRLF, apply with
`git apply --ignore-whitespace` (or normalize to LF first) and verify with
`git apply --reverse --check <patch>`, which succeeds when a tree contains
exactly what a patch describes.
