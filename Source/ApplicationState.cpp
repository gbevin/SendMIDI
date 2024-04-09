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

static const int DEFAULT_OCTAVE_MIDDLE_C = 3;

ApplicationState::ApplicationState()
{
    commands_.add({"dev",   	"device",                   DEVICE,                 1, {"name"},             {"Set the name of the MIDI output port"}});
    commands_.add({"virt",  	"virtual",                  VIRTUAL,               -1, {"(name)"},           {"Use virtual MIDI port with optional name (Linux/macOS)"}});
    commands_.add({"list",  	"",                         LIST,                   0, {""},                 {"Lists the MIDI output ports"}});
    commands_.add({"panic", 	"",                         PANIC,                  0, {""},                 {"Sends all possible Note Offs and relevant panic CCs"}});
    commands_.add({"file",  	"",                         TXTFILE,                1, {"path"},             {"Loads commands from the specified program file"}});
    commands_.add({"dec",   	"decimal",                  DECIMAL,                0, {""},                 {"Interpret the next numbers as decimals by default"}});
    commands_.add({"hex",   	"hexadecimal",              HEXADECIMAL,            0, {""},                 {"Interpret the next numbers as hexadecimals by default"}});
    commands_.add({"ch",    	"channel",                  CHANNEL,                1, {"number"},           {"Set MIDI channel for the commands (1-16), defaults to 1"}});
    commands_.add({"omc",   	"octave-middle-c",          OCTAVE_MIDDLE_C,        1, {"number"},           {"Set octave for middle C, defaults to 3"}});
    commands_.add({"on",    	"note-on",                  NOTE_ON,                2, {"note velocity"},    {"Send Note On with note (0-127) and velocity (0-127)"}});
    commands_.add({"off",   	"note-off",                 NOTE_OFF,               2, {"note velocity"},    {"Send Note Off with note (0-127) and velocity (0-127)"}});
    commands_.add({"pp",    	"poly-pressure",            POLY_PRESSURE,          2, {"note value"},       {"Send Poly Pressure with note (0-127) and value (0-127)"}});
    commands_.add({"cc",    	"control-change",           CONTROL_CHANGE,         2, {"number value"},     {"Send Control Change number (0-127) with value (0-127)"}});
    commands_.add({"cc14",      "control-change-14",        CONTROL_CHANGE_14BIT,   2, {"number value"},     {"Send 14-bit CC number (0-31) with value (0-16383)"}});
    commands_.add({"pc",    	"program-change",           PROGRAM_CHANGE,         1, {"number"},           {"Send Program Change number (0-127)"}});
    commands_.add({"cp",    	"channel-pressure",         CHANNEL_PRESSURE,       1, {"value"},            {"Send Channel Pressure value (0-127)"}});
    commands_.add({"pb",    	"pitch-bend",               PITCH_BEND,             1, {"value"},            {"Send Pitch Bend value (0-16383 or value/range)"}});
    commands_.add({"rpn",   	"",                         RPN,                    2, {"number value"},     {"Send RPN number (0-16383) with value (0-16383)"}});
    commands_.add({"nrpn",  	"",                         NRPN,                   2, {"number value"},     {"Send NRPN number (0-16383) with value (0-16383)"}});
    commands_.add({"clock", 	"",                         CLOCK,                  1, {"bpm"},              {"Send 2 beats of MIDI Timing Clock for a BPM (1-999)"}});
    commands_.add({"mc",    	"midi-clock",               MIDI_CLOCK,             0, {""},                 {"Send one MIDI Timing Clock"}});
    commands_.add({"start", 	"",                         START,                  0, {""},                 {"Start the current sequence playing"}});
    commands_.add({"stop",  	"",                         STOP,                   0, {""},                 {"Stop the current sequence"}});
    commands_.add({"cont",  	"continue",                 CONTINUE,               0, {""},                 {"Continue the current sequence"}});
    commands_.add({"as",    	"active-sensing",           ACTIVE_SENSING,         0, {""},                 {"Send Active Sensing"}});
    commands_.add({"rst",   	"reset",                    RESET,                  0, {""},                 {"Send Reset"}});
    commands_.add({"syx",   	"system-exclusive",         SYSTEM_EXCLUSIVE,      -1, {"bytes"},            {"Send SysEx from a series of bytes (no F0/F7 delimiters)"}});
    commands_.add({"syf",   	"system-exclusive-file",    SYSTEM_EXCLUSIVE_FILE,  1, {"path"},             {"Send SysEx from a .syx file"}});
    commands_.add({"tc",    	"time-code",                TIME_CODE,              2, {"type value"},       {"Send MIDI Time Code with type (0-7) and value (0-15)"}});
    commands_.add({"spp",   	"song-position",            SONG_POSITION,          1, {"beats"},            {"Send Song Position Pointer with beat (0-16383)"}});
    commands_.add({"ss",    	"song-select",              SONG_SELECT,            1, {"number"},           {"Send Song Select with song number (0-127)"}});
    commands_.add({"tun",   	"tune-request",             TUNE_REQUEST,           0, {""},                 {"Send Tune Request"}});
    commands_.add({"mpe",       "",                         MPE_CONFIGURATION,      2, {"zone range"},       {"Send MPE Configuration for zone (1-2) with range (0-15)"}});
    commands_.add({"mpp",       "mpe-profile",              MPE_PROFILE,            3, {"input", "manager", "members"},
                                                                                       {"Configure MPE Profile initiator with MIDI input port name,",
                                                                                        "a manager channel (1-15), and desired member channel",
                                                                                        "count (1-15, 0 to disable) (also uses MIDI output port)"}});
    commands_.add({"mpetest",   "mpe-test",                 MPE_TEST,               0, {""},                 {"Send a sequence of MPE messages to test a receiver"}});
    commands_.add({"raw",       "raw-midi",                 RAW_MIDI,              -1, {"bytes"},            {"Send raw MIDI from a series of bytes"}});
    
    channel_ = 1;
    octaveMiddleC_ = DEFAULT_OCTAVE_MIDDLE_C;
    useHexadecimalsByDefault_ = false;
    currentCommand_ = ApplicationCommand::Dummy();
    lastTimeStampCounter_ = 0;
    lastTimeStamp_ = 0;
    
    mpeProfile_ = std::make_unique<MpeProfileNegotiation>(this);
}

