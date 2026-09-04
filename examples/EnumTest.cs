using System;

enum SignedCode : int
{
    Zero = 0,
    One = 1,
    AliasOne = 1,
    Largest = 2147483647,
    Negative = -1
}

enum WideCode : ulong
{
    Small = 2,
    Large = 0xFEDCBA9876543210ul
}

[Flags]
enum AccessCode : byte
{
    None = 0,
    Read = 1,
    Write = 2,
    Execute = 4
}

class EnumTest
{
    static void Require(bool condition, string message)
    {
        if (!condition)
            throw new Exception(message);
    }

    static int Main()
    {
        Require(SignedCode.Negative.ToString() == "Negative", "Enum.ToString did not resolve a signed constant name.");
        Require(((SignedCode)123).ToString() == "123", "Enum.ToString did not format an unnamed value.");
        Require(WideCode.Large.ToString() == "Large", "Enum.ToString did not resolve a 64-bit unsigned constant.");
        Require((AccessCode.Read | AccessCode.Execute).ToString() == "Read, Execute", "Enum.ToString did not compose a flags value.");
        Require(((AccessCode)8).ToString() == "8", "Enum.ToString did not format an unknown flags value numerically.");

        Require(Enum.IsDefined(typeof(SignedCode), SignedCode.One), "Enum.IsDefined rejected an enum value.");
        Require(Enum.IsDefined(typeof(SignedCode), 1), "Enum.IsDefined rejected an underlying integral value.");
        Require(Enum.IsDefined(typeof(SignedCode), "Negative"), "Enum.IsDefined rejected a constant name.");
        Require(!Enum.IsDefined(typeof(SignedCode), 42), "Enum.IsDefined accepted an unknown value.");
        Require(!Enum.IsDefined(typeof(SignedCode), "negative"), "Enum.IsDefined must compare names with ordinal casing.");

        Require(Enum.GetName(typeof(SignedCode), -1) == "Negative", "Enum.GetName failed for a negative value.");
        Require(Enum.GetName(typeof(SignedCode), 42) == null, "Enum.GetName must return null for an unnamed value.");

        string[] names = Enum.GetNames(typeof(SignedCode));
        Require(names.Length == 5, "Enum.GetNames returned the wrong number of constants.");
        Require(names[0] == "Zero" && names[1] == "One" && names[2] == "AliasOne", "Enum.GetNames did not preserve sorted duplicate values.");
        Require(names[3] == "Largest" && names[4] == "Negative", "Enum.GetNames did not use unsigned value ordering.");

        SignedCode[] values = (SignedCode[])Enum.GetValues(typeof(SignedCode));
        Require(values.Length == 5, "Enum.GetValues returned the wrong number of constants.");
        Require(values[0] == SignedCode.Zero && values[1] == SignedCode.One && values[2] == SignedCode.AliasOne, "Enum.GetValues returned invalid leading values.");
        Require(values[3] == SignedCode.Largest && values[4] == SignedCode.Negative, "Enum.GetValues returned invalid sorted values.");

        object boxedOne = SignedCode.One;
        object boxedAlias = SignedCode.AliasOne;
        object boxedWide = WideCode.Small;
        Require(boxedOne.Equals(boxedAlias), "Enum.Equals rejected equal values of the same enum type.");
        Require(!boxedOne.Equals(boxedWide), "Enum.Equals accepted values of different enum types.");
        Require(boxedOne.GetHashCode() == boxedAlias.GetHashCode(), "Equal enum values produced different hash codes.");

        Console.WriteLine("Enum test passed");
        return 0;
    }
}
