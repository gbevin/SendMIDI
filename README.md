# SendMIDI

SendMIDI is a multi-platform command-line tool makes it very easy to quickly send MIDI messages to MIDI devices from your computer.

All the heavy lifting is done by the wonderful JUCE library.

The project website is https://github.com/gbevin/SendMIDI

## Purpose
This tool is mainly intended for configuration or setup through Continuous Control, RPN and NRPN messages, but many other MIDI messages can be sent

## Download

You can download pre-built binaries from the releases section:
https://github.com/gbevin/SendMIDI/releases

Since SendMIDI is free and open-source, you can also easily build it yourself. Just take a look into the Builds directory when you download the sources.

## Usage
To use it, simply type "sendmidi" on the command line and follow it with a series of commands that you want to execute. These commands have purposefully been chosen to be concise and easy to remember, so that it's extremely fast and intuitive to quickly shoot out a few MIDI messages.

These are all the supported commands:
```
  dev   name           Set the name of the MIDI output port (REQUIRED)
  list                 Lists the MIDI output ports
  file  path           Loads commands from the specified file
  ch    number         Set MIDI channel for the commands (1-16), defaults to 1
  on    note velocity  Send Note On with note (0-127) and velocity (0-127)
  off   note velocity  Send Note Off with note (0-127) and velocity (0-127)
  pp    note value     Send Poly Pressure with note (0-127) and pressure (0-127)
  cc    number value   Send Continuous Controller (0-127) with value (0-127)
  pc    number         Send Program Change number (0-127)
  cp    value          Send Channel Pressure value (0-127)
  pb    value          Send Pitch Bend value (0-16383)
  rpn   number value   Send RPN number (0-16383) with value (0-16383)
  nrpn  number value   Send NRPN number (0-16383) with value (0-16383)
  ```
  
## Examples
  
Here are a few examples to get you started:

List all the available MIDI output ports on your system

```
sendmidi list
```

Switch the LinnStrument to User Firmware Mode by setting NRPN to 245 to the value 1:
  
```
sendmidi dev "LinnStrument MIDI" nrpn 245 1
```
  
Light up LinnStrument column 5 on row 0 in green by setting CCs 20, 21, and 22 to the column, row and color:
  
```
sendmidi dev "LinnStrument MIDI" cc 20 5 cc 21 0 cc 22 3
```
