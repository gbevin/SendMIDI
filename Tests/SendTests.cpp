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

// Exercises the messages each command produces, by parsing a real command line
// and collecting the MIDI it would send (no device opened). The expected bytes
// are the objective MIDI encoding, so the tests pin the wire format, not just
// whatever the code happens to do.
class SendTests : public UnitTest
{
public:
    SendTests() : UnitTest("Send", "Send") {}

    void runTest() override
    {
        beginTest("Channel voice messages carry the right bytes on the current channel");
        {
            ApplicationState s;
            auto on = s.collectLine("on 60 100");
            expectEquals(on.size(), 1);
            expect(on[0].isNoteOn());
            expectEquals(on[0].getChannel(), 1);
            expectEquals(on[0].getNoteNumber(), 60);
            expectEquals((int)on[0].getVelocity(), 100);

            auto off = ApplicationState().collectLine("ch 5 off 60 64");
            expectEquals(off.size(), 1);
            expect(off[0].isNoteOff());
            expectEquals(off[0].getChannel(), 5);
            expectEquals(off[0].getNoteNumber(), 60);
            expectEquals((int)off[0].getVelocity(), 64);

            auto pp = ApplicationState().collectLine("pp 60 50");
            expect(pp[0].isAftertouch());
            expectEquals(pp[0].getNoteNumber(), 60);
            expectEquals(pp[0].getAfterTouchValue(), 50);

            auto cc = ApplicationState().collectLine("cc 74 55");
            expect(cc[0].isController());
            expectEquals(cc[0].getControllerNumber(), 74);
            expectEquals(cc[0].getControllerValue(), 55);

            auto pc = ApplicationState().collectLine("pc 42");
            expect(pc[0].isProgramChange());
            expectEquals(pc[0].getProgramChangeNumber(), 42);

            auto cp = ApplicationState().collectLine("cp 90");
            expect(cp[0].isChannelPressure());
            expectEquals(cp[0].getChannelPressureValue(), 90);

            auto pb = ApplicationState().collectLine("pb 8192");
            expect(pb[0].isPitchWheel());
            expectEquals(pb[0].getPitchWheelValue(), 8192);
        }

        beginTest("14-bit CC splits into MSB and LSB controllers");
        {
            auto m = ApplicationState().collectLine("cc14 1 1000");
            expectEquals(m.size(), 2);
            expect(m[0].isController());
            expectEquals(m[0].getControllerNumber(), 1);
            expectEquals(m[0].getControllerValue(), 1000 >> 7);          // MSB = 7
            expectEquals(m[1].getControllerNumber(), 1 + 32);            // LSB controller
            expectEquals(m[1].getControllerValue(), 1000 & 0x7f);        // LSB = 104
        }

        beginTest("RPN emits select, data entry and closing null");
        {
            auto m = ApplicationState().collectLine("rpn 1 200");
            expectEquals(m.size(), 6);
            auto isCc = [&](int i, int num, int val)
            {
                expect(m[i].isController());
                expectEquals(m[i].getControllerNumber(), num);
                expectEquals(m[i].getControllerValue(), val);
            };
            isCc(0, 101, 1 >> 7);                          // RPN MSB of param 1 -> 0
            isCc(1, 100, 1 & 0x7f);                        // RPN LSB of param 1 -> 1
            isCc(2, 6,  200 >> 7);                          // data entry MSB -> 1
            isCc(3, 38, 200 & 0x7f);                        // data entry LSB -> 72
            isCc(4, 101, 0x7f);                             // null RPN
            isCc(5, 100, 0x7f);
        }

        beginTest("NRPN selects with 99/98 and enters data on 6/38");
        {
            auto m = ApplicationState().collectLine("nrpn 1000 200");
            expectEquals(m.size(), 6);
            expectEquals(m[0].getControllerNumber(), 99);
            expectEquals(m[0].getControllerValue(), 1000 >> 7);   // 7
            expectEquals(m[1].getControllerNumber(), 98);
            expectEquals(m[1].getControllerValue(), 1000 & 0x7f); // 104
            expectEquals(m[2].getControllerNumber(), 6);
            expectEquals(m[2].getControllerValue(), 200 >> 7);    // 1
            expectEquals(m[3].getControllerNumber(), 38);
            expectEquals(m[3].getControllerValue(), 200 & 0x7f);  // 72
        }

        beginTest("MPE configuration declares the member range via RPN 6");
        {
            // lower zone uses master channel 1; range is packed into the RPN MSB
            auto m = ApplicationState().collectLine("mpe 1 5");
            expectEquals(m.size(), 6);
            expectEquals(m[0].getChannel(), 1);
            expectEquals(m[0].getControllerNumber(), 101);     // RPN MSB
            expectEquals(m[0].getControllerValue(), 0);        // param 6 -> MSB 0
            expectEquals(m[1].getControllerNumber(), 100);
            expectEquals(m[1].getControllerValue(), 6);        // param 6 -> LSB 6
            expectEquals(m[2].getControllerNumber(), 6);
            expectEquals(m[2].getControllerValue(), 5);        // (5 << 7) >> 7 = 5

            // upper zone uses master channel 16
            auto u = ApplicationState().collectLine("mpe 2 4");
            expectEquals(u[0].getChannel(), 16);
        }

        beginTest("System real-time and common messages");
        {
            expect(ApplicationState().collectLine("mc")[0].isMidiClock());
            expect(ApplicationState().collectLine("start")[0].isMidiStart());
            expect(ApplicationState().collectLine("stop")[0].isMidiStop());
            expect(ApplicationState().collectLine("cont")[0].isMidiContinue());
            expect(ApplicationState().collectLine("as")[0].isActiveSense());

            auto rst = ApplicationState().collectLine("rst");
            expectEquals(rst[0].getRawDataSize(), 1);
            expectEquals((int)rst[0].getRawData()[0], 0xff);

            auto tun = ApplicationState().collectLine("tun");
            expectEquals((int)tun[0].getRawData()[0], 0xf6);

            auto spp = ApplicationState().collectLine("spp 100");
            expect(spp[0].isSongPositionPointer());
            expectEquals(spp[0].getSongPositionPointerMidiBeat(), 100);

            auto ss = ApplicationState().collectLine("ss 5");
            expectEquals((int)ss[0].getRawData()[0], 0xf3);
            expectEquals((int)ss[0].getRawData()[1], 5);

            auto tc = ApplicationState().collectLine("tc 1 5");
            expect(tc[0].isQuarterFrame());
            expectEquals(tc[0].getQuarterFrameSequenceNumber(), 1);
            expectEquals(tc[0].getQuarterFrameValue(), 5);
        }

        beginTest("SysEx and raw MIDI carry the given bytes");
        {
            auto syx = ApplicationState().collectLine("syx 1 2 3");
            const MidiMessage syxMsg = syx[0];
            expect(syxMsg.isSysEx());
            expectEquals(syxMsg.getSysExDataSize(), 3);
            const uint8* data = syxMsg.getSysExData();
            expectEquals((int)data[0], 1);
            expectEquals((int)data[1], 2);
            expectEquals((int)data[2], 3);

            // 144 = 0x90 note-on status, then note and velocity (decimal by default)
            auto raw = ApplicationState().collectLine("raw 144 60 100");
            expect(raw[0].isNoteOn());
            expectEquals(raw[0].getNoteNumber(), 60);
            expectEquals((int)raw[0].getVelocity(), 100);
            // the same bytes in hexadecimal
            auto rawHex = ApplicationState().collectLine("hex raw 90 3C 64");
            expect(rawHex[0].isNoteOn());
            expectEquals(rawHex[0].getNoteNumber(), 0x3C);
            expectEquals((int)rawHex[0].getVelocity(), 0x64);
        }

        beginTest("Panic sends pedal/all-off, resets and every note off on all channels");
        {
            auto m = ApplicationState().collectLine("panic");
            // per channel: sustain off, all-sound-off, reset-all-controllers,
            // all-notes-off, pitch bend recenter, 128 note offs
            expectEquals(m.size(), 16 * (5 + 128));
            expect(m[0].isController());
            expectEquals(m[0].getControllerNumber(), 64);    // sustain off
            expectEquals(m[0].getChannel(), 1);
            expectEquals(m[2].getControllerNumber(), 121);   // reset all controllers
            expect(m[4].isPitchWheel());                     // pitch bend recentered
            expectEquals(m[4].getPitchWheelValue(), 0x2000);
            expect(m[m.size() - 1].isNoteOff());
            expectEquals(m[m.size() - 1].getChannel(), 16);
        }

        beginTest("Out-of-range channel is rejected and leaves the channel unchanged");
        {
            // channel 0 and >16 are invalid, so the note still sends on the default channel 1
            auto low = ApplicationState().collectLine("ch 0 on 60 100");
            expectEquals(low.size(), 1);
            expectEquals(low[0].getChannel(), 1);

            auto high = ApplicationState().collectLine("ch 20 on 60 100");
            expectEquals(high.size(), 1);
            expectEquals(high[0].getChannel(), 1);

            // a valid channel is applied
            auto ok = ApplicationState().collectLine("ch 10 on 60 100");
            expectEquals(ok[0].getChannel(), 10);
        }

        beginTest("Time code clamps type to 0-7 and value to 0-15");
        {
            auto m = ApplicationState().collectLine("tc 9 20");
            expect(m[0].isQuarterFrame());
            expectEquals(m[0].getQuarterFrameSequenceNumber(), 7);
            expectEquals(m[0].getQuarterFrameValue(), 15);
        }

        beginTest("Channel, octave and hex settings carry across a command line");
        {
            // the channel set earlier applies to later messages
            auto m = ApplicationState().collectLine("ch 3 on 60 10 on 62 20");
            expectEquals(m.size(), 2);
            expectEquals(m[0].getChannel(), 3);
            expectEquals(m[1].getChannel(), 3);
            expectEquals(m[1].getNoteNumber(), 62);

            // moving middle C down an octave lowers a note name by 12
            auto omc = ApplicationState().collectLine("omc 4 on C3 100");
            expectEquals(omc[0].getNoteNumber(), 48);

            // hex mode and the H suffix both parse hexadecimal
            auto hex = ApplicationState().collectLine("hex cc 40 7f");
            expectEquals(hex[0].getControllerNumber(), 0x40);
            expectEquals(hex[0].getControllerValue(), 0x7f);

            auto suffix = ApplicationState().collectLine("cc 64H 64H");
            expectEquals(suffix[0].getControllerNumber(), 0x64);
            expectEquals(suffix[0].getControllerValue(), 0x64);
        }
    }
};

static SendTests sendTests;
