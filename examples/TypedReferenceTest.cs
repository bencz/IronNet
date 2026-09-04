using System;

public static class TypedReferenceTest
{
    public static int Main()
    {
        int value = 42;
        TypedReference reference = __makeref(value);
        bool valueMatches = __refvalue(reference, int) == 42;
        bool typeMatches = __reftype(reference).FullName == "System.Int32";

        __refvalue(reference, int) = 73;
        bool writeMatches = value == 73;

        Console.WriteLine(valueMatches);
        Console.WriteLine(typeMatches);
        Console.WriteLine(writeMatches);

        return valueMatches && typeMatches && writeMatches ? 0 : 1;
    }
}
