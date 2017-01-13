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

enum CommandIndex
{
    NONE,
    LIST,
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
    NRPN
};

struct ApplicationCommand
{
    static ApplicationCommand Dummy()
    {
        return {"", NONE, 0, "", ""};
    }
    
    void clear()
    {
        param_ = "";
        command_ = NONE;
        expectedOptions_ = 0;
        optionsDescription_ = "";
        commandDescription_ = "";
        options_.clear();
    }
    
    String param_;
    CommandIndex command_;
    unsigned expectedOptions_;
    String optionsDescription_;
    String commandDescription_;
    StringArray options_;
};

class sendMidiApplication  : public JUCEApplication
{
public:
    sendMidiApplication()
    {
        commands_.add({"dev",  DEVICE,             1, "name",           "Set the name of the MIDI output port (REQUIRED)"});
        commands_.add({"list",  LIST,              0, "",               "Lists the MIDI output ports"});
        commands_.add({"file", TXTFILE,            1, "path",           "Loads commands from the specified file"});
        commands_.add({"ch",   CHANNEL,            1, "number",         "Set MIDI channel for the commands (1-16), defaults to 1"});
        commands_.add({"on",   NOTE_ON,            2, "note velocity",  "Send Note On with note (0-127) and velocity (0-127)"});
        commands_.add({"off",  NOTE_OFF,           2, "note velocity",  "Send Note Off with note (0-127) and velocity (0-127)"});
        commands_.add({"pp",   NOTE_OFF,           2, "note value",     "Send Poly Pressure with note (0-127) and pressure (0-127)"});
        commands_.add({"cc",   CONTROL_CHANGE,     2, "number value",   "Send Continuous Controller (0-127) with value (0-127)"});
        commands_.add({"pc",   PROGRAM_CHANGE,     1, "number",         "Send Program Change number (0-127)"});
        commands_.add({"cp",   CHANNEL_PRESSURE,   1, "value",          "Send Channel Pressure value (0-127)"});
        commands_.add({"pb",   PITCH_BEND,         1, "value",          "Send Pitch Bend value (0-16383)"});
        commands_.add({"rpn",  RPN,                2, "number value",   "Send RPN number (0-16383) with value (0-16383)"});
        commands_.add({"nrpn", NRPN,               2, "number value",   "Send NRPN number (0-16383) with value (0-16383)"});
        
        channel_ = 1;
    }
    
    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    
    void initialise(const String& commandLine) override
    {
        StringArray parameters(getCommandLineParameterArray());
        parseParameters(parameters);
        
        if (parameters.isEmpty())
        {
            printUsage();
        }
        
        systemRequestedQuit();
    }
                            
    void shutdown() override
    {
    }

    void anotherInstanceStarted(const String& commandLine) override
    {
    }

private:
    ApplicationCommand* findApplicationCommand(const String& param)
    {
        for (auto&& command : commands_)
        {
            if (command.param_ == param)
            {
                return &command;
            }
        }
        return nullptr;
    }
            
