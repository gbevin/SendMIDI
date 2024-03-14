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
    state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, 0x00));
    state.sendMidiMessage(MidiMessage::channelPressureChange(2, 0));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x3c, (uint8)0x60));
    
    state.sendMidiMessage(MidiMessage::pitchWheel(3, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(3, 74, 0x00));
    state.sendMidiMessage(MidiMessage::channelPressureChange(3, 0));
    state.sendMidiMessage(MidiMessage::noteOn(3, 0x40, (uint8)0x7f));
    
    state.sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(16, 74, 0x00));
    state.sendMidiMessage(MidiMessage::channelPressureChange(16, 0));
    state.sendMidiMessage(MidiMessage::noteOn(16, 0x43, (uint8)0x80));
    
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
    
    step("Release the active notes");
    
    state.sendMidiMessage(MidiMessage::noteOff(2, 0x3c, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(3, 0x40, (uint8)0x40));
    state.sendMidiMessage(MidiMessage::noteOff(16, 0x43, (uint8)0x40));
    
    Thread::sleep(2000);
    
    step("Different Major C triad G3 E4 C3 on Member Channels with neutral starting expression");
    
    state.sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(2, 74, 0x00));
    state.sendMidiMessage(MidiMessage::channelPressureChange(2, 0));
    state.sendMidiMessage(MidiMessage::noteOn(2, 0x43, (uint8)0x60));
    
    state.sendMidiMessage(MidiMessage::pitchWheel(3, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(3, 74, 0x00));
    state.sendMidiMessage(MidiMessage::channelPressureChange(3, 0));
    state.sendMidiMessage(MidiMessage::noteOn(3, 0x4c, (uint8)0x7f));
    
    state.sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000));
    state.sendMidiMessage(MidiMessage::controllerEvent(16, 74, 0x00));
    state.sendMidiMessage(MidiMessage::channelPressureChange(16, 0));
    state.sendMidiMessage(MidiMessage::noteOn(16, 0x3c, (uint8)0x80));
    
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
