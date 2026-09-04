using System;

internal struct CopyValue
{
    public int Number;
    public object Reference;
}

internal static class ValueCopyTest
{
    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static CopyValue MakeValue(int number, object reference)
    {
        CopyValue result;
        result.Number = number;
        result.Reference = reference;
        return result;
    }

    private static void ReplaceThroughReference(ref CopyValue value, CopyValue replacement)
    {
        ref CopyValue alias = ref value;
        value = replacement;
        Require(alias.Number == replacement.Number, "stobj invalidated an existing managed reference.");
        alias.Number++;
        Require(value.Number == replacement.Number + 1, "A managed reference points to stale struct storage.");
    }

    private static int MutateArgument(CopyValue value)
    {
        CopyValue copy = value;
        value.Number = 91;
        return copy.Number;
    }

    public static void Main()
    {
        object reference = new object();
        CopyValue original = MakeValue(7, reference);
        CopyValue copy = original;
        original.Number = 9;
        Require(copy.Number == 7, "Struct assignment shared the source storage.");
        Require(ReferenceEquals(copy.Reference, reference), "Copying a struct deep-copied its reference field.");
        Require(MutateArgument(original) == 9 && original.Number == 9, "A by-value argument shared storage.");

        ref CopyValue alias = ref original;
        original = MakeValue(21, reference);
        Require(alias.Number == 21, "stloc invalidated an existing managed reference.");
        ReplaceThroughReference(ref original, MakeValue(35, reference));
        Require(original.Number == 36 && alias.Number == 36, "Nested managed references lost the updated value.");

        object boxed = original;
        original.Number = 40;
        CopyValue unboxed = (CopyValue)boxed;
        Require(unboxed.Number == 36, "Boxing did not capture an independent value.");
        unboxed.Number = 50;
        Require(((CopyValue)boxed).Number == 36, "unbox.any shared boxed storage.");

        CopyValue[] array = new CopyValue[1];
        array[0] = original;
        original.Number = 60;
        Require(array[0].Number == 40, "A struct array element shared source storage.");
        GC.Collect();
        Require(ReferenceEquals(array[0].Reference, reference), "GC lost the reference in copied struct storage.");
        Console.WriteLine("Value copy test passed");
    }
}