void ApplicationState::initialise(JUCEApplicationBase& app)
{
    StringArray cmdLineParams(app.getCommandLineParameterArray());
    if (cmdLineParams.contains("--help") || cmdLineParams.contains("-h"))
    {
        printUsage();
        app.systemRequestedQuit();
        return;
    }
    else if (cmdLineParams.contains("--version"))
    {
        printVersion();
        app.systemRequestedQuit();
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
    
    while (mpeProfile_->isWaitingForNegotation())
    {
        if (!MessageManager::getInstance()->runDispatchLoopUntil(100))
        {
            break;
        }
    }
    
    midiIn_ = nullptr;
    
    app.systemRequestedQuit();
}

ApplicationCommand* ApplicationState::findApplicationCommand(const String& param)
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

StringArray ApplicationState::parseLineAsParameters(const String& line)
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

bool ApplicationState::isNumeric(const String& string)
{
    return string.containsOnly("1234567890");
}

int64_t ApplicationState::parseTimestamp(const String& param)
{
    int64_t timestamp = 0;
    if (param.length() == 12 && param[2] == ':' && param[5] == ':' && param[8] == '.')
    {
        auto hours = param.substring(0, 2);
        auto minutes = param.substring(3, 5);
        auto seconds = param.substring(6, 8);
        auto millis = param.substring(9);
        if (isNumeric(hours) && isNumeric(minutes) && isNumeric(seconds) && isNumeric(millis))
        {
            auto now = Time();
            timestamp = Time(now.getYear(), now.getMonth(), now.getDayOfMonth(),
                             hours.getIntValue(), minutes.getIntValue(), seconds.getIntValue(), millis.getIntValue()).toMilliseconds();
        }
    }
    else if (param.length() == 13 && param[0] == '+' && param[3] == ':' && param[6] == ':' && param[9] == '.')
    {
        auto hours = param.substring(1, 3);
        auto minutes = param.substring(4, 6);
        auto seconds = param.substring(7, 9);
        auto millis = param.substring(10);
        if (isNumeric(hours) && isNumeric(minutes) && isNumeric(seconds) && isNumeric(millis))
        {
            timestamp = (((int64_t(hours.getIntValue()) * 60 + int64_t(minutes.getIntValue())) * 60) + int64_t(seconds.getIntValue())) * 1000 + millis.getIntValue();
        }
    }
    else if (param.length() == 7 && param[0] == '+' && param[3] == '.')
    {
        auto seconds = param.substring(1, 3);
        auto millis = param.substring(4);
        if (isNumeric(seconds) && isNumeric(millis))
        {
            timestamp = (int64_t(seconds.getIntValue())) * 1000 + millis.getIntValue();
        }
    }
    return timestamp;
}

void ApplicationState::openOutputDevice(const String& name)
{
    midiOut_ = nullptr;
    midiOutName_ = name;
    auto devices = MidiOutput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices[i].name == midiOutName_)
        {
            midiOut_ = MidiOutput::openDevice(devices[i].identifier);
            midiOutName_ = devices[i].name;
            break;
        }
    }
    
    if (midiOut_ == nullptr)
    {
        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices[i].name.containsIgnoreCase(midiOutName_))
            {
                midiOut_ = MidiOutput::openDevice(devices[i].identifier);
                midiOutName_ = devices[i].name;
                break;
            }
        }
    }
    if (midiOut_ == nullptr)
    {
        std::cerr << "Couldn't find MIDI output port \"" << midiOutName_ << "\"" << std::endl;
        JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
    }
}

