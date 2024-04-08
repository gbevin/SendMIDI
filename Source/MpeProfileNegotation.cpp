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

// MPE Profile ID
ci::Profile MpeProfileNegotiation::MPE_PROFILE = { std::byte(0x7E), std::byte(0x31), std::byte(0x00), std::byte(0x01), std::byte(0x01) };
    
MpeProfileNegotiation::MpeProfileNegotiation(ApplicationState* state)
{
    ci_ = std::make_unique<ci::Device>(ci::DeviceOptions()
                                       .withFeatures(ci::DeviceFeatures().withProfileConfigurationSupported())
                                       .withDeviceInfo( {
                                           ////////////////////////
                                           // IMPORTANT!
                                           //
                                           // This is Uwyn's SysEx ID, don't use for non-Uwyn products
                                           ////////////////////////
                                           { std::byte(0x5B), std::byte(0x02), std::byte(0x00) },
                                           // Uwyn open-source product family
                                           { std::byte(0x01), std::byte(0x00) },
                                           // Uwyn SendMIDI model number
                                           { std::byte(0x01), std::byte(0x00) },
                                           // Uwyn SendMIDI revision
                                           { std::byte(0x01), std::byte(0x00), std::byte(0x00), std::byte(0x00) }} )
                                       .withOutputs({ state }));
    
    ci_->addListener(*this);
}

void MpeProfileNegotiation::negotiate(int manager, int members)
{
    if (members > 0)
    {
        std::cout << "Initiator " << muidToString(ci_->getMuid()) << " negotating MPE Profile with manager channel " << manager << " and " << members << " member channel" << (members > 1 ? "s" : "") << std::endl;
    }
    else
    {
        std::cout << "Initiator " << muidToString(ci_->getMuid()) << " negotating MPE Profile with manager channel " << manager << " to be disabled" << std::endl;
    }
    
    ci_->getProfileHost()->addProfile({ MPE_PROFILE, ci::ChannelAddress().withChannel(ci::ChannelInGroup::wholeBlock) });

    waiting_ = true;
    address_ = (ci::ChannelInGroup)(manager - 1);
    manager_ = manager;
    members_ = members;

    ci_->sendDiscovery();
    startNegotationTimer();
}

void MpeProfileNegotiation::timerCallback()
{
    std::cerr << "Failed to negotiate MPE Profile." << std::endl;
    stopTimer();
    waiting_ = false;
}

bool MpeProfileNegotiation::isWaitingForNegotation()
{
    return waiting_;
}

void MpeProfileNegotiation::processMessage(ump::BytesOnGroup umsg)
{
    if (waiting_)
    {
        startNegotationTimer();
        ci_->processMessage(umsg);
    }
}

std::string MpeProfileNegotiation::muidToString(ci::MUID muid)
{
    std::stringstream s;
    s << "MUID 0x" << std::hex << std::setfill('0') << std::setw(8) << muid.get() << std::dec;
    return s.str();
}

void MpeProfileNegotiation::startNegotationTimer()
{
    startTimer(3000);
}

void MpeProfileNegotiation::deviceAdded(ci::MUID muid)
{
    std::cout << muidToString(muid) << " : Discovered" << std::endl;
    
    ci_->sendProfileInquiry(muid, address_);
}

void MpeProfileNegotiation::profileStateReceived(ci::MUID muid, ci::ChannelInGroup destination)
{
    auto* states = ci_->getProfileStateForMuid(muid, ci::ChannelAddress().withChannel(destination));
    if (states && states->size() > 0)
    {
        for (auto state : *states)
        {
            if (std::equal(MPE_PROFILE.begin(), MPE_PROFILE.end(), state.profile.begin()) && state.state.isSupported() && !state.state.isActive())
            {
                if (members_ > 0)
                {
                    std::cout << muidToString(muid) << " : Requesting MPE profile enablement with manager channel " << manager_ << " and " << members_ << " member channel" << (members_ > 1 ? "s" : "") << std::endl;
                    ci_->sendProfileEnablement(muid, address_, MPE_PROFILE, members_ + 1);
                }
                else
                {
                    std::cout << muidToString(muid) << " : Requesting MPE profile disablement with manager channel " << manager_ << std::endl;
                    ci_->sendProfileEnablement(muid, address_, MPE_PROFILE, 0);
                }
            }
        }
    }
}

void MpeProfileNegotiation::profileEnablementChanged(ci::MUID muid, ci::ChannelInGroup destination, ci::Profile profile, int numChannels)
{
    auto manager = (int)destination;
    if (numChannels > 0)
    {
        auto members = numChannels - 1;
        std::cout << muidToString(muid) << " : MPE profile enabled with manager channel " << (manager + 1) << " and " << members << " member channel" << (members > 1 ? "s" : "") << std::endl;
        waiting_ = false;
    }
    else
    {
        std::cout << muidToString(muid) << " : MPE profile disabled with manager channel " << (manager + 1) << std::endl;
    }
}
