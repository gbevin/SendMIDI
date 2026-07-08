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

// Runs the whole SendMIDI UnitTest suite. The individual tests register
// themselves through static instances in the other files in this folder. This
// mirrors the way JUCE tests its own modules (juce::UnitTest / UnitTestRunner).

class ConsoleTestRunner : public UnitTestRunner
{
    void logMessage(const String& message) override
    {
        std::cout << message << std::endl;
    }
};

class TestRunnerApplication : public JUCEApplicationBase
{
public:
    const String getApplicationName() override       { return "SendMIDITests"; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    void systemRequestedQuit() override              { quit(); }

    void initialise(const String& commandLine) override
    {
        ConsoleTestRunner runner;
        runner.setAssertOnFailure(false);

        if (commandLine.isNotEmpty() && !commandLine.startsWith("-"))
        {
            runner.runTestsInCategory(commandLine.trim());
        }
        else
        {
            runner.runAllTests();
        }

        int totalPasses = 0;
        int totalFailures = 0;
        for (int i = 0; i < runner.getNumResults(); ++i)
        {
            auto* result = runner.getResult(i);
            totalPasses += result->passes;
            totalFailures += result->failures;
        }

        std::cout << std::endl
                  << "============================================================" << std::endl
                  << (totalFailures == 0 ? "  ALL TESTS PASSED" : "  TESTS FAILED")
                  << "   (" << totalPasses << " passed, " << totalFailures << " failed)" << std::endl
                  << "============================================================" << std::endl;

        setApplicationReturnValue(totalFailures == 0 ? 0 : 1);
        quit();
    }

    void shutdown() override {}
    void suspended() override {}
    void resumed() override {}
    void anotherInstanceStarted(const String&) override {}
    void unhandledException(const std::exception*, const String&, int) override { jassertfalse; }
};

START_JUCE_APPLICATION (TestRunnerApplication)