void ApplicationState::openInputDevice(const String& name)
{
    midiIn_ = nullptr;
    
    if (!tryToConnectMidiInput(name))
    {
        std::cerr << "Couldn't find MIDI input port \"" << name << "\"" << std::endl;
    }
}

bool ApplicationState::tryToConnectMidiInput(const String& name)
{
    std::unique_ptr<MidiInput> midi_input = nullptr;
    String midi_input_name = name;
    
    auto devices = MidiInput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices[i].name == midi_input_name)
        {
            midi_input = MidiInput::openDevice(devices[i].identifier, this);
            midi_input_name = devices[i].name;
            break;
        }
    }
    
    if (midi_input == nullptr)
    {
        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices[i].name.containsIgnoreCase(midi_input_name))
            {
                midi_input = MidiInput::openDevice(devices[i].identifier, this);
                midi_input_name = devices[i].name;
                break;
            }
        }
    }
    
    if (midi_input)
    {
        midi_input->start();
        midiIn_.swap(midi_input);
        fullMidiInName_ = midi_input_name;
        return true;
    }
    
    return false;
}

bool ApplicationState::isMidiInDeviceAvailable(const String& name)
{
    auto devices = MidiInput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices[i].name == name)
        {
            return true;
        }
    }
    return false;
}

void ApplicationState::handleIncomingMidiMessage(MidiInput*, const MidiMessage& msg)
{
    if (msg.isSysEx())
    {
        // we don't have any other application threads going on, so this is safe
        // even though ci::Device is not thread safe, in a proper application
        // this should done asynchronously on the message thread
        mpeProfile_->processMessage({0, msg.getSysExDataSpan()});
    }
}

void ApplicationState::processMessage(ump::BytesOnGroup umsg)
{
    // only process CI messages if both MIDI input and output is connected
    if (midiIn_.get())
    {
        if (auto out = midiOut_.get())
        {
            auto msg = MidiMessage::createSysExMessage(umsg.bytes);
            out->sendMessageNow(msg);
        }
    }
}

void ApplicationState::virtualDevice(const String& name)
{
#if (JUCE_LINUX || JUCE_MAC)
    midiOut_ = MidiOutput::createNewDevice(name);
    if (midiOut_ == nullptr)
    {
        std::cerr << "Couldn't create virtual MIDI output port \"" << name << "\"" << std::endl;
        JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
    }
    else
    {
        midiOutName_ = name;
    }
#else
    std::cerr << "Virtual MIDI output ports are not supported on Windows" << std::endl;
    JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
#endif
}

void ApplicationState::executeCurrentCommand()
{
    auto cmd = currentCommand_;
    currentCommand_ = ApplicationCommand::Dummy();
    cmd.execute(*this);
}

void ApplicationState::handleVarArgCommand()
{
    if (currentCommand_.expectedOptions_ < 0)
    {
        executeCurrentCommand();
    }
}

