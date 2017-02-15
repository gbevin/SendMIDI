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
    DECIMAL,
    HEXADECIMAL,
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
    CLOCK,
    SONG_POSITION,
    SONG_SELECT,
    MPE_CONFIGURATION,
    SYSTEM_EXCLUSIVE,
    SYSTEM_EXCLUSIVE_FILE,
    TUNE_REQUEST,
    ACTIVE_SENSING,
    RESET
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
    int expectedOptions_;
    String optionsDescription_;
    String commandDescription_;
    StringArray opts_;
};

inline float sign(float value)
{
    return (float)(value > 0.) - (value < 0.);
}

class sendMidiApplication  : public JUCEApplicationBase
{
public:
    sendMidiApplication()
    {
        commands_.add({"dev",   "device",                   DEVICE,                 1, "name",           "Set the name of the MIDI output port (REQUIRED)"});
        commands_.add({"list",  "",                         LIST,                   0, "",               "Lists the MIDI output ports"});
        commands_.add({"panic", "",                         PANIC,                  0, "",               "Sends all possible Note Offs and relevant panic CCs"});
        commands_.add({"file",  "",                         TXTFILE,                1, "path",           "Loads commands from the specified program file"});
        commands_.add({"dec",   "decimal",                  DECIMAL,                0, "",               "Interpret the next numbers as decimals by default"});
        commands_.add({"hex",   "hexadecimal",              HEXADECIMAL,            0, "",               "Interpret the next numbers as hexadecimals by default"});
        commands_.add({"ch",    "channel",                  CHANNEL,                1, "number",         "Set MIDI channel for the commands (1-16), defaults to 1"});
        commands_.add({"on",    "note-on",                  NOTE_ON,                2, "note velocity",  "Send Note On with note (0-127) and velocity (0-127)"});
        commands_.add({"off",   "note-off",                 NOTE_OFF,               2, "note velocity",  "Send Note Off with note (0-127) and velocity (0-127)"});
        commands_.add({"pp",    "poly-pressure",            POLY_PRESSURE,          2, "note value",     "Send Poly Pressure with note (0-127) and value (0-127)"});
        commands_.add({"cc",    "control-change",           CONTROL_CHANGE,         2, "number value",   "Send Control Change number (0-127) with value (0-127)"});
        commands_.add({"pc",    "program-change",           PROGRAM_CHANGE,         1, "number",         "Send Program Change number (0-127)"});
        commands_.add({"cp",    "channel-pressure",         CHANNEL_PRESSURE,       1, "value",          "Send Channel Pressure value (0-127)"});
        commands_.add({"pb",    "pitch-bend",               PITCH_BEND,             1, "value",          "Send Pitch Bend value (0-16383 or value/range)"});
        commands_.add({"rpn",   "",                         RPN,                    2, "number value",   "Send RPN number (0-16383) with value (0-16383)"});
        commands_.add({"nrpn",  "",                         NRPN,                   2, "number value",   "Send NRPN number (0-16383) with value (0-16383)"});
        commands_.add({"start", "",                         START,                  0, "",               "Start the current sequence playing"});
        commands_.add({"stop",  "",                         STOP,                   0, "",               "Stop the current sequence"});
        commands_.add({"cont",  "continue",                 CONTINUE,               0, "",               "Continue the current sequence"});
        commands_.add({"clock", "",                         CLOCK,                  1, "bpm",            "Send 2 beats of MIDI Timing Clock for a BPM (1-999)"});
        commands_.add({"spp",   "song-position",            SONG_POSITION,          1, "beats",          "Send Song Position Pointer with beat (0-16383)"});
        commands_.add({"ss",    "song-select",              SONG_SELECT,            1, "number",         "Send Song Select with song number (0-127)"});
        commands_.add({"syx",   "system-exclusive",         SYSTEM_EXCLUSIVE,      -1, "bytes",          "Send SysEx from a series of bytes (no F0/F7 delimiters)"});
        commands_.add({"syf",   "system-exclusive-file",    SYSTEM_EXCLUSIVE_FILE,  1, "path",           "Send SysEx from a .syx file"});
        commands_.add({"tun",   "tune-request",             TUNE_REQUEST,           0, "",               "Send Tune Request"});
        commands_.add({"as",    "active-sensing",           ACTIVE_SENSING,         0, "",               "Send Active Sensing"});
        commands_.add({"rst",   "reset",                    RESET,                  0, "",               "Send Reset"});
        commands_.add({"mpe",   "",                         MPE_CONFIGURATION,      2, "zone range",     "Send MPE Configuration for zone (1-2) with range (0-15)"});
        
        channel_ = 1;
        useHexadecimalsByDefault_ = false;
        currentCommand_ = ApplicationCommand::Dummy();
    }
    
    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    void systemRequestedQuit() override              { quit(); }
    
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
    void suspended() override {}
    void resumed() override {}
    void anotherInstanceStarted(const String&) override {}
    void unhandledException(const std::exception*, const String&, int) override { jassertfalse; }
    
private:
    ApplicationCommand* findApplicationCommand(const String& param)
    {
        for (auto&& cmd : commands_)
        {
            if (cmd.param_.equalsIgnoreCase(param) || cmd.altParam_.equalsIgnoreCase(param))
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
        for (String param : parameters)
        {
            ApplicationCommand* cmd = findApplicationCommand(param);
            if (cmd)
            {
                // handle configuration commands immediately without setting up a new
                switch (cmd->command_)
                {
                    case DECIMAL:
                        useHexadecimalsByDefault_ = false;
                        break;
                    case HEXADECIMAL:
                        useHexadecimalsByDefault_ = true;
                        break;
                    default:
                        // handle variable arg commands
                        if (currentCommand_.expectedOptions_ < 0)
                        {
                            executeCommand(currentCommand_);
                        }
                        
                        currentCommand_ = *cmd;
                        break;
                }
            }
            else if (currentCommand_.command_ == NONE)
            {
                File file = File::getCurrentWorkingDirectory().getChildFile(param);
                if (file.existsAsFile())
                {
                    parseFile(file);
                }
            }
            else if (currentCommand_.expectedOptions_ != 0)
            {
                currentCommand_.opts_.add(param);
                currentCommand_.expectedOptions_ -= 1;
            }
            
            // handle fixed arg commands
            if (currentCommand_.expectedOptions_ == 0)
            {
                executeCommand(currentCommand_);
            }
        }
        
        // handle variable arg commands
        if (currentCommand_.expectedOptions_ < 0)
        {
            executeCommand(currentCommand_);
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
            case DECIMAL:
            case HEXADECIMAL:
                // these are not commands but rather configuration options
                // allow them to be inlined anywhere by handling them immediately in the
                // parseParameters method
                break;
            case CHANNEL:
                channel_ = asDecOrHex7BitValue(cmd.opts_[0]);
                break;
            case NOTE_ON:
                sendMidiMessage(MidiMessage::noteOn(channel_,
                                                    asDecOrHex7BitValue(cmd.opts_[0]),
                                                    asDecOrHex7BitValue(cmd.opts_[1])));
                break;
            case NOTE_OFF:
                sendMidiMessage(MidiMessage::noteOff(channel_,
                                                     asDecOrHex7BitValue(cmd.opts_[0]),
                                                     asDecOrHex7BitValue(cmd.opts_[1])));
                break;
            case POLY_PRESSURE:
                sendMidiMessage(MidiMessage::aftertouchChange(channel_,
                                                              asDecOrHex7BitValue(cmd.opts_[0]),
                                                              asDecOrHex7BitValue(cmd.opts_[1])));
                break;
            case CONTROL_CHANGE:
                sendMidiMessage(MidiMessage::controllerEvent(channel_,
                                                             asDecOrHex7BitValue(cmd.opts_[0]),
                                                             asDecOrHex7BitValue(cmd.opts_[1])));
                break;
            case PROGRAM_CHANGE:
                sendMidiMessage(MidiMessage::programChange(channel_,
                                                           asDecOrHex7BitValue(cmd.opts_[0])));
                break;
            case CHANNEL_PRESSURE:
                sendMidiMessage(MidiMessage::channelPressureChange(channel_,
                                                                   asDecOrHex7BitValue(cmd.opts_[0])));
                break;
            case PITCH_BEND:
            {
                String arg = cmd.opts_[0];
                int value = 0;
                if (arg.containsChar('/'))
                {
                    String numerator = arg.upToFirstOccurrenceOf("/", false, true);
                    String denominator = arg.substring(numerator.length()+1);
                    float numVal = numerator.getFloatValue();
                    float denomVal = denominator.getFloatValue();
                    if (fabs(numVal) > denomVal)
                    {
                        numVal = sign(numVal)*denomVal;
                    }
                    value = limit14Bit(MidiMessage::pitchbendToPitchwheelPos(numVal, denomVal));
                }
                else
                {
                    value = asDecOrHex14BitValue(arg);
                }
                sendMidiMessage(MidiMessage::pitchWheel(channel_, value));
                break;
            }
            case NRPN:
            {
                int number = asDecOrHex14BitValue(cmd.opts_[0]);
                int value = asDecOrHex14BitValue(cmd.opts_[1]);
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
                sendRPN(channel_, asDecOrHexIntValue(cmd.opts_[0]), asDecOrHexIntValue(cmd.opts_[1]));
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
            case CLOCK:
            {
                uint32 now = Time::getMillisecondCounter();
                float bpm = float(jlimit(1, 999, asDecOrHexIntValue(cmd.opts_[0])));
                float msPerTick = (60.f * 1000.f / bpm) / 24.f;
                sendMidiMessage(MidiMessage::midiClock());
                for (int ticks = 1; ticks <= 24 * 2; ++ticks)
                {
                    Time::waitForMillisecondCounter(now + uint32(float(ticks) * msPerTick));
                    sendMidiMessage(MidiMessage::midiClock());
                }
                break;
            }
            case SONG_POSITION:
                sendMidiMessage(MidiMessage::songPositionPointer(asDecOrHex14BitValue(cmd.opts_[0])));
                break;
            case SONG_SELECT:
                sendMidiMessage(MidiMessage(0xf3, asDecOrHex7BitValue(cmd.opts_[0])));
                break;
            case MPE_CONFIGURATION:
            {
                int zone = jlimit(1, 2, asDecOrHexIntValue(cmd.opts_[0]));
                int range = jlimit(0, 15, asDecOrHexIntValue(cmd.opts_[1]));
                sendRPN(zone == 1 ? 1 : 16, 6, range);
                break;
            }
            case SYSTEM_EXCLUSIVE:
            {
                MemoryBlock mem(cmd.opts_.size(), true);
                for (int i = 0; i < cmd.opts_.size(); ++i)
                {
                    mem[i] = asDecOrHex7BitValue(cmd.opts_[i]);
                }
                sendMidiMessage(MidiMessage::createSysExMessage(mem.getData(), mem.getSize()));
                break;
            }
            case SYSTEM_EXCLUSIVE_FILE:
            {
                String path(cmd.opts_[0]);
                File file = File::getCurrentWorkingDirectory().getChildFile(path);
                if (file.existsAsFile())
                {
                    MemoryBlock mem;
                    bool readSuccess = file.loadFileAsData(mem);
                    if (readSuccess)
                    {
                        sendMidiMessage(MidiMessage(mem.getData(), mem.getSize()));
                    }
                }
                else
                {
                    std::cout << "Couldn't find file \"" << path << "\"" << std::endl;
                }
                break;
            }
            case TUNE_REQUEST:
                sendMidiMessage(MidiMessage(0xf6));
                break;
            case ACTIVE_SENSING:
                sendMidiMessage(MidiMessage(0xfe));
                break;
            case RESET:
                sendMidiMessage(MidiMessage(0xff));
                break;
        }
        
        cmd.clear();
    }
    
    void sendRPN(int channel, int number, int value)
    {
        number = limit14Bit(number);
        value = limit14Bit(value);
        sendMidiMessage(MidiMessage::controllerEvent(channel, 101, number >> 7));
        sendMidiMessage(MidiMessage::controllerEvent(channel, 100, number & 0x7f));
        sendMidiMessage(MidiMessage::controllerEvent(channel, 6, value >> 7));
        sendMidiMessage(MidiMessage::controllerEvent(channel, 38, value & 0x7f));
        sendMidiMessage(MidiMessage::controllerEvent(channel, 101, 0x7f));
        sendMidiMessage(MidiMessage::controllerEvent(channel, 100, 0x7f));
    }
    
    uint8 asDecOrHex7BitValue(String value)
    {
        return (uint8)limit7Bit(asDecOrHexIntValue(value));
    }
    
    uint16 asDecOrHex14BitValue(String value)
    {
        return (uint16)limit14Bit(asDecOrHexIntValue(value));
    }
    
    int asDecOrHexIntValue(String value)
    {
        if (value.endsWithIgnoreCase("H"))
        {
            return value.dropLastCharacters(1).getHexValue32();
        }
        else if (value.endsWithIgnoreCase("D"))
        {
            return value.getIntValue();
        }
        else if (useHexadecimalsByDefault_)
        {
            return value.getHexValue32();
        }
        else
        {
            return value.getIntValue();
        }
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
        std::cout << "By default, numbers are interpreted in the decimal system, this can be changed" << std::endl
                  << "to hexadecimal by sending the \"hex\" command. Additionally, by suffixing a " << std::endl
                  << "number with \"D\" or \"H\", it will be interpreted as a decimal or hexadecimal" << std::endl
                  << "respectively." << std::endl;
        std::cout << std::endl;
        std::cout << "The MIDI device name doesn't have to be an exact match." << std::endl;
        std::cout << "If SendMIDI can't find the exact name that was specified, it will pick the" << std::endl
                  << "first MIDI output port that contains the provided text, irrespective of case." << std::endl;
        std::cout << std::endl;
    }
    
    Array<ApplicationCommand> commands_;
    int channel_;
    String midiOutName_;
    ScopedPointer<MidiOutput> midiOut_;
    bool useHexadecimalsByDefault_;
    ApplicationCommand currentCommand_;
};

START_JUCE_APPLICATION (sendMidiApplication)
