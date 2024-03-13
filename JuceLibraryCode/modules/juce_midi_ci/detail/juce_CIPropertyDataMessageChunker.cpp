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

namespace juce::midi_ci::detail
{

PropertyDataMessageChunker::PropertyDataMessageChunker (std::vector<std::byte>& storageIn,
                                                        int chunkSizeIn,
                                                        const std::byte messageKindIn,
                                                        const std::byte requestIdIn,
                                                        Span<const std::byte> headerIn,
                                                        MUID sourceIn,
                                                        MUID destIn,
                                                        InputStream& bodyIn)
    : header (headerIn),
      storage (&storageIn),
      body (&bodyIn),
      source (sourceIn),
      dest (destIn),
      chunkSize (chunkSizeIn),
      messageKind (messageKindIn),
      requestId (requestIdIn)
{
    if (hasRoomForBody())
    {
        populateStorage();
    }
    else
    {
        // Header too large! There's no way to fit this message into the requested chunk size.
        jassertfalse;
        *this = PropertyDataMessageChunker();
    }
}

PropertyDataMessageChunker& PropertyDataMessageChunker::operator++() noexcept
{
    if (*this != PropertyDataMessageChunker())
    {
        if (body->isExhausted())
        {
            *this = PropertyDataMessageChunker();
        }
        else
        {
            ++thisChunk;
            populateStorage();
        }
    }

    return *this;
}

Span<const std::byte> PropertyDataMessageChunker::operator*() const noexcept
{
    // The end of the stream was reached, no point dereferencing the iterator now!
    jassert (storage != nullptr && (int) storage->size() <= chunkSize);
    return *storage;
}

Span<const std::byte> PropertyDataMessageChunker::getHeaderForBlock() const
{
    return thisChunk == 1 ? header : Span<const std::byte>{};
}

int PropertyDataMessageChunker::getRoomForBody() const
{
    return chunkSize - (int) (getHeaderForBlock().size() + 22);
}

bool PropertyDataMessageChunker::hasRoomForBody() const
{
    const auto bodyRoom = getRoomForBody();
    return (0 < bodyRoom)
           || (0 == bodyRoom && body->getNumBytesRemaining() == 0);
}

void PropertyDataMessageChunker::populateStorage() const
{
    storage->clear();
    storage->resize ((size_t) getRoomForBody());

    // Read body data into buffer
    const auto numBytesRead = (uint16_t) jmax (ssize_t (0), body->read (storage->data(), storage->size()));

    const auto [numChunks, thisChunkNum] = [&]() -> std::tuple<uint16_t, uint16_t>
    {
        if (body->isExhausted() || body->getNumBytesRemaining() == 0)
            return std::tuple (thisChunk, thisChunk);

        const auto totalLength = body->getTotalLength();

        if (totalLength < 0)
            return std::tuple ((uint16_t) 0, thisChunk); // 0 means "unknown number"

        const auto roomForBody = getRoomForBody();

        if (roomForBody != 0)
            return std::tuple ((uint16_t) ((totalLength + roomForBody - 1) / roomForBody), thisChunk);

        // During construction, the input stream reported that it had no data remaining, so no
        // space was reserved for body content.
        // Now, the input stream reports that it has data remaining, but there's nowhere
        // to fit it in the message!
        jassertfalse;
        return std::tuple (thisChunk, (uint16_t) 0); // 0 means "data potentially unusable"
    }();

    // Now we know how many bytes we managed to read, write the header at the end of the buffer
    const auto headerForBlock = getHeaderForBlock();
    detail::Marshalling::Writer writer { *storage };
    writer (Message::Header { ChannelInGroup::wholeBlock,
                              messageKind,
                              detail::MessageMeta::implementationVersion,
                              source,
                              dest },
            requestId,
            detail::MessageMeta::makeSpanWithSizeBytes<2> (headerForBlock),
            numChunks,
            thisChunkNum,
            numBytesRead);

    // Finally, swap the header to the beginning of the buffer
    std::rotate (storage->begin(), storage->begin() + getRoomForBody(), storage->end());

    // ...and bring the storage buffer down to size, if we didn't manage to fill it
    const auto room = (size_t) getRoomForBody();
    storage->resize (storage->size() + numBytesRead - room);
}

} // namespace juce::midi_ci::detail
