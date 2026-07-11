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

#include "MpeTestScenario.h"

void MpeTestScenario::step(const String& message)
{
    std::cout << message << " ";
    for (auto i = 0 ; i < 3; ++i)
    {
        Thread::sleep(100);
        std::cout << ".";
        std::cout.flush();
    }
    Thread::sleep(300);
    std::cout << std::endl;
    std::cout.flush();
}

void MpeTestScenario::send(ApplicationState& state)
{
    auto bend_messages = 1000;
    auto pressure_messages = 1000;
    auto timbre_messages = 1000;
    auto control_messages = 500;   // shorter sweeps for the global Manager Channel demos

    // CC74 (Timbre, the Third Dimension) has its neutral at 64 in the MPE
    // specification, so notes start from that center rather than from 0
    const auto timbre_neutral = 0x40;

    auto range = 15;
    step(String("MPE Zone 1 with ") + String(range) + String(" Member Channels"));
    state.sendRPN(1, 6, range << 7);
    
    auto mgr_pbsens = 7;
    step(String("Pitch Bend Sensitivity on Manager Channel to ") + String(mgr_pbsens) + " semitones");
    state.sendRPN(1, 0, mgr_pbsens << 7);
    
    auto note_pbsens = 48;
    step(String("Pitch Bend Sensitivity on Member Channels to ") + String(note_pbsens) + " semitones");
    state.sendRPN(2, 0, note_pbsens << 7);
    
    step("Major C triad C3 E3 G3 on Member Channels with neutral starting expression");
    
    state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(2, 0));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x3c, (uint8)0x60));

    state.sendMidiMessage(MidiMessage::pitchWheel(3, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(3, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(3, 0));
    state.sendMidiMessage(MidiMessage::noteOn(3, 0x40, (uint8)0x7f));

    state.sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(16, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(16, 0));
    state.sendMidiMessage(MidiMessage::noteOn(16, 0x43, (uint8)0x7f));
    
    Thread::sleep(2000);
    
    step("Pitch bend into different directions, resulting into G3 E4 C3");
    
    auto bend_interval = 7;
    auto ch02_pitch_target = + (0x1FFF * bend_interval / note_pbsens);
    auto ch03_pitch_target = + (0x1FFF * 12 / note_pbsens);
    auto ch16_pitch_target = - (0x1FFF * bend_interval / note_pbsens);
    for (auto i = 1; i <= bend_messages; ++i)
    {
        state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000 + (ch02_pitch_target * i) / bend_messages));
        state.sendMidiMessage(MidiMessage::pitchWheel(3, 0x2000 + (ch03_pitch_target * i) / bend_messages));
        state.sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000 + (ch16_pitch_target * i) / bend_messages));
        Thread::sleep(1);
    }
    
    Thread::sleep(2000);
    
    step("Independent pressure across different notes");
    
    auto ch02_last_pressure = 0;
    auto ch03_last_pressure = 0;
    auto ch16_last_pressure = 0;
    for (auto i = 0; i <= pressure_messages; ++i)
    {
        auto ch02_val = (0x7F * i) / pressure_messages;
        if (ch02_last_pressure != ch02_val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(2, ch02_val));
            ch02_last_pressure = ch02_val;
        }
        Thread::sleep(1);
    }
    for (auto i = 0; i <= pressure_messages; ++i)
    {
        auto ch02_val = 0x7F - (0x7F * i) / pressure_messages;
        if (ch02_last_pressure != ch02_val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(2, ch02_val));
            ch02_last_pressure = ch02_val;
        }
        auto ch03_val = (0x7F * i) / pressure_messages;
        if (ch03_last_pressure != ch03_val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(3, ch03_val));
            ch03_last_pressure = ch03_val;
        }
        Thread::sleep(1);
    }
    for (auto i = 0; i <= pressure_messages; ++i)
    {
        auto ch03_val = 0x7F - (0x7F * i) / pressure_messages;
        if (ch03_last_pressure != ch03_val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(3, ch03_val));
            ch03_last_pressure = ch03_val;
        }
        auto ch16_val = (0x7F * i) / pressure_messages;
        if (ch16_last_pressure != ch16_val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(16, ch16_val));
            ch16_last_pressure = ch16_val;
        }
        Thread::sleep(1);
    }
    for (auto i = 0; i <= pressure_messages; ++i)
    {
        auto ch16_val = 0x7F - (0x7F * i) / pressure_messages;
        if (ch16_last_pressure != ch16_val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(16, ch16_val));
            ch16_last_pressure = ch16_val;
        }
        Thread::sleep(1);
    }
    
    Thread::sleep(2000);
    
    step("Independent timbral motion across different notes");
    
    auto ch02_last_timbre = 0;
    auto ch03_last_timbre = 0;
    auto ch16_last_timbre = 0;
    for (auto i = 0; i <= timbre_messages; ++i)
    {
        auto ch02_val = (0x7F * i) / timbre_messages;
        if (ch02_last_timbre != ch02_val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, ch02_val));
            ch02_last_timbre = ch02_val;
        }
        Thread::sleep(1);
    }
    for (auto i = 0; i <= timbre_messages; ++i)
    {
        auto ch02_val = 0x7F - (0x7F * i) / timbre_messages;
        if (ch02_last_timbre != ch02_val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, ch02_val));
            ch02_last_timbre = ch02_val;
        }
        auto ch03_val = (0x7F * i) / timbre_messages;
        if (ch03_last_timbre != ch03_val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(3, 74, ch03_val));
            ch03_last_timbre = ch03_val;
        }
        Thread::sleep(1);
    }
    for (auto i = 0; i <= timbre_messages; ++i)
    {
        auto ch03_val = 0x7F - (0x7F * i) / timbre_messages;
        if (ch03_last_timbre != ch03_val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(3, 74, ch03_val));
            ch03_last_timbre = ch03_val;
        }
        auto ch16_val = (0x7F * i) / timbre_messages;
        if (ch16_last_timbre != ch16_val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(16, 74, ch16_val));
            ch16_last_timbre = ch16_val;
        }
        Thread::sleep(1);
    }
    for (auto i = 0; i <= timbre_messages; ++i)
    {
        auto ch16_val = 0x7F - (0x7F * i) / timbre_messages;
        if (ch16_last_timbre != ch16_val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(16, 74, ch16_val));
            ch16_last_timbre = ch16_val;
        }
        Thread::sleep(1);
    }
    
    Thread::sleep(2000);
    
    step("Manager Channel pitch bend transposes the whole held chord and returns");

    // the Manager Channel (channel 1 in the Lower Zone) affects every sounding
    // Member note at once, so this bends the entire chord up and back to center
    auto mgr_bend_target = (0x1FFF * 5) / mgr_pbsens;   // roughly +5 semitones
    for (auto i = 1; i <= control_messages; ++i)
    {
        state.sendMidiMessage(MidiMessage::pitchWheel(1, 0x2000 + (mgr_bend_target * i) / control_messages));
        Thread::sleep(1);
    }
    for (auto i = control_messages; i >= 0; --i)
    {
        state.sendMidiMessage(MidiMessage::pitchWheel(1, 0x2000 + (mgr_bend_target * i) / control_messages));
        Thread::sleep(1);
    }

    Thread::sleep(1000);

    step("Manager Channel modulation, expression and pressure applied to all notes");

    // modulation (CC 1) rising from 0 to full
    auto mgr_last = -1;
    for (auto i = 0; i <= control_messages; ++i)
    {
        auto val = (0x7F * i) / control_messages;
        if (mgr_last != val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(1, 1, val));
            mgr_last = val;
        }
        Thread::sleep(1);
    }
    // expression (CC 11) falling from full to 0
    mgr_last = -1;
    for (auto i = 0; i <= control_messages; ++i)
    {
        auto val = 0x7F - (0x7F * i) / control_messages;
        if (mgr_last != val)
        {
            state.sendMidiMessage(MidiMessage::controllerEvent(1, 11, val));
            mgr_last = val;
        }
        Thread::sleep(1);
    }
    // channel pressure rising from 0 to full
    mgr_last = -1;
    for (auto i = 0; i <= control_messages; ++i)
    {
        auto val = (0x7F * i) / control_messages;
        if (mgr_last != val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(1, val));
            mgr_last = val;
        }
        Thread::sleep(1);
    }

    Thread::sleep(2000);

    step("Release the active notes");

    state.sendMidiMessage(MidiMessage::noteOff(2, 0x3c, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(3, 0x40, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(16, 0x43, (uint8)0x40));

    // restore the Manager Channel to its defaults before continuing
    state.sendMidiMessage(MidiMessage::pitchWheel(1, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(1, 1, 0x00));
    state.sendMidiMessage(MidiMessage::controllerEvent(1, 11, 0x7f));
    state.sendMidiMessage(MidiMessage::channelPressureChange(1, 0));

    Thread::sleep(2000);

    step("Pitch bend to the extreme low and high limits on a Member Channel");

    // a full-scale bend to both rails checks clamping at the sensitivity limit
    state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(2, 0));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x3c, (uint8)0x60));
    for (auto i = 0; i <= bend_messages; ++i)
    {
        state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000 - (0x2000 * i) / bend_messages));
        Thread::sleep(1);
    }
    Thread::sleep(500);
    for (auto i = 0; i <= bend_messages; ++i)
    {
        state.sendMidiMessage(MidiMessage::pitchWheel(2, (0x3FFF * i) / bend_messages));
        Thread::sleep(1);
    }
    Thread::sleep(500);
    state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000));
    state.sendMidiMessage(MidiMessage::noteOff(2, 0x3c, (uint8)0x40));

    Thread::sleep(2000);

    step("Multiple notes stacked on a single Member Channel share its expression");

    // more notes than channels forces several onto one channel; they can no
    // longer be shaped independently and move together with that channel
    state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(2, 0));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x3c, (uint8)0x60));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x40, (uint8)0x60));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x43, (uint8)0x60));
    Thread::sleep(1000);
    auto shared_last = -1;
    for (auto i = 0; i <= pressure_messages; ++i)
    {
        auto val = (0x7F * i) / pressure_messages;
        if (shared_last != val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(2, val));
            shared_last = val;
        }
        Thread::sleep(1);
    }
    for (auto i = 0; i <= pressure_messages; ++i)
    {
        auto val = 0x7F - (0x7F * i) / pressure_messages;
        if (shared_last != val)
        {
            state.sendMidiMessage(MidiMessage::channelPressureChange(2, val));
            shared_last = val;
        }
        Thread::sleep(1);
    }
    state.sendMidiMessage(MidiMessage::noteOff(2, 0x3c, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(2, 0x40, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(2, 0x43, (uint8)0x40));
    
    Thread::sleep(2000);
    
    step("Different Major C triad G3 E4 C3 on Member Channels with neutral starting expression");
    
    state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(2, 0));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x43, (uint8)0x60));

    state.sendMidiMessage(MidiMessage::pitchWheel(3, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(3, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(3, 0));
    state.sendMidiMessage(MidiMessage::noteOn(3, 0x4c, (uint8)0x7f));

    state.sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(16, 74, timbre_neutral));
    state.sendMidiMessage(MidiMessage::channelPressureChange(16, 0));
    state.sendMidiMessage(MidiMessage::noteOn(16, 0x3c, (uint8)0x7f));
    
    Thread::sleep(2000);
    
    note_pbsens = 96;
    
    step(String("Pitch Bend Sensitivity on Member Channels to ") + String(note_pbsens) + " semitones");
    state.sendRPN(2, 0, note_pbsens << 7);
    
    Thread::sleep(2000);
    
    step("Pitch bend back to the original Major C triad C3 E3 G3");
    
    ch02_pitch_target = - (0x1FFF * bend_interval / note_pbsens);
    ch03_pitch_target = - (0x1FFF * 12 / note_pbsens);
    ch16_pitch_target = + (0x1FFF * bend_interval / note_pbsens);
    for (auto i = 1; i <= bend_messages; ++i)
    {
        state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000 + (ch02_pitch_target * i) / bend_messages));
        state.sendMidiMessage(MidiMessage::pitchWheel(3, 0x2000 + (ch03_pitch_target * i) / bend_messages));
        state.sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000 + (ch16_pitch_target * i) / bend_messages));
        Thread::sleep(1);
    }
    
    Thread::sleep(2000);
    
    step("Release the active notes");
    
    state.sendMidiMessage(MidiMessage::noteOff(2, 0x43, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(3, 0x4c, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(16, 0x3c, (uint8)0x40));
}
