/*
 * This file is part of SendMIDI.
 * Copyright (command) 2017-2026 Uwyn LLC.  https://www.uwyn.com
 *
 * RouteMIDI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RouteMIDI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// Lives in its own translation unit because the Windows implementation needs
// <windows.h>, whose INPUT and DECIMAL types clash with the CommandIndex
// enumerators in ApplicationCommand.h.
namespace ansi
{
    // whether standard output is an interactive terminal expected to render
    // ANSI escape codes: NO_COLOR always wins, CLICOLOR_FORCE forces color on,
    // TERM=dumb opts out. On Windows the console's virtual terminal processing
    // is turned on first (Windows 10+); if it can't be enabled, output stays
    // plain.
    bool terminalSupportsColor();

    // whether that terminal accepts 24-bit truecolor codes (vs the nearest
    // 16-color fallback). On Unix this is advertised through COLORTERM; on
    // Windows a real console that accepted virtual terminal processing is
    // truecolor-capable (Windows 10 1703+), even though it sets no COLORTERM.
    // Only meaningful when terminalSupportsColor() is true.
    bool terminalSupportsTrueColor();
}
