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

#pragma once

#include "JuceHeader.h"

enum CommandIndex
{
    NONE,
    LIST,
    PANIC,
    DEVICE,
    VIRTUAL,
    TXTFILE,
    DECIMAL,
    HEXADECIMAL,
    CHANNEL,
    OCTAVE_MIDDLE_C,
    NOTE_ON,
    NOTE_OFF,
    POLY_PRESSURE,
    CONTROL_CHANGE,
    CONTROL_CHANGE_14BIT,
    PROGRAM_CHANGE,
    CHANNEL_PRESSURE,
    PITCH_BEND,
    RPN,
    NRPN,
    CLOCK,
    MIDI_CLOCK,
    START,
    STOP,
    CONTINUE,
    ACTIVE_SENSING,
    RESET,
    SYSTEM_EXCLUSIVE,
    SYSTEM_EXCLUSIVE_FILE,
    TIME_CODE,
    SONG_POSITION,
    SONG_SELECT,
    TUNE_REQUEST,
    MPE_CONFIGURATION,
    MPE_PROFILE,
    MPE_TEST,
    RAW_MIDI
};

class ApplicationState;

struct ApplicationCommand
{
    static ApplicationCommand Dummy();
    
    void clear();
    void execute(ApplicationState& state);
    
    String param_;
    String altParam_;
    CommandIndex command_;
    int expectedOptions_;
    StringArray optionsDescriptions_;
    StringArray commandDescriptions_;
    StringArray opts_;
};
