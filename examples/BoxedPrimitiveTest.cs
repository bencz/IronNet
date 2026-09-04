using System;

class BoxedPrimitiveTest
{
    static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    static void RequireText(object value, string expected, string typeName)
    {
        string actual = value.ToString();

        Console.WriteLine(typeName + ": " + actual);
        Require(actual == expected, typeName + " boxed ToString returned '" + actual + "' instead of '" + expected + "'.");
    }

    static int Main()
    {
        RequireText(true, "True", "Boolean");
        RequireText(false, "False", "Boolean");
        RequireText('Z', "Z", "Char");
        RequireText((sbyte)-128, "-128", "SByte");
        RequireText((byte)255, "255", "Byte");
        RequireText((short)-32768, "-32768", "Int16");
        RequireText((ushort)65535, "65535", "UInt16");
        RequireText(int.MinValue, "-2147483648", "Int32");
        RequireText(uint.MaxValue, "4294967295", "UInt32");
        RequireText(long.MinValue, "-9223372036854775808", "Int64");
        RequireText(ulong.MaxValue, "18446744073709551615", "UInt64");
        RequireText(1.5f, "1.5", "Single");
        RequireText(-2.25, "-2.25", "Double");

        Require(((int)42).ToString() == "42", "Unboxed Int32.ToString returned the wrong value.");
        Require(((float)1.5f).ToString() == "1.5", "Unboxed Single.ToString returned the wrong value.");

        Console.WriteLine("Boxed primitive test passed");
        return 0;
    }
}
