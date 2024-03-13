/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

//==============================================================================
/**
    A smart-pointer that automatically creates and manages the lifetime of a
    shared static instance of a class.

    The SharedObjectType template type indicates the class to use for the shared
    object - the only requirements on this class are that it must have a public
    default constructor and destructor.

    The SharedResourcePointer offers a pattern that differs from using a singleton or
    static instance of an object, because it uses reference-counting to make sure that
    the underlying shared object is automatically created/destroyed according to the
    number of SharedResourcePointer objects that exist. When the last one is deleted,
    the underlying object is also immediately destroyed. This allows you to use scoping
    to manage the lifetime of a shared resource.

    Note: The construction/deletion of the shared object must not involve any
    code that makes recursive calls to a SharedResourcePointer, or you'll cause
    a deadlock.

    Example:
    @code
    // An example of a class that contains the shared data you want to use.
    struct MySharedData
    {
        // There's no need to ever create an instance of this class directly yourself,
        // but it does need a public constructor that does the initialisation.
        MySharedData()
        {
            sharedStuff = generateHeavyweightStuff();
        }

        Array<SomeKindOfData> sharedStuff;
    };

    struct DataUserClass
    {
        DataUserClass()
        {
            // Multiple instances of the DataUserClass will all have the same
            // shared common instance of MySharedData referenced by their sharedData
            // member variables.
            useSharedStuff (sharedData->sharedStuff);
        }

        // By keeping this pointer as a member variable, the shared resource
        // is guaranteed to be available for as long as the DataUserClass object.
        SharedResourcePointer<MySharedData> sharedData;
    };

    @endcode

    @tags{Core}
*/
template <typename SharedObjectType>
class SharedResourcePointer
{
public:
    /** Creates an instance of the shared object.
        If other SharedResourcePointer objects for this type already exist, then
        this one will simply point to the same shared object that they are already
        using. Otherwise, if this is the first SharedResourcePointer to be created,
        then a shared object will be created automatically.
    */
    SharedResourcePointer() = default;

    /** Copy constructor. */
    SharedResourcePointer (const SharedResourcePointer&) = default;

    /** Move constructor. */
    SharedResourcePointer (SharedResourcePointer&&) noexcept = default;

    /** Destructor.
        If no other SharedResourcePointer objects exist, this will also delete
        the shared object to which it refers.
    */
    ~SharedResourcePointer() = default;

    /** Returns a pointer to the shared object. */
    operator SharedObjectType*() const noexcept         { return sharedObject.get(); }

    /** Returns a reference to the shared object. */
    SharedObjectType& get() const noexcept              { return *sharedObject; }

    /** Returns a reference to the shared object. */
    SharedObjectType& getObject() const noexcept        { return *sharedObject; }

    /** Returns a pointer to the shared object. */
    SharedObjectType* operator->() const noexcept       { return sharedObject.get(); }

    /** Returns a reference to the shared object. */
    SharedObjectType& operator*() const noexcept        { return *sharedObject; }

   #ifndef DOXYGEN
    [[deprecated ("If you are relying on this function please inform the JUCE team as we are planing on removing this in a subsequent release")]]
    int getReferenceCount() const noexcept              { return (int) sharedObject.use_count(); }
   #endif

    /** Returns the SharedResourcePointer if one already exists, or a null optional otherwise. */
    static std::optional<SharedResourcePointer> getSharedObjectWithoutCreating()
    {
        if (auto sharedPtr = weak().lock())
            return SharedResourcePointer { std::move (sharedPtr) };

        return {};
    }

private:
    explicit SharedResourcePointer (std::shared_ptr<SharedObjectType>&& other) noexcept
        : sharedObject (std::move (other))
    {
        jassert (sharedObject != nullptr);
    }

    class Weak
    {
    public:
        std::shared_ptr<SharedObjectType> lock()
        {
            const SpinLock::ScopedLockType lock { mutex };
            return ptr.lock();
        }

        std::shared_ptr<SharedObjectType> lockOrCreate()
        {
            const SpinLock::ScopedLockType lock { mutex };

            if (auto locked = ptr.lock())
                return locked;

            const std::shared_ptr<SharedObjectType> shared (new SharedObjectType());
            ptr = shared;
            return shared;
        }

    private:
        SpinLock mutex;
        std::weak_ptr<SharedObjectType> ptr;
    };

    inline static Weak& weak()
    {
        static Weak weak;
        return weak;
    }

    std::shared_ptr<SharedObjectType> sharedObject = weak().lockOrCreate();

    // There's no need to assign to a SharedResourcePointer because every
    // instance of the class is exactly the same!
    SharedResourcePointer& operator= (const SharedResourcePointer&) = delete;
    SharedResourcePointer& operator= (SharedResourcePointer&&) noexcept = delete;

    JUCE_LEAK_DETECTOR (SharedResourcePointer)
};

} // namespace juce
