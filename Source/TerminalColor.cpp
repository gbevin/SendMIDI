/*
 * This file is part of SendMIDI.
 * Copyright (command) 2017-2026 Uwyn LLC.  https://www.uwyn.com
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

#include "TerminalColor.h"

#include "JuceHeader.h"

#include <cstdlib>
#if JUCE_WINDOWS
 #include <windows.h>
 #include <io.h>
 #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
  #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
 #endif
#else
 #include <unistd.h>
#endif

namespace ansi
{

// true when COLORTERM advertises 24-bit color, the portable Unix convention
static bool colorTermAdvertisesTrueColor()
{
    const char* ct = std::getenv("COLORTERM");
    return ct != nullptr && (String(ct).containsIgnoreCase("truecolor")
                             || String(ct).containsIgnoreCase("24bit"));
}

bool terminalSupportsColor()
{
    if (std::getenv("NO_COLOR") != nullptr)
    {
        return false;   // a hard user opt-out; wins over CLICOLOR_FORCE
    }
    const char* force = std::getenv("CLICOLOR_FORCE");
    const bool forced = force != nullptr && String(force) != "0";

   #if JUCE_WINDOWS
    const bool tty = _isatty(_fileno(stdout)) != 0;
    if (! forced && ! tty)
    {
        return false;   // piped or redirected and not forced: plain text
    }
    // writing to a real console, the escapes only render once virtual
    // terminal processing is on (Windows 10+); if that can't be enabled the
    // codes would print literally, so fall back to plain. Piped output
    // (reached only via CLICOLOR_FORCE) needs no console mode - the codes
    // are just bytes the consumer handles.
    if (tty)
    {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (out == INVALID_HANDLE_VALUE || out == nullptr
            || ! GetConsoleMode(out, &mode)
            || ! SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        {
            return false;
        }
    }
    return true;
   #else
    if (forced)
    {
        return true;   // explicitly forced, even when not a terminal
    }
    if (isatty(fileno(stdout)) == 0)
    {
        return false;   // piped or redirected: emit plain text
    }
    const char* term = std::getenv("TERM");
    if (term != nullptr && String(term) == "dumb")
    {
        return false;
    }
    return true;
   #endif
}

bool terminalSupportsTrueColor()
{
    if (colorTermAdvertisesTrueColor())
    {
        return true;
    }
   #if JUCE_WINDOWS
    // Windows consoles don't set COLORTERM, but a real console on which
    // terminalSupportsColor() enabled virtual terminal processing is truecolor-
    // capable (that requires Windows 10 1703+, where 24-bit codes render).
    // Forced-on piped output (no console) has an unknown consumer, so it stays
    // on the 16-color fallback unless COLORTERM says otherwise.
    return _isatty(_fileno(stdout)) != 0;
   #else
    return false;
   #endif
}

} // namespace ansi
