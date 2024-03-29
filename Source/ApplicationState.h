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

#include "ApplicationCommand.h"
#include "MpeProfileNegotiation.h"

class ApplicationState : public MidiInputCallback, public ci::DeviceMessageHandler
{
public:
    ApplicationState();
    void initialise(JUCEApplicationBase& app);
        
    void openOutputDevice(const String& name);
    void openInputDevice(const String& name);
    void virtualDevice(const String& name);
    void parseFile(File file);
    void sendMidiMessage(MidiMessage&& msg);
    void sendRPN(int channel, int number, int value);
    void negotiateMpeProfile(const String& name, int manager, int members);
    
    uint8 asNoteNumber(String value);
    uint8 asDecOrHex7BitValue(String value);
    uint16 asDecOrHex14BitValue(String value);
    int asDecOrHexIntValue(String value);
    
    static uint8 limit7Bit(int value);
    static uint16 limit14Bit(int value);
    
    void printVersion();
    void printUsage();
    
    int channel_;
    int octaveMiddleC_;
    
private:
    ApplicationCommand* findApplicationCommand(const String& param);
    StringArray parseLineAsParameters(const String& line);
    
    bool tryToConnectMidiInput(const String& name);
    bool isMidiInDeviceAvailable(const String& name);
    void handleIncomingMidiMessage(MidiInput*, const MidiMessage& msg) override;
    void processMessage(ump::BytesOnGroup) override;

    bool isNumeric(const String& string);
    int64_t parseTimestamp(const String& param);

    void executeCurrentCommand();
    void handleVarArgCommand();
    void parseParameters(StringArray& parameters);

    Array<ApplicationCommand> commands_;
    ApplicationCommand currentCommand_;

    String midiOutName_;
    std::unique_ptr<MidiOutput> midiOut_;
    
    std::unique_ptr<MidiInput> midiIn_;
    
    std::unique_ptr<MpeProfileNegotiation> mpeProfile_;
    
    String fullMidiInName_;
    bool useHexadecimalsByDefault_;
    uint32 lastTimeStampCounter_;
    int64_t lastTimeStamp_;
};

