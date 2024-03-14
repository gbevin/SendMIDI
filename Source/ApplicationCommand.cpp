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

#include "ApplicationCommand.h"

#include "ApplicationState.h"
#include "MpeTestScenario.h"

static const String& DEFAULT_VIRTUAL_NAME = "SendMIDI";

inline float sign(float value)
{
    return (float)(value > 0.) - (value < 0.);
}

ApplicationCommand ApplicationCommand::Dummy()
{
    return {"", "", NONE, 0, {""}, {""}};
}

void ApplicationCommand::clear()
{
    param_ = "";
    command_ = NONE;
    expectedOptions_ = 0;
    optionsDescriptions_ = StringArray({""});
    commandDescriptions_ = StringArray({""});
    opts_.clear();
}

void ApplicationCommand::execute(ApplicationState& state)
{
    switch (command_)
    {
        case NONE:
            break;
        case LIST:
            for (auto&& device : MidiOutput::getAvailableDevices())
            {
                std::cout << device.name << std::endl;
            }
            break;
        case DEVICE:
        {
            state.openOutputDevice(opts_[0]);
            break;
        }
        case VIRTUAL:
        {
            auto name = DEFAULT_VIRTUAL_NAME;
            if (opts_.size())
            {
                name = opts_[0];
            }
            state.virtualDevice(name);
            break;
        }
        case PANIC:
        {
            for (auto ch = 1; ch <= 16; ++ch)
            {
                state.sendMidiMessage(MidiMessage::controllerEvent(ch, 64, 0));
                state.sendMidiMessage(MidiMessage::controllerEvent(ch, 120, 0));
                state.sendMidiMessage(MidiMessage::controllerEvent(ch, 123, 0));
                for (auto note = 0; note <= 127; ++note)
                {
                    state.sendMidiMessage(MidiMessage::noteOff(ch, note, (uint8)0));
                }
            }
            break;
        }
        case TXTFILE:
        {
            auto path(opts_[0]);
            auto file = File::getCurrentWorkingDirectory().getChildFile(path);
            if (file.existsAsFile())
            {
                state.parseFile(file);
            }
            else
            {
                std::cerr << "Couldn't find file \"" << path << "\"" << std::endl;
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
            }
            break;
        }
        case DECIMAL:
        case HEXADECIMAL:
            // these are not commands but rather configuration options
            // allow them to be inlined anywhere by handling them immediately in the
            // parseParameters method
            break;
        case CHANNEL:
            state.channel_ = state.asDecOrHex7BitValue(opts_[0]);
            break;
        case OCTAVE_MIDDLE_C:
            state.octaveMiddleC_ = state.asDecOrHex7BitValue(opts_[0]);
            break;
        case NOTE_ON:
            state.sendMidiMessage(MidiMessage::noteOn(state.channel_,
                                                      state.asNoteNumber(opts_[0]),
                                                      state.asDecOrHex7BitValue(opts_[1])));
            break;
        case NOTE_OFF:
            state.sendMidiMessage(MidiMessage::noteOff(state.channel_,
                                                       state.asNoteNumber(opts_[0]),
                                                       state.asDecOrHex7BitValue(opts_[1])));
            break;
        case POLY_PRESSURE:
            state.sendMidiMessage(MidiMessage::aftertouchChange(state.channel_,
                                                                state.asNoteNumber(opts_[0]),
                                                                state.asDecOrHex7BitValue(opts_[1])));
            break;
        case CONTROL_CHANGE:
            state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_,
                                                               state.asDecOrHex7BitValue(opts_[0]),
                                                               state.asDecOrHex7BitValue(opts_[1])));
            break;
        case CONTROL_CHANGE_14BIT:
        {
            auto number = state.asDecOrHex7BitValue(opts_[0]);
            if (number >= 32)
            {
                std::cerr << "Can't send 14bit MIDI CC for number " << number << " (it has to be smaller than 32)" << std::endl;
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
            }
            else
            {
                auto value = state.asDecOrHex14BitValue(opts_[1]);
                state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, number, (value >> 7) & 0x7f));
                state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, number + 32, value & 0x7f));
            }
            break;
        }
        case PROGRAM_CHANGE:
            state.sendMidiMessage(MidiMessage::programChange(state.channel_,
                                                             state.asDecOrHex7BitValue(opts_[0])));
            break;
        case CHANNEL_PRESSURE:
            state.sendMidiMessage(MidiMessage::channelPressureChange(state.channel_,
                                                                     state.asDecOrHex7BitValue(opts_[0])));
            break;
        case PITCH_BEND:
        {
            auto arg = opts_[0];
            auto value = 0;
            if (arg.containsChar('/'))
            {
                auto numerator = arg.upToFirstOccurrenceOf("/", false, true);
                auto denominator = arg.substring(numerator.length()+1);
                auto numVal = numerator.getFloatValue();
                auto denomVal = denominator.getFloatValue();
                if (fabs(numVal) > denomVal)
                {
                    numVal = sign(numVal)*denomVal;
                }
                value = state.limit14Bit(MidiMessage::pitchbendToPitchwheelPos(numVal, denomVal));
            }
            else
            {
                value = state.asDecOrHex14BitValue(arg);
            }
            state.sendMidiMessage(MidiMessage::pitchWheel(state.channel_, value));
            break;
        }
        case NRPN:
        {
            auto number = state.asDecOrHex14BitValue(opts_[0]);
            auto value = state.asDecOrHex14BitValue(opts_[1]);
            state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, 99, (number >> 7) & 0x7f));
            state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, 98, number & 0x7f));
            state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, 6, (value >> 7) & 0x7f));
            state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, 38, value & 0x7f));
            state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, 101, 0x7f));
            state.sendMidiMessage(MidiMessage::controllerEvent(state.channel_, 100, 0x7f));
            break;
        }
        case RPN:
        {
            state.sendRPN(state.channel_, state.asDecOrHexIntValue(opts_[0]), state.asDecOrHexIntValue(opts_[1]));
            break;
        }
        case CLOCK:
        {
            auto now = Time::getMillisecondCounter();
            auto bpm = float(jlimit(1, 999, state.asDecOrHexIntValue(opts_[0])));
            auto msPerTick = (60.f * 1000.f / bpm) / 24.f;
            state.sendMidiMessage(MidiMessage::midiClock());
            for (auto ticks = 1; ticks < 24 * 2; ++ticks)
            {
                Time::waitForMillisecondCounter(now + uint32(float(ticks) * msPerTick));
                state.sendMidiMessage(MidiMessage::midiClock());
            }
            break;
        }
        case MIDI_CLOCK:
            state.sendMidiMessage(MidiMessage::midiClock());
            break;
        case START:
            state.sendMidiMessage(MidiMessage::midiStart());
            break;
        case STOP:
            state.sendMidiMessage(MidiMessage::midiStop());
            break;
        case CONTINUE:
            state.sendMidiMessage(MidiMessage::midiContinue());
            break;
        case ACTIVE_SENSING:
            state.sendMidiMessage(MidiMessage(0xfe));
            break;
        case RESET:
            state.sendMidiMessage(MidiMessage(0xff));
            break;
        case TIME_CODE:
            state.sendMidiMessage(MidiMessage::quarterFrame(state.asDecOrHex14BitValue(opts_[0]), state.asDecOrHex14BitValue(opts_[1])));
            break;
        case SONG_POSITION:
            state.sendMidiMessage(MidiMessage::songPositionPointer(state.asDecOrHex14BitValue(opts_[0])));
            break;
        case SONG_SELECT:
            state.sendMidiMessage(MidiMessage(0xf3, state.asDecOrHex7BitValue(opts_[0])));
            break;
        case SYSTEM_EXCLUSIVE:
        {
            MemoryBlock mem(opts_.size(), true);
            for (auto i = 0; i < opts_.size(); ++i)
            {
                mem[i] = opts_[i].getIntValue();
            }
            state.sendMidiMessage(MidiMessage::createSysExMessage(mem.getData(), (int)mem.getSize()));
            break;
        }
        case SYSTEM_EXCLUSIVE_FILE:
        {
            auto path(opts_[0]);
            auto file = File::getCurrentWorkingDirectory().getChildFile(path);
            if (file.existsAsFile())
            {
                MemoryBlock mem;
                auto readSuccess = file.loadFileAsData(mem);
                if (readSuccess && mem.getSize() > 0)
                {
                    const uint8* data = (uint8*)mem.getData();
                    const auto data_size = (int)mem.getSize();
                    const auto buffer_size = 256;
                    state.sendMidiMessage(MidiMessage(data, data_size));
                    
                    std::cout << "Waiting for typical completion on DIN connections 0% (could be done sooner)" << std::flush;
                    for (auto i = 0; i < data_size; i += buffer_size)
                    {
                        auto length = std::min(buffer_size, data_size - i);
                        std::cout << "\rWaiting for typical completion on DIN connections " << (((i + length) * 100) / data_size) << "% (could be done sooner)" << std::flush;
                        // don't exceed 31250 baudrate (bits per second)
                        Thread::sleep((buffer_size * 8 * 1000) / 31250);
                    }
                    std::cout << "\rWaiting for typical completion on DIN connections 100%" << std::endl;
                    
                    Thread::sleep(((data_size / buffer_size) + 1) * (8 * 1000) / 31250);
                }
            }
            else
            {
                std::cerr << "Couldn't find file \"" << path << "\"" << std::endl;
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
            }
            break;
        }
        case TUNE_REQUEST:
            state.sendMidiMessage(MidiMessage(0xf6));
            break;
        case MPE_CONFIGURATION:
        {
            auto zone = jlimit(1, 2, state.asDecOrHexIntValue(opts_[0]));
            auto range = jlimit(0, 15, state.asDecOrHexIntValue(opts_[1]));
            state.sendRPN(zone == 1 ? 1 : 16, 6, range << 7);
            break;
        }
        case MPE_PROFILE:
        {
            auto input = opts_[0];
            auto channel = jlimit(1, 15, state.asDecOrHexIntValue(opts_[1]));
            state.negotiateMpeProfile(input, channel);
            break;
        }
        case MPE_TEST:
        {
            MpeTestScenario().send(state);
            break;
        }
        case RAW_MIDI:
        {
            MemoryBlock mem(opts_.size(), true);
            for (auto i = 0; i < opts_.size(); ++i)
            {
                mem[i] = (uint8)state.asDecOrHexIntValue(opts_[i]);
            }
            state.sendMidiMessage(MidiMessage(mem.getData(), (int)mem.getSize()));
        }
    }
    
    clear();
}
