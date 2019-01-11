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
    MPE_TEST,
    RAW_MIDI
};

static const int DEFAULT_OCTAVE_MIDDLE_C = 3;
static const String& DEFAULT_VIRTUAL_NAME = "SendMIDI";

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
        commands_.add({"dev",   	"device",                   DEVICE,                 1, "name",           "Set the name of the MIDI output port"});
        commands_.add({"virt",  	"virtual",                  VIRTUAL,               -1, "(name)",         "Use virtual MIDI port with optional name (Linux/macOS)"});
        commands_.add({"list",  	"",                         LIST,                   0, "",               "Lists the MIDI output ports"});
        commands_.add({"panic", 	"",                         PANIC,                  0, "",               "Sends all possible Note Offs and relevant panic CCs"});
        commands_.add({"file",  	"",                         TXTFILE,                1, "path",           "Loads commands from the specified program file"});
        commands_.add({"dec",   	"decimal",                  DECIMAL,                0, "",               "Interpret the next numbers as decimals by default"});
        commands_.add({"hex",   	"hexadecimal",              HEXADECIMAL,            0, "",               "Interpret the next numbers as hexadecimals by default"});
        commands_.add({"ch",    	"channel",                  CHANNEL,                1, "number",         "Set MIDI channel for the commands (1-16), defaults to 1"});
        commands_.add({"omc",   	"octave-middle-c",          OCTAVE_MIDDLE_C,        1, "number",         "Set octave for middle C, defaults to 3"});
        commands_.add({"on",    	"note-on",                  NOTE_ON,                2, "note velocity",  "Send Note On with note (0-127) and velocity (0-127)"});
        commands_.add({"off",   	"note-off",                 NOTE_OFF,               2, "note velocity",  "Send Note Off with note (0-127) and velocity (0-127)"});
        commands_.add({"pp",    	"poly-pressure",            POLY_PRESSURE,          2, "note value",     "Send Poly Pressure with note (0-127) and value (0-127)"});
        commands_.add({"cc",    	"control-change",           CONTROL_CHANGE,         2, "number value",   "Send Control Change number (0-127) with value (0-127)"});
        commands_.add({"pc",    	"program-change",           PROGRAM_CHANGE,         1, "number",         "Send Program Change number (0-127)"});
        commands_.add({"cp",    	"channel-pressure",         CHANNEL_PRESSURE,       1, "value",          "Send Channel Pressure value (0-127)"});
        commands_.add({"pb",    	"pitch-bend",               PITCH_BEND,             1, "value",          "Send Pitch Bend value (0-16383 or value/range)"});
        commands_.add({"rpn",   	"",                         RPN,                    2, "number value",   "Send RPN number (0-16383) with value (0-16383)"});
        commands_.add({"nrpn",  	"",                         NRPN,                   2, "number value",   "Send NRPN number (0-16383) with value (0-16383)"});
        commands_.add({"clock", 	"",                         CLOCK,                  1, "bpm",            "Send 2 beats of MIDI Timing Clock for a BPM (1-999)"});
        commands_.add({"mc",    	"midi-clock",               MIDI_CLOCK,             0, "",               "Send one MIDI Timing Clock"});
        commands_.add({"start", 	"",                         START,                  0, "",               "Start the current sequence playing"});
        commands_.add({"stop",  	"",                         STOP,                   0, "",               "Stop the current sequence"});
        commands_.add({"cont",  	"continue",                 CONTINUE,               0, "",               "Continue the current sequence"});
        commands_.add({"as",    	"active-sensing",           ACTIVE_SENSING,         0, "",               "Send Active Sensing"});
        commands_.add({"rst",   	"reset",                    RESET,                  0, "",               "Send Reset"});
        commands_.add({"syx",   	"system-exclusive",         SYSTEM_EXCLUSIVE,      -1, "bytes",          "Send SysEx from a series of bytes (no F0/F7 delimiters)"});
        commands_.add({"syf",   	"system-exclusive-file",    SYSTEM_EXCLUSIVE_FILE,  1, "path",           "Send SysEx from a .syx file"});
        commands_.add({"tc",    	"time-code",                TIME_CODE,              2, "type value",     "Send MIDI Time Code with type (0-7) and value (0-15)"});
        commands_.add({"spp",   	"song-position",            SONG_POSITION,          1, "beats",          "Send Song Position Pointer with beat (0-16383)"});
        commands_.add({"ss",    	"song-select",              SONG_SELECT,            1, "number",         "Send Song Select with song number (0-127)"});
        commands_.add({"tun",   	"tune-request",             TUNE_REQUEST,           0, "",               "Send Tune Request"});
        commands_.add({"mpe",       "",                         MPE_CONFIGURATION,      2, "zone range",     "Send MPE Configuration for zone (1-2) with range (0-15)"});
        commands_.add({"mpetest",   "mpe-test",                 MPE_TEST,               0, "",               "Send a sequence of MPE messages to test a receiver"});
        commands_.add({"raw",       "raw-midi",                 RAW_MIDI,              -1, "bytes",          "Send raw MIDI from a series of bytes"});
        
        channel_ = 1;
        octaveMiddleC_ = DEFAULT_OCTAVE_MIDDLE_C;
        useHexadecimalsByDefault_ = false;
        currentCommand_ = ApplicationCommand::Dummy();
        lastTimeStampCounter_ = 0;
        lastTimeStamp_ = 0;
    }
    
    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    void systemRequestedQuit() override              { quit(); }
    
    void initialise(const String&) override
    {
        StringArray cmdLineParams(getCommandLineParameterArray());
        if (cmdLineParams.contains("--help") || cmdLineParams.contains("-h"))
        {
            printUsage();
            systemRequestedQuit();
            return;
        }
        else if (cmdLineParams.contains("--version"))
        {
            printVersion();
            systemRequestedQuit();
            return;
        }

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
    
    static bool isNumeric(const String& string)
    {
        return string.containsOnly("1234567890");
    }

    int64_t parseTimestamp(const String& param)
    {
        int64_t timestamp = 0;
        if (param.length() == 12 && param[2] == ':' && param[5] == ':' && param[8] == '.')
        {
            String hours = param.substring(0, 2);
            String minutes = param.substring(3, 5);
            String seconds = param.substring(6, 8);
            String millis = param.substring(9);
            if (isNumeric(hours) && isNumeric(minutes) && isNumeric(seconds) && isNumeric(millis))
            {
                Time now = Time();
                timestamp = Time(now.getYear(), now.getMonth(), now.getDayOfMonth(),
                                 hours.getIntValue(), minutes.getIntValue(), seconds.getIntValue(), millis.getIntValue()).toMilliseconds();
            }
        }
        else if (param.length() == 13 && param[0] == '+' && param[3] == ':' && param[6] == ':' && param[9] == '.')
        {
            String hours = param.substring(1, 3);
            String minutes = param.substring(4, 6);
            String seconds = param.substring(7, 9);
            String millis = param.substring(10);
            if (isNumeric(hours) && isNumeric(minutes) && isNumeric(seconds) && isNumeric(millis))
            {
                timestamp = (((int64_t(hours.getIntValue()) * 60 + int64_t(minutes.getIntValue())) * 60) + int64_t(seconds.getIntValue())) * 1000 + millis.getIntValue();
            }
        }
        else if (param.length() == 7 && param[0] == '+' && param[3] == '.')
        {
            String seconds = param.substring(1, 3);
            String millis = param.substring(4);
            if (isNumeric(seconds) && isNumeric(millis))
            {
                timestamp = (int64_t(seconds.getIntValue())) * 1000 + millis.getIntValue();
            }
        }
        return timestamp;
    }
    
    void executeCurrentCommand()
    {
        ApplicationCommand cmd = currentCommand_;
        currentCommand_ = ApplicationCommand::Dummy();
        executeCommand(cmd);
    }
    
    void handleVarArgCommand()
    {
        if (currentCommand_.expectedOptions_ < 0)
        {
            executeCurrentCommand();
        }
    }
    
    void parseParameters(StringArray& parameters)
    {
        for (String param : parameters)
        {
            if (param == "--") continue;
            
            ApplicationCommand* cmd = findApplicationCommand(param);
            if (cmd)
            {
                // handle configuration commands immediately without setting up a new one
                switch (cmd->command_)
                {
                    case DECIMAL:
                        useHexadecimalsByDefault_ = false;
                        break;
                    case HEXADECIMAL:
                        useHexadecimalsByDefault_ = true;
                        break;
                    default:
                        handleVarArgCommand();
                        
                        currentCommand_ = *cmd;
                        break;
                }
            }
            else
            {
                int64_t timestamp = parseTimestamp(param);
                if (timestamp)
                {
                    handleVarArgCommand();
                    
                    if (param[0] == '+')
                    {
                        Time::waitForMillisecondCounter(uint32(Time::getMillisecondCounter() + timestamp));
                    }
                    else if (lastTimeStamp_ != 0)
                    {
                        // wait for the time that needs to have elapsed since the previous timestamp
                        uint32 now_counter = Time::getMillisecondCounter();
                        int64_t delta = (timestamp - lastTimeStamp_) - (now_counter - lastTimeStampCounter_);
                        
                        // compensate for day boundary wrap around
                        if (timestamp < lastTimeStamp_)
                        {
                            delta += 24 * 60 * 60 * 1000;
                        }
                        
                        // wait for the required time
                        if (delta > 0)
                        {
                            Time::waitForMillisecondCounter(uint32(now_counter + delta));
                        }
                    }
                    
                    lastTimeStampCounter_ = Time::getMillisecondCounter();
                    lastTimeStamp_ = timestamp;
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
                    if (currentCommand_.command_ == SYSTEM_EXCLUSIVE)
                    {
                        currentCommand_.opts_.add(String(asDecOrHex7BitValue(param)));
                    }
                    else
                    {
                        currentCommand_.opts_.add(param);
                    }
                    currentCommand_.expectedOptions_ -= 1;
                }
            }
            
            // handle fixed arg commands
            if (currentCommand_.expectedOptions_ == 0)
            {
                executeCurrentCommand();
            }
        }
        
        handleVarArgCommand();
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
                std::cerr << "No valid MIDI output port was specified for some of the messages" << std::endl;
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
                    std::cerr << "Couldn't find MIDI output port \"" << midiOutName_ << "\"" << std::endl;
                }
                break;
            }
            case VIRTUAL:
            {
#if (JUCE_LINUX || JUCE_MAC)
                String name = DEFAULT_VIRTUAL_NAME;
                if (cmd.opts_.size())
                {
                    name = cmd.opts_[0];
                }
                midiOut_ = MidiOutput::createNewDevice(name);
                if (midiOut_ == nullptr)
                {
                    std::cerr << "Couldn't create virtual MIDI output port \"" << name << "\"" << std::endl;
                }
                else
                {
                    midiOutName_ = cmd.opts_[0];
                }
#else
                std::cerr << "Virtual MIDI output ports are not supported on Windows" << std::endl;
#endif
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
                    std::cerr << "Couldn't find file \"" << path << "\"" << std::endl;
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
            case OCTAVE_MIDDLE_C:
                octaveMiddleC_ = asDecOrHex7BitValue(cmd.opts_[0]);
                break;
            case NOTE_ON:
                sendMidiMessage(MidiMessage::noteOn(channel_,
                                                    asNoteNumber(cmd.opts_[0]),
                                                    asDecOrHex7BitValue(cmd.opts_[1])));
                break;
            case NOTE_OFF:
                sendMidiMessage(MidiMessage::noteOff(channel_,
                                                     asNoteNumber(cmd.opts_[0]),
                                                     asDecOrHex7BitValue(cmd.opts_[1])));
                break;
            case POLY_PRESSURE:
                sendMidiMessage(MidiMessage::aftertouchChange(channel_,
                                                              asNoteNumber(cmd.opts_[0]),
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
            case MIDI_CLOCK:
                sendMidiMessage(MidiMessage::midiClock());
                break;
            case START:
                sendMidiMessage(MidiMessage::midiStart());
                break;
            case STOP:
                sendMidiMessage(MidiMessage::midiStop());
                break;
            case CONTINUE:
                sendMidiMessage(MidiMessage::midiContinue());
                break;
            case ACTIVE_SENSING:
                sendMidiMessage(MidiMessage(0xfe));
                break;
            case RESET:
                sendMidiMessage(MidiMessage(0xff));
                break;
            case TIME_CODE:
                sendMidiMessage(MidiMessage::quarterFrame(asDecOrHex14BitValue(cmd.opts_[0]), asDecOrHex14BitValue(cmd.opts_[1])));
                break;
            case SONG_POSITION:
                sendMidiMessage(MidiMessage::songPositionPointer(asDecOrHex14BitValue(cmd.opts_[0])));
                break;
            case SONG_SELECT:
                sendMidiMessage(MidiMessage(0xf3, asDecOrHex7BitValue(cmd.opts_[0])));
                break;
            case SYSTEM_EXCLUSIVE:
            {
                MemoryBlock mem(cmd.opts_.size(), true);
                for (int i = 0; i < cmd.opts_.size(); ++i)
                {
                    mem[i] = cmd.opts_[i].getIntValue();
                }
                sendMidiMessage(MidiMessage::createSysExMessage(mem.getData(), (int)mem.getSize()));
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
                        sendMidiMessage(MidiMessage(mem.getData(), (int)mem.getSize()));
                    }
                }
                else
                {
                    std::cerr << "Couldn't find file \"" << path << "\"" << std::endl;
                }
                break;
            }
            case TUNE_REQUEST:
                sendMidiMessage(MidiMessage(0xf6));
                break;
            case MPE_CONFIGURATION:
            {
                int zone = jlimit(1, 2, asDecOrHexIntValue(cmd.opts_[0]));
                int range = jlimit(0, 15, asDecOrHexIntValue(cmd.opts_[1]));
                sendRPN(zone == 1 ? 1 : 16, 6, range << 7);
                break;
            }
            case MPE_TEST:
            {
                sendMpeTestScenario();
                break;
            }
            case RAW_MIDI:
            {
                MemoryBlock mem(cmd.opts_.size(), true);
                for (int i = 0; i < cmd.opts_.size(); ++i)
                {
                    mem[i] = (uint8)asDecOrHexIntValue(cmd.opts_[i]);
                }
                sendMidiMessage(MidiMessage(mem.getData(), (int)mem.getSize()));
            }
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
    
    void scenarioStep(const String& message)
    {
        std::cout << message << " ";
        for (int i = 0 ; i < 3; ++i)
        {
            Thread::sleep(100);
            std::cout << ".";
            std::cout.flush();
        }
        Thread::sleep(300);
        std::cout << std::endl;
        std::cout.flush();
    }
    
    void sendMpeTestScenario()
    {
        scenarioStep("MPE Zone 1 with 15 Member Channels");
        sendMidiMessage(MidiMessage::controllerEvent(1, 0x64, 6));
        sendMidiMessage(MidiMessage::controllerEvent(1, 0x65, 0));
        sendMidiMessage(MidiMessage::controllerEvent(1, 0x06, 15));
        
        scenarioStep("Pitch Bend Sensitivity on Master Channel to 7 semitones");
        sendMidiMessage(MidiMessage::controllerEvent(1, 0x64, 0));
        sendMidiMessage(MidiMessage::controllerEvent(1, 0x65, 0));
        sendMidiMessage(MidiMessage::controllerEvent(1, 0x06, 7));
        
        int note_pbsens = 48;
        scenarioStep(String("Pitch Bend Sensitivity on Member Channels to ") + String(note_pbsens) + " semitones");
        sendMidiMessage(MidiMessage::controllerEvent(2, 0x64, 0));
        sendMidiMessage(MidiMessage::controllerEvent(2, 0x65, 0));
        sendMidiMessage(MidiMessage::controllerEvent(2, 0x06, note_pbsens));
        
        scenarioStep("Major C triad on Member Channels with neutral starting expression");
        
        sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000));
        sendMidiMessage(MidiMessage::controllerEvent(2, 74, 0x00));
        sendMidiMessage(MidiMessage::channelPressureChange(2, 0));
        sendMidiMessage(MidiMessage::noteOn(2, 0x3c, (uint8)0x60));
        
        sendMidiMessage(MidiMessage::pitchWheel(3, 0x2000));
        sendMidiMessage(MidiMessage::controllerEvent(3, 74, 0x00));
        sendMidiMessage(MidiMessage::channelPressureChange(3, 0));
        sendMidiMessage(MidiMessage::noteOn(3, 0x40, (uint8)0x7f));

        sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000));
        sendMidiMessage(MidiMessage::controllerEvent(16, 74, 0x00));
        sendMidiMessage(MidiMessage::channelPressureChange(16, 0));
        sendMidiMessage(MidiMessage::noteOn(16, 0x43, (uint8)0x80));
        
        scenarioStep("Pitch bend into opposite directions, also resulting into Major C triad");

        int bend_interval = 7;
        int ch02_pitch_target = + (0x1FFF * bend_interval / note_pbsens);
        int ch16_pitch_target = - (0x1FFF * bend_interval / note_pbsens);
        int bend_messages = 1000;
        for (int i = 1; i <= bend_messages; ++i)
        {
            sendMidiMessage(MidiMessage::pitchWheel(2, 0x2000 + (ch02_pitch_target * i) / bend_messages));
            sendMidiMessage(MidiMessage::pitchWheel(16, 0x2000 + (ch16_pitch_target * i) / bend_messages));
            Thread::sleep(1);
        }
        
        Thread::sleep(2000);
        
        scenarioStep("Independent timbral motion across different notes");
        
        int timbre_messages = 1000;
        int ch02_last_timbre = 0;
        int ch03_last_timbre = 0;
        int ch16_last_timbre = 0;
        for (int i = 0; i <= timbre_messages; ++i)
        {
            int ch02_val = (0x7F * i) / timbre_messages;
            if (ch02_last_timbre != ch02_val)
            {
                sendMidiMessage(MidiMessage::controllerEvent(2, 74, ch02_val));
                ch02_last_timbre = ch02_val;
            }
            Thread::sleep(1);
        }
        for (int i = 0; i <= timbre_messages; ++i)
        {
            int ch02_val = 0x7F - (0x7F * i) / timbre_messages;
            if (ch02_last_timbre != ch02_val)
            {
                sendMidiMessage(MidiMessage::controllerEvent(2, 74, ch02_val));
                ch02_last_timbre = ch02_val;
            }
            int ch03_val = (0x7F * i) / timbre_messages;
            if (ch03_last_timbre != ch03_val)
            {
                sendMidiMessage(MidiMessage::controllerEvent(3, 74, ch03_val));
                ch03_last_timbre = ch03_val;
            }
            Thread::sleep(1);
        }
        for (int i = 0; i <= timbre_messages; ++i)
        {
            int ch03_val = 0x7F - (0x7F * i) / timbre_messages;
            if (ch03_last_timbre != ch03_val)
            {
                sendMidiMessage(MidiMessage::controllerEvent(3, 74, ch03_val));
                ch03_last_timbre = ch03_val;
            }
            int ch16_val = (0x7F * i) / timbre_messages;
            if (ch16_last_timbre != ch16_val)
            {
                sendMidiMessage(MidiMessage::controllerEvent(16, 74, ch16_val));
                ch16_last_timbre = ch16_val;
            }
            Thread::sleep(1);
        }
        for (int i = 0; i <= timbre_messages; ++i)
        {
            int ch16_val = 0x7F - (0x7F * i) / timbre_messages;
            if (ch16_last_timbre != ch16_val)
            {
                sendMidiMessage(MidiMessage::controllerEvent(16, 74, ch16_val));
                ch16_last_timbre = ch16_val;
            }
            Thread::sleep(1);
        }

        Thread::sleep(2000);

        scenarioStep("Release the active Notes");
        
        sendMidiMessage(MidiMessage::noteOff(2, 0x3c, (uint8)0x40));
        sendMidiMessage(MidiMessage::noteOff(3, 0x40, (uint8)0x40));
        sendMidiMessage(MidiMessage::noteOff(16, 0x43, (uint8)0x40));
    }
    
    uint8 asNoteNumber(String value)
    {
        if (value.length() >= 2)
        {
            value = value.toUpperCase();
            String first = value.substring(0, 1);
            if (first.containsOnly("CDEFGABH") && value.substring(value.length()-1).containsOnly("1234567890"))
            {
                int note = 0;
                switch (first[0])
                {
                    case 'C': note = 0; break;
                    case 'D': note = 2; break;
                    case 'E': note = 4; break;
                    case 'F': note = 5; break;
                    case 'G': note = 7; break;
                    case 'A': note = 9; break;
                    case 'B': note = 11; break;
                    case 'H': note = 11; break;
                }
                
                if (value[1] == 'B')
                {
                    note -= 1;
                }
                else if (value[1] == '#')
                {
                    note += 1;
                }
                
                note += (value.getTrailingIntValue() + 5 - octaveMiddleC_) * 12;
                
                return (uint8)limit7Bit(note);
            }
        }
        
        return (uint8)limit7Bit(asDecOrHexIntValue(value));
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
        else if (value.endsWithIgnoreCase("M"))
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
    
    void printVersion()
    {
        std::cout << ProjectInfo::projectName << " v" << ProjectInfo::versionString << std::endl;
        std::cout << "https://github.com/gbevin/SendMIDI" << std::endl;
    }
    
    void printUsage()
    {
        printVersion();
        std::cout << std::endl;
        std::cout << "Usage: " << ProjectInfo::projectName << " [ commands ] [ programfile ] [ -- ]" << std::endl << std::endl
                  << "Commands:" << std::endl;
        for (auto&& cmd : commands_)
        {
            String param_option;
            param_option << "  " << cmd.param_.paddedRight(' ', 5);
            if (cmd.optionsDescription_.isNotEmpty())
            {
                param_option << " " << cmd.optionsDescription_.paddedRight(' ', 13);
            }
            else
            {
                param_option << "              ";
            }
            param_option << "  ";
            param_option = param_option.substring(0, 23);
            std::cout << param_option << cmd.commandDescription_;
            std::cout << std::endl;
        }
        std::cout << "  -h  or  --help       Print Help (this message) and exit" << std::endl;
        std::cout << "  --version            Print version information and exit" << std::endl;
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
                  << "number with \"M\" or \"H\", it will be interpreted as a decimal or hexadecimal" << std::endl
                  << "respectively." << std::endl;
        std::cout << std::endl;
        std::cout << "The MIDI device name doesn't have to be an exact match." << std::endl;
        std::cout << "If SendMIDI can't find the exact name that was specified, it will pick the" << std::endl
                  << "first MIDI output port that contains the provided text, irrespective of case." << std::endl;
        std::cout << std::endl;
        std::cout << "Where notes can be provided as arguments, they can also be written as note" << std::endl
                  << "names, by default from C-2 to G8 which corresponds to note numbers 0 to 127." << std::endl
                  << "By setting the octave for middle C, the note name range can be changed. " << std::endl
                  << "Sharps can be added by using the '#' symbol after the note letter, and flats" << std::endl
                  << "by using the letter 'b'. " << std::endl;
        std::cout << std::endl;
        std::cout << "In between commands, timestamps can be added in the format: HH:MM:SS.MIL," << std::endl
                  << "standing for hours, minutes, seconds and milliseconds" << std::endl
                  << "(for example: 08:10:17.056). All the digits need to be present, possibly" << std::endl
                  << "requiring leading zeros. When a timestamp is detected, SendMIDI ensures that" << std::endl
                  << "the time difference since the previous timestamp has elapsed." << std::endl;
        std::cout << std::endl;
        std::cout << "When a timestamp is prefixed with a plus sign, it's considered relative and" << std::endl
                  << "will be processed as a time offset instead of an absolute time. For example" << std::endl
                  << "+00:00:01.060 will execute the next command one second and 60 milliseconds"  << std::endl
                  << "later. For convenience, a relative timestamp can also be shortened to +SS.MIL"  << std::endl
                  << "(for example: +01.060)." << std::endl;
    }
    
    Array<ApplicationCommand> commands_;
    int channel_;
    int octaveMiddleC_;
    bool useHexadecimalsByDefault_;
    String midiOutName_;
    ScopedPointer<MidiOutput> midiOut_;
    ApplicationCommand currentCommand_;
    uint32 lastTimeStampCounter_;
    int64_t lastTimeStamp_;
};

START_JUCE_APPLICATION (sendMidiApplication)
