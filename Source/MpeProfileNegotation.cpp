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

#include "MpeProfileNegotiation.h"

#include "ApplicationState.h"

MpeProfileNegotiation::MpeProfileNegotiation(ApplicationState* state)
{
    ci_ = std::make_unique<ci::Device>(ci::DeviceOptions()
                                       .withFeatures(ci::DeviceFeatures().withProfileConfigurationSupported())
                                       .withDeviceInfo( {
                                           { std::byte(0), std::byte(0), std::byte(0) },
                                           { std::byte(0), std::byte(0) },
                                           { std::byte(0), std::byte(0) },
                                           { std::byte(0), std::byte(0), std::byte(0), std::byte(0) }} )
                                       .withOutputs({ state })
                                       .withProfileDelegate(this));
    ci::ProfileAtAddress mpe_profile
    {
        { std::byte(0x7e), std::byte(0x31), std::byte(0x00), std::byte(0x01), std::byte(0x01) },
        ci::ChannelAddress().withChannel(juce::midi_ci::ChannelInGroup::wholeBlock)
    };
    ci_->getProfileHost()->addProfile(mpe_profile);
}

void MpeProfileNegotiation::negotiate()
{
    ci_->sendDiscovery();
}

void MpeProfileNegotiation::processMessage(ump::BytesOnGroup umsg)
{
    ci_->processMessage(umsg);
}

void MpeProfileNegotiation::profileEnablementRequested(ci::MUID x, ci::ProfileAtAddress profileAtAddress, int numChannels, bool enabled)
{
    std::cout << "profileEnablementRequested" << std::endl;
    auto host = ci_->getProfileHost();
    if (enabled)
    {
        host->setProfileEnablement(profileAtAddress, numChannels);
    }
    else
    {
        host->setProfileEnablement(profileAtAddress, -1);
    }
}