void ApplicationState::parseParameters(StringArray& parameters)
{
    for (auto param : parameters)
    {
        if (param == "--") continue;
        
        auto cmd = findApplicationCommand(param);
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
            auto timestamp = parseTimestamp(param);
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
                    auto now_counter = Time::getMillisecondCounter();
                    auto delta = (timestamp - lastTimeStamp_) - (now_counter - lastTimeStampCounter_);
                    
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
                auto file = File::getCurrentWorkingDirectory().getChildFile(param);
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

void ApplicationState::parseFile(File file)
{
    StringArray parameters;
    
    StringArray lines;
    file.readLines(lines);
    for (auto line : lines)
    {
        parameters.addArray(parseLineAsParameters(line));
    }
    
    parseParameters(parameters);
}

void ApplicationState::sendMidiMessage(MidiMessage&& msg)
{
    if (auto out = midiOut_.get())
    {
        out->sendMessageNow(msg);
    }
    else
    {
        static bool missingOutputPortWarningPrinted = false;
        if (!missingOutputPortWarningPrinted)
        {
            std::cerr << "No valid MIDI output port was specified for some of the messages" << std::endl;
            JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
            missingOutputPortWarningPrinted = true;
        }
    }
}

void ApplicationState::sendRPN(int channel, int number, int value)
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

void ApplicationState::negotiateMpeProfile(const String& name, int manager, int members)
{
    openInputDevice(name);
    if (midiIn_)
    {
        mpeProfile_->negotiate(manager, members);
    }
}

uint8 ApplicationState::asNoteNumber(String value)
{
    if (value.length() >= 2)
    {
        value = value.toUpperCase();
        auto first = value.substring(0, 1);
        if (first.containsOnly("CDEFGABH") &&
            value.substring(value.length()-1).containsOnly("1234567890"))
        {
            auto note = 0;
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

uint8 ApplicationState::asDecOrHex7BitValue(String value)
{
    return (uint8)limit7Bit(asDecOrHexIntValue(value));
}

uint16 ApplicationState::asDecOrHex14BitValue(String value)
{
    return (uint16)limit14Bit(asDecOrHexIntValue(value));
}

int ApplicationState::asDecOrHexIntValue(String value)
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

uint8 ApplicationState::limit7Bit(int value)
{
    return (uint8)jlimit(0, 0x7f, value);
}

uint16 ApplicationState::limit14Bit(int value)
{
    return (uint16)jlimit(0, 0x3fff, value);
}

void ApplicationState::printVersion()
{
    std::cout << ProjectInfo::projectName << " v" << ProjectInfo::versionString << std::endl;
    std::cout << "https://github.com/gbevin/SendMIDI" << std::endl;
}

void ApplicationState::printUsage()
{
    printVersion();
    std::cout << std::endl;
    std::cout << "Usage: " << ProjectInfo::projectName << " [ commands ] [ programfile ] [ -- ]" << std::endl << std::endl
    << "Commands:" << std::endl;
    for (auto&& cmd : commands_)
    {
        String param_option;
        param_option << "  " << cmd.param_.paddedRight(' ', 5);
        if (!cmd.optionsDescriptions_.isEmpty())
        {
            param_option << " " << cmd.optionsDescriptions_.getReference(0).paddedRight(' ', 13);
        }
        else
        {
            param_option << "              ";
        }
        param_option << "  ";
        param_option = param_option.substring(0, 23);
        std::cout << param_option;
        if (!cmd.commandDescriptions_.isEmpty())
        {
            std::cout << cmd.commandDescriptions_.getReference(0);
        }
        std::cout << std::endl;
        
        if (cmd.optionsDescriptions_.size() > 1)
        {
            auto i = 1;
            for (; i < cmd.optionsDescriptions_.size(); ++i)
            {
                auto line = cmd.optionsDescriptions_.getReference(i);
                String param_option2;
                param_option2 << "        " << line.paddedRight(' ', 13) << "  ";
                param_option2 = param_option2.substring(0, 23);
                std::cout << param_option2;
                
                if (i < cmd.commandDescriptions_.size())
                {
                    std::cout << cmd.commandDescriptions_.getReference(i);
                }
                
                std::cout << std::endl;
            }
            for (; i < cmd.commandDescriptions_.size(); ++i)
            {
                std::cout << "                       " << cmd.commandDescriptions_.getReference(i) << std::endl;
            }
        }
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
