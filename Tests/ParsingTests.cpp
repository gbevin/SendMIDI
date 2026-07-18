/*
 * This file is part of SendMIDI.
 * Copyright (command) 2017-2024 Uwyn LLC.  https://www.uwyn.com
 *
 * SendMIDI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SendMIDI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "JuceHeader.h"

#include "../Source/ApplicationState.h"

// Exercises the number and note-name parsing that turns command arguments into
// MIDI values: note names against the middle-C octave, decimal/hexadecimal
// selection, the M/H suffixes, and the 7/14-bit clamping.
class ParsingTests : public UnitTest
{
public:
    ParsingTests() : UnitTest("Parsing", "Parsing") {}

    void runTest() override
    {
        beginTest("Note names map to numbers around the middle-C octave");
        {
            ApplicationState s;
            s.octaveMiddleC_ = 3;                     // the default
            expectEquals((int)s.asNoteNumber("C3"), 60);
            expectEquals((int)s.asNoteNumber("C-2"), 0);
            expectEquals((int)s.asNoteNumber("G8"), 127);
            expectEquals((int)s.asNoteNumber("C#3"), 61);
            expectEquals((int)s.asNoteNumber("Db3"), 61);   // flat of D
            expectEquals((int)s.asNoteNumber("A3"), 69);

            s.octaveMiddleC_ = 4;                     // middle C is now C4
            expectEquals((int)s.asNoteNumber("C3"), 48);
            expectEquals((int)s.asNoteNumber("C4"), 60);
        }

        beginTest("A plain number is read as a note number too");
        {
            ApplicationState s;
            expectEquals((int)s.asNoteNumber("60"), 60);
        }

        beginTest("Decimal is the default, the H and M suffixes force a base");
        {
            ApplicationState s;
            expectEquals((int)s.asDecOrHex7BitValue("100"), 100);   // decimal by default
            expectEquals((int)s.asDecOrHex7BitValue("64H"), 0x64);  // 100, forced hex
            expectEquals((int)s.asDecOrHex7BitValue("100M"), 100);  // forced decimal
            expectEquals(s.asDecOrHexIntValue("1F4H"), 0x1F4);      // 500
        }

        beginTest("The hex command flips the default base for later values");
        {
            ApplicationState s;
            s.collectLine("hex");                     // sets hexadecimal-by-default
            expectEquals((int)s.asDecOrHex7BitValue("40"), 0x40);   // now hex
            expectEquals((int)s.asDecOrHex7BitValue("10M"), 10);    // M still forces decimal
        }

        beginTest("Values are clamped to 7 and 14 bits");
        {
            ApplicationState s;
            expectEquals((int)s.asDecOrHex7BitValue("200"), 127);
            expectEquals((int)s.asDecOrHex14BitValue("20000"), 16383);
            expectEquals((int)ApplicationState::limit7Bit(-5), 0);
            expectEquals((int)ApplicationState::limit14Bit(99999), 16383);
        }

        beginTest("Ports that share a name get numbered and can be picked");
        {
            Array<MidiDeviceInfo> devices;
            devices.add({"sooperlooper", "id1"});
            devices.add({"Synth", "id2"});
            devices.add({"sooperlooper", "id3"});

            auto names = ApplicationState::displayNames(devices);
            expectEquals(names[0], String("sooperlooper (1)"));
            expectEquals(names[1], String("Synth"));
            expectEquals(names[2], String("sooperlooper (2)"));

            // a numbered name selects that specific port
            expectEquals(ApplicationState::matchDeviceIndex(devices, "sooperlooper (2)"), 2);
            expectEquals(ApplicationState::matchDeviceIndex(devices, "sooperlooper (1)"), 0);
            // the plain name still picks the first one, as before
            expectEquals(ApplicationState::matchDeviceIndex(devices, "sooperlooper"), 0);
        }

        beginTest("Port matching prefers an exact name over a partial one");
        {
            Array<MidiDeviceInfo> devices;
            devices.add({"loopMidi Port 10", "id1"});
            devices.add({"loopMidi Port 1", "id2"});

            // the exact name wins even when an earlier port contains it
            expectEquals(ApplicationState::matchDeviceIndex(devices, "loopMidi Port 1"), 1);
            // partial matching still works, ignoring case
            expectEquals(ApplicationState::matchDeviceIndex(devices, "port 10"), 0);
            expectEquals(ApplicationState::matchDeviceIndex(devices, "LOOPMIDI"), 0);
            expectEquals(ApplicationState::matchDeviceIndex(devices, "nothing here"), -1);
        }
    }
};

static ParsingTests parsingTests;
