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
#include "JuceHeader.h"

#include "ApplicationState.h"

class sendMidiApplication : public JUCEApplicationBase
{
public:
    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    void systemRequestedQuit() override              { quit(); }
    
    void initialise(const String&) override
    {
        state.initialise(*this);
    }
    
    void shutdown() override {}
    void suspended() override {}
    void resumed() override {}
    void anotherInstanceStarted(const String&) override {}
    void unhandledException(const std::exception*, const String&, int) override { jassertfalse; }
    
    ApplicationState state;
};

START_JUCE_APPLICATION (sendMidiApplication)
