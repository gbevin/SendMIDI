/*
 * This file is part of SendMIDI.
 * Copyright (command) 2017 Uwyn SPRL.  http://www.uwyn.com
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

#include <sstream>

enum CommandIndex
{
    NONE,
    LIST,
    PANIC,
    DEVICE,
    TXTFILE,
    CHANNEL,
    NOTE_ON,
    NOTE_OFF,
    POLY_PRESSURE,
    CONTROL_CHANGE,
    PROGRAM_CHANGE,
    CHANNEL_PRESSURE,
    PITCH_BEND,
    RPN,
    NRPN,
    START,
    STOP,
    CONTINUE,
    SONG_POSITION,
    SONG_SELECT
};

struct ApplicationCommand
{
    static ApplicationCommand Dummy()
    {
        return {"", "", NONE, 0, "", ""};
    }
    
    void clear()
    {
        param_ = "";
        command_ = NONE;
        expectedOptions_ = 0;
        optionsDescription_ = "";
        commandDescription_ = "";
        opts_.clear();
    }
    
    String param_;
    String altParam_;
    CommandIndex command_;
    unsigned expectedOptions_;
    String optionsDescription_;
    String commandDescription_;
    StringArray opts_;
};

class sendMidiApplication  : public JUCEApplication
{
public:
    sendMidiApplication()
    {
        commands_.add({"dev",   "device",                   DEVICE,             1, "name",           "Set the name of the MIDI output port (REQUIRED)"});
        commands_.add({"list",  "",                         LIST,               0, "",               "Lists the MIDI output ports"});
        commands_.add({"panic", "",                         PANIC,              0, "",               "Sends all possible Note Offs and relevant panic CCs"});
        commands_.add({"file",  "",                         TXTFILE,            1, "path",           "Loads commands from the specified program file"});
        commands_.add({"ch",    "channel",                  CHANNEL,            1, "number",         "Set MIDI channel for the commands (1-16), defaults to 1"});
        commands_.add({"on",    "note-on",                  NOTE_ON,            2, "note velocity",  "Send Note On with note (0-127) and velocity (0-127)"});
        commands_.add({"off",   "note-off",                 NOTE_OFF,           2, "note velocity",  "Send Note Off with note (0-127) and velocity (0-127)"});
        commands_.add({"pp",    "poly-pressure",            POLY_PRESSURE,      2, "note value",     "Send Poly Pressure with note (0-127) and pressure (0-127)"});
        commands_.add({"cc",    "continuous-controller",    CONTROL_CHANGE,     2, "number value",   "Send Continuous Controller (0-127) with value (0-127)"});
        commands_.add({"pc",    "program-change",           PROGRAM_CHANGE,     1, "number",         "Send Program Change number (0-127)"});
        commands_.add({"cp",    "channel-pressure",         CHANNEL_PRESSURE,   1, "value",          "Send Channel Pressure value (0-127)"});
        commands_.add({"pb",    "pitch-bend",               PITCH_BEND,         1, "value",          "Send Pitch Bend value (0-16383)"});
        commands_.add({"rpn",   "",                         RPN,                2, "number value",   "Send RPN number (0-16383) with value (0-16383)"});
        commands_.add({"nrpn",  "",                         NRPN,               2, "number value",   "Send NRPN number (0-16383) with value (0-16383)"});
        commands_.add({"start", "",                         START,              0, "",               "Start the current sequence playing"});
        commands_.add({"stop",  "",                         STOP,               0, "",               "Stop the current sequence"});
        commands_.add({"cont",  "continue",                 CONTINUE,           0, "",               "Continue the current sequence"});
        commands_.add({"spp",   "song-position",            SONG_POSITION,      1, "beats",          "Send Song Position Pointer with beat (0-16383)"});
        commands_.add({"ss",    "song-select",              SONG_SELECT,        1, "number",         "Send Song Select with song number (0-127)"});
        
        channel_ = 1;
    }
    
    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    
    void initialise(const String&) override
    {
        StringArray cmdLineParams(getCommandLineParameterArray());
        parseParameters(cmdLineParams);
        
        if (cmdLineParams.contains("--"))
        {
            while (std::cin)
            {
                std::string line;
                getline(std::cin, line);
                StringArray params = parseLineAsParameters(line);
                parseParameters(params);
            }
        }
        
        if (cmdLineParams.isEmpty())
        {
            printUsage();
        }
        
        systemRequestedQuit();
    }
    
    void shutdown() override {}
    void anotherInstanceStarted(const String&) override {}
    
private:
    ApplicationCommand* findApplicationCommand(const String& param)
    {
        for (auto&& cmd : commands_)
        {
            if (cmd.param_ == param || cmd.altParam_ == param)
            {
                return &cmd;
            }
        }
        return nullptr;
    }
    
    StringArray parseLineAsParameters(const String& line)
    {
        StringArray parameters;
        if (!line.startsWith("#"))
        {
            StringArray tokens;
            tokens.addTokens(line, true);
            tokens.removeEmptyStrings(true);
            for (String token : tokens)
            {
                parameters.add(token.trimCharactersAtStart("\"").trimCharactersAtEnd("\""));
            }
        }
        return parameters;
    }
    
    void parseParameters(StringArray& parameters)
    {
        ApplicationCommand currentCommand = ApplicationCommand::Dummy();
        for (String param : parameters)
        {
            ApplicationCommand* cmd = findApplicationCommand(param);
            if (cmd)
            {
                currentCommand = *cmd;
            }
            else if (currentCommand.command_ == NONE)
            {
                File file = File::getCurrentWorkingDirectory().getChildFile(param);
                if (file.existsAsFile())
                {
                    parseFile(file);
                }
            }
            else
            {
                if (currentCommand.expectedOptions_ > 0)
                {
                    currentCommand.opts_.add(param);
                    currentCommand.expectedOptions_ -= 1;
                }
            }
            
            if (currentCommand.expectedOptions_ == 0)
            {
                executeCommand(currentCommand);
            }
        }
    }
    
    void parseFile(File file)
    {
        StringArray parameters;
        
        StringArray lines;
        file.readLines(lines);
        for (String line : lines)
        {
            parameters.addArray(parseLineAsParameters(line));
        }
        
        parseParameters(parameters);
    }
    
    void sendMidiMessage(MidiMessage&& msg)
    {
        if (midiOut_)
        {
            midiOut_->sendMessageNow(msg);
        }
        else
        {
            static bool missingOutputPortWarningPrinted = false;
            if (!missingOutputPortWarningPrinted)
            {
                std::cout << "No valid MIDI output port was specified for some of the messages" << std::endl;
                missingOutputPortWarningPrinted = true;
            }
        }
    }
    
    void executeCommand(ApplicationCommand& cmd)
    {
        switch (cmd.command_)
        {
            case NONE:
                break;
            case LIST:
                for (auto&& device : MidiOutput::getDevices())
                {
                    std::cout << device << std::endl;
                }
                break;
            case DEVICE:
            {
                midiOut_ = nullptr;
                midiOutName_ = cmd.opts_[0];
                int index = MidiOutput::getDevices().indexOf(midiOutName_);
                if (index >= 0)
                {
                    midiOut_ = MidiOutput::openDevice(index);
                }
                else
                {
                    StringArray devices = MidiOutput::getDevices();
                    for (int i = 0; i < devices.size(); ++i)
                    {
                        if (devices[i].containsIgnoreCase(midiOutName_))
                        {
                            midiOut_ = MidiOutput::openDevice(i);
                            midiOutName_ = devices[i];
                            break;
                        }
                    }
                }
                if (midiOut_ == nullptr)
                {
                    std::cout << "Couldn't find MIDI output port \"" << midiOutName_ << "\"" << std::endl;
                }
                break;
            }
            case PANIC:
            {
                for (int ch = 1; ch <= 16; ++ch)
                {
                    sendMidiMessage(MidiMessage::controllerEvent(ch, 64, 0));
                    sendMidiMessage(MidiMessage::controllerEvent(ch, 120, 0));
                    sendMidiMessage(MidiMessage::controllerEvent(ch, 123, 0));
                    for (int note = 0; note <= 127; ++note)
                    {
                        sendMidiMessage(MidiMessage::noteOff(ch, note, (uint8)0));
                    }
                }
                break;
            }
            case TXTFILE:
            {
                String path(cmd.opts_[0]);
                File file = File::getCurrentWorkingDirectory().getChildFile(path);
                if (file.existsAsFile())
                {
                    parseFile(file);
                }
                else
                {
                    std::cout << "Couldn't find file \"" << path << "\"" << std::endl;
                }
                break;
            }
            case CHANNEL:
                channel_ = limit7Bit(cmd.opts_[0].getIntValue());
                break;
            case NOTE_ON:
                sendMidiMessage(MidiMessage::noteOn(channel_,
                                                    limit7Bit(cmd.opts_[0].getIntValue()),
                                                    limit7Bit(cmd.opts_[1].getIntValue())));
                break;
            case NOTE_OFF:
                sendMidiMessage(MidiMessage::noteOff(channel_,
                                                     limit7Bit(cmd.opts_[0].getIntValue()),
                                                     limit7Bit(cmd.opts_[1].getIntValue())));
                break;
            case POLY_PRESSURE:
                sendMidiMessage(MidiMessage::aftertouchChange(channel_,
                                                              limit7Bit(cmd.opts_[0].getIntValue()),
                                                              limit7Bit(cmd.opts_[1].getIntValue())));
                break;
            case CONTROL_CHANGE:
                sendMidiMessage(MidiMessage::controllerEvent(channel_,
                                                             limit7Bit(cmd.opts_[0].getIntValue()),
                                                             limit7Bit(cmd.opts_[1].getIntValue())));
                break;
            case PROGRAM_CHANGE:
                sendMidiMessage(MidiMessage::programChange(channel_,
                                                           limit7Bit(cmd.opts_[0].getIntValue())));
                break;
            case CHANNEL_PRESSURE:
                sendMidiMessage(MidiMessage::channelPressureChange(channel_,
                                                                   limit7Bit(cmd.opts_[0].getIntValue())));
                break;
            case PITCH_BEND:
                sendMidiMessage(MidiMessage::pitchWheel(channel_,
                                                        limit14Bit(cmd.opts_[0].getIntValue())));
                break;
            case NRPN:
            {
                int number = limit14Bit(cmd.opts_[0].getIntValue());
                int value = limit14Bit(cmd.opts_[1].getIntValue());
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 99, number >> 7));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 98, number & 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 6, value >> 7));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 38, value & 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 101, 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 100, 0x7f));
                break;
            }
            case RPN:
            {
                int number = limit14Bit(cmd.opts_[0].getIntValue());
                int value = limit14Bit(cmd.opts_[1].getIntValue());
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 101, number >> 7));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 100, number & 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 6, value >> 7));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 38, value & 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 101, 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 100, 0x7f));
                break;
            }
            case START:
                sendMidiMessage(MidiMessage::midiStart());
                break;
            case STOP:
                sendMidiMessage(MidiMessage::midiStop());
                break;
            case CONTINUE:
                sendMidiMessage(MidiMessage::midiContinue());
                break;
            case SONG_POSITION:
                sendMidiMessage(MidiMessage::songPositionPointer(limit14Bit(cmd.opts_[0].getIntValue())));
                break;
            case SONG_SELECT:
                sendMidiMessage(MidiMessage(0xf3, limit7Bit(cmd.opts_[0].getIntValue())));
                break;
        }
        
        cmd.clear();
    }
    
    static uint8 limit7Bit(int value)
    {
        return (uint8)jlimit(0, 0x7f, value);
    }
    
    static uint16 limit14Bit(int value)
    {
        return (uint16)jlimit(0, 0x3fff, value);
    }
    
    void printUsage()
    {
        std::cout << ProjectInfo::projectName << " v" << ProjectInfo::versionString << std::endl;
        std::cout << "https://github.com/gbevin/SendMIDI" << std::endl << std::endl;
        std::cout << "Usage: " << ProjectInfo::projectName << " [ commands ] [ programfile ] [ -- ]" << std::endl << std::endl
                  << "Commands:" << std::endl;
        for (auto&& cmd : commands_)
        {
            std::cout << "  " << cmd.param_.paddedRight(' ', 5);
            if (cmd.optionsDescription_.isNotEmpty())
            {
                std::cout << " " << cmd.optionsDescription_.paddedRight(' ', 13);
            }
            else
            {
                std::cout << "              ";
            }
            std::cout << "  " << cmd.commandDescription_;
            std::cout << std::endl;
        }
        std::cout << "  --                   Read commands from standard input until it's closed" << std::endl;
        std::cout << std::endl;
        std::cout << "Alternatively, you can use the following long versions of the commands:" << std::endl;
        String line = " ";
        for (auto&& cmd : commands_)
        {
            if (cmd.altParam_.isNotEmpty())
            {
                if (line.length() + cmd.altParam_.length() + 1 >= 80)
                {
                    std::cout << line << std::endl;
                    line = " ";
                }
                line << " " << cmd.altParam_;
            }
        }
        std::cout << line << std::endl << std::endl;
        std::cout << "The MIDI device name doesn't have to be an exact match." << std::endl;
        std::cout << "If SendMIDI can't find the exact name that was specified, it will pick the first" << std::endl
                  << "MIDI output port that contains the provided text, irrespective of case." << std::endl;
        std::cout << std::endl;
    }
    
    Array<ApplicationCommand> commands_;
    int channel_;
    String midiOutName_;
    ScopedPointer<MidiOutput> midiOut_;
};

START_JUCE_APPLICATION (sendMidiApplication)
