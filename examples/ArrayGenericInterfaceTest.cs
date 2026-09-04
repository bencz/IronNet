using System;
using System.Collections.Generic;

public static class ArrayGenericInterfaceTest
{
    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static void VerifyGenericContext<T>(T[] source, T expectedFirst, T replacement)
    {
        IList<T> list = source;
        ICollection<T> collection = source;

        Require(list.Count == source.Length, "Generic array Count failed.");
        Require(collection.IsReadOnly, "Generic array IsReadOnly failed.");
        Require(EqualityComparer<T>.Default.Equals(list[0], expectedFirst), "Generic array indexer getter failed.");
        Require(collection.Contains(expectedFirst), "Generic array Contains failed.");
        Require(list.IndexOf(expectedFirst) == 0, "Generic array IndexOf failed.");

        list[0] = replacement;
        Require(EqualityComparer<T>.Default.Equals(source[0], replacement), "Generic array indexer setter failed.");

        T[] destination = new T[source.Length + 2];
        collection.CopyTo(destination, 1);
        for (int index = 0; index < source.Length; index++)
        {
            Require(EqualityComparer<T>.Default.Equals(destination[index + 1], source[index]), "Generic array CopyTo failed.");
        }
    }

    private static void VerifyFixedSizeOperations(ICollection<int> collection, IList<int> list)
    {
        bool addRejected = false;
        bool clearRejected = false;
        bool removeRejected = false;
        bool insertRejected = false;
        bool removeAtRejected = false;

        try
        {
            collection.Add(4);
        }
        catch (NotSupportedException)
        {
            addRejected = true;
        }

        try
        {
            collection.Clear();
        }
        catch (NotSupportedException)
        {
            clearRejected = true;
        }

        try
        {
            collection.Remove(2);
        }
        catch (NotSupportedException)
        {
            removeRejected = true;
        }

        try
        {
            list.Insert(0, 4);
        }
        catch (NotSupportedException)
        {
            insertRejected = true;
        }

        try
        {
            list.RemoveAt(0);
        }
        catch (NotSupportedException)
        {
            removeAtRejected = true;
        }

        Require(addRejected && clearRejected && removeRejected && insertRejected && removeAtRejected, "Fixed-size array mutation semantics failed.");
    }

    public static int Main()
    {
        int[] numbers = new int[] { 1, 2, 3 };
        VerifyGenericContext(numbers, 1, 10);
        VerifyFixedSizeOperations(numbers, numbers);

        string[] names = new string[] { "alpha", null, "gamma" };
        VerifyGenericContext(names, "alpha", "updated");
        Require(((ICollection<string>)names).Contains(null), "Reference array null lookup failed.");

        IEnumerable<object> covariant = names;
        int enumerated = 0;
        foreach (object value in covariant)
        {
            if (enumerated == 0)
            {
                Require((string)value == "updated", "Covariant array enumeration returned an invalid value.");
            }
            enumerated++;
        }

        Require(enumerated == names.Length, "Covariant array enumeration length failed.");
        Console.WriteLine("Array generic interface test passed");
        return 0;
    }
}
