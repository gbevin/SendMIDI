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

class ApplicationState;

class MpeProfileNegotiation : ci::ProfileDelegate
{
public:
    MpeProfileNegotiation(ApplicationState* state);
    void processMessage(ump::BytesOnGroup);
    
    void negotiate();
    
private:
    virtual void profileEnablementRequested(ci::MUID x,
                                            ci::ProfileAtAddress profileAtAddress,
                                            int numChannels,
                                            bool enabled);

    std::unique_ptr<ci::Device> ci_;
};
