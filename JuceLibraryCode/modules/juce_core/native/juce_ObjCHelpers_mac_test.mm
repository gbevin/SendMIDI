namespace juce
{

class ObjCHelpersTest final : public UnitTest
{
public:
    ObjCHelpersTest() : UnitTest { "ObjCHelpers", UnitTestCategories::native } {}

    void runTest() final
    {
        beginTest ("Range");
        {
            constexpr auto start = 10;
            constexpr auto length = 20;

            const auto juceRange = Range<int>::withStartAndLength (start, length);
            const auto nsRange = NSMakeRange (start, length);

            expect (nsRangeToJuce (nsRange) == juceRange);
            expect (NSEqualRanges (nsRange, juceRangeToNS (juceRange)));
        }

        beginTest ("String");
        {
            String juceString { "Hello world!" };
            NSString *nsString { @"Hello world!" };

            expect (nsStringToJuce (nsString) == juceString);
            expect ([nsString isEqualToString: juceStringToNS (juceString)]);
            expect ([nsString isEqualToString: nsStringLiteral ("Hello world!")]);
        }

        beginTest ("StringArray");
        {
            const StringArray stringArray { "Hello world!", "this", "is", "a", "test" };
            NSArray *nsArray { @[@"Hello world!", @"this", @"is", @"a", @"test"] };

            expect ([nsArray isEqualToArray: createNSArrayFromStringArray (stringArray)]);
        }

        beginTest ("Dictionary");
        {
            DynamicObject::Ptr data { new DynamicObject() };
            data->setProperty ("integer", 1);
            data->setProperty ("double", 2.3);
            data->setProperty ("boolean", true);
            data->setProperty ("string", "Hello world!");

            Array<var> array { 45, 67.8, true, "Hello array!" };
            data->setProperty ("array", array);

            const auto* nsDictionary = varToNSDictionary (data.get());
            expect (nsDictionary != nullptr);

            const auto clone = nsDictionaryToVar (nsDictionary);
            expect (clone.isObject());

            expect (clone.getProperty ("integer", {}).isInt());
            expect (clone.getProperty ("double",  {}).isDouble());
            expect (clone.getProperty ("boolean", {}).isBool());
            expect (clone.getProperty ("string",  {}).isString());
            expect (clone.getProperty ("array",   {}).isArray());

            expect (clone.getProperty ("integer", {}) == var { 1 });
            expect (clone.getProperty ("double",  {}) == var { 2.3 });
            expect (clone.getProperty ("boolean", {}) == var { true });
            expect (clone.getProperty ("string",  {}) == var { "Hello world!" });
            expect (clone.getProperty ("array",   {}) == var { array });
        }

        beginTest ("varToNSDictionary converts a void variant to an empty dictionary");
        {
            var voidVariant;

            const auto* nsDictionary = varToNSDictionary (voidVariant);
            expect (nsDictionary != nullptr);

            const auto result = nsDictionaryToVar (nsDictionary);
            expect (result.isObject());
            expect (result.getDynamicObject()->getProperties().isEmpty());
        }
    }
};

static ObjCHelpersTest objCHelpersTest;

} // namespace juce
