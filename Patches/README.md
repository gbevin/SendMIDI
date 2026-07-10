# Vendored JUCE patches

The JUCE modules vendored under `JuceLibraryCode/modules` are based on JUCE
7.0.11 with two local patches to `juce_midi_ci`, kept as the patch files in this
folder. They exist because stock JUCE (verified absent through JUCE master as of
July 2026) doesn't support what the MPE Profile negotiation needs:

- **`juce_midi_ci-profile-inquiry-inactive.patch`** — a Profile Inquiry Reply
  must list a profile as either enabled or disabled, but stock
  `ChannelProfileStates::getInactive()` returns every supported profile,
  including the active ones, so active profiles were also reported as disabled.
  The fix excludes active profiles from the inactive list.
- **`juce_midi_ci-profile-details-inquiry.patch`** — stock JUCE unconditionally
  NAKs a Profile Details Inquiry with a non-zero target. This adds a
  `profileDetailsInquired()` hook to `ci::ProfileDelegate` (default: empty, which
  still NAKs) and makes the profile host answer with the delegate's data, so a
  responder can serve the MPE Profile optional-features inquiry.

The same patches are applied in both SendMIDI and ReceiveMIDI, keeping their
vendored modules identical.

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
