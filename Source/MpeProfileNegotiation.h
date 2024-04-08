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

class MpeProfileNegotiation : ci::DeviceListener, Timer
{
public:
    MpeProfileNegotiation(ApplicationState* state);
    void processMessage(ump::BytesOnGroup);
    
    void negotiate(int manager, int members);
    bool isWaitingForNegotation();
    virtual void timerCallback() override;

private:
    static std::string muidToString(ci::MUID muid);

    void startNegotationTimer();
    
    virtual void deviceAdded(ci::MUID muid) override;
    
    virtual void profileStateReceived(ci::MUID muid,
                                      ci::ChannelInGroup destination) override;

    virtual void profileEnablementChanged(ci::MUID muid,
                                          ci::ChannelInGroup destination,
                                          ci::Profile profile,
                                          int numChannels) override;

    virtual void profileDetailsReceived(ci::MUID muid,
                                        ci::ChannelInGroup destination,
                                        ci::Profile profile,
                                        std::byte target,
                                        Span<const std::byte> data) override;

    static ci::Profile MPE_PROFILE;
    static std::byte TARGET_FEATURES_SUPPORTED;
    
    std::unique_ptr<ci::Device> ci_;
    ci::ChannelInGroup address_ { ci::ChannelInGroup::wholeGroup };
    int manager_ { 0 };
    int members_ { 0 };
    bool waiting_ { false };
};
