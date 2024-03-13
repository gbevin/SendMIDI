/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce::midi_ci
{

/**
    Holds a profile ID, along with the number of supported and active channels
    corresponding to that profile.

    @tags{Audio}
*/
struct ProfileStateEntry
{
    Profile profile;            ///< A MIDI-CI profile ID
    SupportedAndActive state;   ///< The number of channels corresponding to the profile

    bool operator< (const Profile& other) const { return profile < other; }
    bool operator< (const ProfileStateEntry& other) const { return profile < other.profile; }
};

//==============================================================================
/**
    Holds the number of channels that are supported and activated for all profiles
    at a particular channel address.

    @tags{Audio}
*/
class ChannelProfileStates
{
public:
    using Entry = ProfileStateEntry;

    /** Returns the number of channels that are supported and active for the
        given profile.
    */
    SupportedAndActive get (const Profile& profile) const;

    /** Returns all profiles that are active at this address. */
    std::vector<Profile> getActive() const;

    /** Returns all profiles that are supported but inactive at this address. */
    std::vector<Profile> getInactive() const;

    /** Sets the number of channels that are supported/active for a given profile. */
    void set (const Profile& profile, SupportedAndActive state);

    /** Removes the record of a particular profile, equivalent to removing support. */
    void erase (const Profile& profile);

    /** Gets a const iterator over all profiles, for range-for compatibility. */
    auto begin() const { return entries.begin(); }

    /** Gets a const iterator over all profiles, for range-for compatibility. */
    auto end()   const { return entries.end(); }

    /** Returns true if no profiles are supported. */
    auto empty() const { return entries.empty(); }

    /** Returns the number of profiles that are supported at this address. */
    auto size()  const { return entries.size(); }

private:
    std::vector<Entry> entries;
};

//==============================================================================
/**
    Contains profile states for each channel in a group, along with the state
    of profiles that apply to the group itself.

    @tags{Audio}
*/
class GroupProfileStates
{
    template <typename This>
    static auto getStateForDestinationImpl (This& t, ChannelInGroup destination) -> decltype (&t.groupState)
    {
        if (destination == ChannelInGroup::wholeGroup)
            return &t.groupState;

        if (const auto index = (size_t) destination; index < t.channelStates.size())
            return &t.channelStates[index];

        return nullptr;
    }

public:
    /** Returns the profile state for the group or a contained channel as appropriate.
        Returns nullptr if ChannelInGroup refers to a whole function block.
    */
    auto* getStateForDestination (ChannelInGroup d)       { return getStateForDestinationImpl (*this, d); }

    /** Returns the profile state for the group or a contained channel as appropriate.
        Returns nullptr if ChannelInGroup refers to a whole function block.
    */
    auto* getStateForDestination (ChannelInGroup d) const { return getStateForDestinationImpl (*this, d); }

    std::array<ChannelProfileStates, 16> channelStates; ///< Profile states for each channel in the group
    ChannelProfileStates groupState;                    ///< Profile states for the group itself
};

//==============================================================================
/**
    Contains profile states for each group and channel in a function block, along with the state
    of profiles that apply to the function block itself.

    @tags{Audio}
*/
class BlockProfileStates
{
    template <typename This>
    static auto getStateForDestinationImpl (This& t, ChannelAddress address) -> decltype (&t.blockState)
    {
        if (address.isBlock())
            return &t.blockState;

        if (const auto index = (size_t) address.getGroup(); index < t.groupStates.size())
            return t.groupStates[index].getStateForDestination (address.getChannel());

        return nullptr;
    }

public:
    /** Returns the profile state for the function block, group, or channel as appropriate.
        Returns nullptr if the address refers to a non-existent channel or group.
    */
    auto* getStateForDestination (ChannelAddress address)       { return getStateForDestinationImpl (*this, address); }

    /** Returns the profile state for the function block, group, or channel as appropriate.
        Returns nullptr if the address refers to a non-existent channel or group.
    */
    auto* getStateForDestination (ChannelAddress address) const { return getStateForDestinationImpl (*this, address); }

    std::array<GroupProfileStates, 16> groupStates; ///< Profile states for each group in the function block
    ChannelProfileStates blockState;                ///< Profile states for the whole function block
};

} // namespace juce::midi_ci