    void parseParameters(StringArray& parameters)
    {
        ApplicationCommand currentCommand = ApplicationCommand::Dummy();
        for (String param : parameters)
        {
            ApplicationCommand* command = findApplicationCommand(param);
            if (command)
            {
                currentCommand = *command;
            }
            else
            {
                if (currentCommand.expectedOptions_ > 0)
                {
                    currentCommand.options_.add(param);
                    currentCommand.expectedOptions_ -= 1;
                }
            }
            
            if (currentCommand.expectedOptions_ == 0)
            {
                executeCommand(currentCommand);
            }
        }
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
            
    void executeCommand(ApplicationCommand& command)
    {
        switch (command.command_)
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
                midiOutName_ = command.options_[0];
                int index = MidiOutput::getDevices().indexOf(midiOutName_);
                if (index >= 0)
                {
                    midiOut_ = MidiOutput::openDevice(index);
                }
                else
                {
                    std::cout << "Couldn't find MIDI output port \"" << midiOutName_ << "\"" << std::endl;
                }
                break;
            }
            case TXTFILE:
            {
                String path(command.options_[0]);
                File file = File::getCurrentWorkingDirectory().getChildFile(path);
                if (file.existsAsFile())
                {
                    StringArray lines;
                    file.readLines(lines);
                    StringArray noComments;
                    for (String line : lines)
                    {
                        if (!line.startsWith("#"))
                        {
                            noComments.add(line);
                        }
                    }
                    
                    StringArray parameters;
                    for (String line : noComments)
                    {
                        parameters.addTokens(line, true);
                    }
                    
                    parameters.removeEmptyStrings(true);
                    
                    parseParameters(parameters);
                }
                else
                {
                    std::cout << "Couldn't find file \"" << path << "\"" << std::endl;
                }
                break;
            }
            case CHANNEL:
                channel_ = limit7Bit(command.options_[0].getIntValue());
                break;
            case NOTE_ON:
                sendMidiMessage(MidiMessage::noteOn(channel_,
                                                    limit7Bit(command.options_[0].getIntValue()),
                                                    float(limit7Bit(command.options_[1].getIntValue()))/127.f));
                break;
            case NOTE_OFF:
                sendMidiMessage(MidiMessage::noteOff(channel_,
                                                     limit7Bit(command.options_[0].getIntValue()),
                                                     float(limit7Bit(command.options_[1].getIntValue()))/127.f));
                break;
            case POLY_PRESSURE:
                sendMidiMessage(MidiMessage::aftertouchChange(channel_,
                                                              limit7Bit(command.options_[0].getIntValue()),
                                                              limit7Bit(command.options_[1].getIntValue())));
                break;
            case CONTROL_CHANGE:
                sendMidiMessage(MidiMessage::controllerEvent(channel_,
                                                             limit7Bit(command.options_[0].getIntValue()),
                                                             limit7Bit(command.options_[1].getIntValue())));
                break;
            case PROGRAM_CHANGE:
                sendMidiMessage(MidiMessage::programChange(channel_,
                                                           limit7Bit(command.options_[0].getIntValue())));
                break;
            case CHANNEL_PRESSURE:
                sendMidiMessage(MidiMessage::channelPressureChange(channel_,
                                                                   limit7Bit(command.options_[0].getIntValue())));
                break;
            case PITCH_BEND:
                sendMidiMessage(MidiMessage::pitchWheel(channel_,
                                                        limit14Bit(command.options_[0].getIntValue())));
                break;
            case NRPN:
            {
                int number = limit14Bit(command.options_[0].getIntValue());
                int value = limit14Bit(command.options_[1].getIntValue());
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
                int number = limit14Bit(command.options_[0].getIntValue());
                int value = limit14Bit(command.options_[1].getIntValue());
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 101, number >> 7));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 100, number & 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 6, value >> 7));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 38, value & 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 101, 0x7f));
                sendMidiMessage(MidiMessage::controllerEvent(channel_, 100, 0x7f));
                break;
            }
        }
        
        command.clear();
    }
            
    static int limit7Bit(int value)
    {
        return jlimit(0, 0x7f, value);
    }
    
    static int limit14Bit(int value)
    {
        return jlimit(0, 0x3fff, value);
    }
            
    void printUsage()
    {
        std::cout << ProjectInfo::projectName << " v" << ProjectInfo::versionString << std::endl;
        std::cout << "https://github.com/gbevin/SendMIDI" << std::endl << std::endl;
        std::cout
        << "Usage: " << ProjectInfo::projectName << " [commands]" << std::endl
        << "Commands:" << std::endl;
        for (auto&& command : commands_)
        {
            std::cout << "  " << command.param_.paddedRight(' ', 4);
            if (command.optionsDescription_.isNotEmpty())
            {
                std::cout << "  " << command.optionsDescription_.paddedRight(' ', 13);
            }
            else
            {
                std::cout << "               ";
            }
            std::cout << "  " << command.commandDescription_;
            std::cout << std::endl;
        }
    }

    Array<ApplicationCommand> commands_;
    int channel_;
    String midiOutName_;
    ScopedPointer<MidiOutput> midiOut_;
};

START_JUCE_APPLICATION (sendMidiApplication)
