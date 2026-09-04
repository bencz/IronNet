using System;
using System.Collections;
using System.Collections.Generic;

public static class ArrayCollectionTest
{
    private sealed class Item
    {
        public int Value;

        public Item(int value)
        {
            Value = value;
        }
    }

    private struct Record
    {
        public Item Item;
        public long Number;
    }

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            Console.WriteLine(message);
            throw new Exception(message);
        }
    }

    private static void Throws<TException>(Action action) where TException : Exception
    {
        Exception caught = null;
        try
        {
            action();
        }
        catch (Exception exception)
        {
            caught = exception;
        }

        Require(caught != null && caught.GetType() == typeof(TException), "Expected " + typeof(TException).FullName + ", received " + caught);
    }

    private static void TestInterfaces()
    {
        int[] values = { 7, 9, 7 };
        Array array = values;
        IList list = values;
        ICollection collection = values;

        Require(array.IsFixedSize && !array.IsReadOnly && !array.IsSynchronized, "Array public flags");
        Require(list.IsFixedSize && !list.IsReadOnly && !collection.IsSynchronized && collection.Count == 3, "Array interface flags");
        Require(object.ReferenceEquals(array, collection.SyncRoot) && object.ReferenceEquals(array, array.SyncRoot), "Array sync root identity");
        Require(object.ReferenceEquals(((ICollection)Array.AsReadOnly(values)).SyncRoot, array), "Read-only wrapper shares the array sync root");
        Require(list.Contains(7) && list.IndexOf(7) == 0 && !list.Contains("7") && list.IndexOf(null) == -1, "Non-generic lookup uses boxed equality");

        list[1] = 12;
        Require(values[1] == 12 && (int)list[1] == 12, "IList element mutation");
        list[1] = null;
        Require(values[1] == 0, "Null assignment resets value-type elements");
        Throws<InvalidCastException>(() => list[1] = "wrong");
        Throws<IndexOutOfRangeException>(() => { object ignored = list[-1]; });
        Throws<IndexOutOfRangeException>(() => list[3] = 1);

        Throws<NotSupportedException>(() => list.Add(4));
        Throws<NotSupportedException>(() => list.Insert(-1, "wrong"));
        Throws<NotSupportedException>(() => list.Remove("absent"));
        Throws<NotSupportedException>(() => list.RemoveAt(-1));
        Throws<NotSupportedException>(() => ((ICollection<int>)values).Clear());

        list.Clear();
        Require(values.Length == 3 && values[0] == 0 && values[1] == 0 && values[2] == 0, "IList.Clear preserves size and clears elements");

        int?[] nullable = { 8, null };
        IList nullableList = nullable;
        Require(nullableList.Contains(null) && nullableList.IndexOf(8) == 0, "Nullable lookup");
        nullableList[0] = null;
        nullableList[1] = 19;
        Require(!nullable[0].HasValue && nullable[1].Value == 19, "Nullable IList writes");
    }

    private static void TestBoundsAndSearch()
    {
        Array array = Array.CreateInstance(typeof(int), new int[] { 4 }, new int[] { -3 });
        IList list = (IList)array;
        for (int index = -3; index <= 0; index++)
        {
            list[index] = index + 10;
        }

        Require((int)list[-3] == 7 && list.IndexOf(9) == -1 && list.Contains(9), "Negative index can identify a found element");
        Require(list.IndexOf(100) == -4 && !list.Contains(100), "Missing value sentinel is lower bound minus one");
        Require(Array.IndexOf(array, 9, -2) == -1 && Array.IndexOf(array, 9, -3, 2) == -4, "IndexOf ranges");
        Require(Array.IndexOf(array, null, 1, 0) == -4, "Empty search at end");
        Throws<ArgumentOutOfRangeException>(() => Array.IndexOf(array, 7, -4));
        Throws<ArgumentOutOfRangeException>(() => Array.IndexOf(array, 7, 2));
        Throws<ArgumentOutOfRangeException>(() => Array.IndexOf(array, 7, -3, -1));
        Throws<ArgumentOutOfRangeException>(() => Array.IndexOf(array, 7, 0, 2));
        Throws<ArgumentNullException>(() => Array.IndexOf(null, null));

        Array.Reverse(array, -2, 3);
        Require((int)list[-3] == 7 && (int)list[-2] == 10 && (int)list[0] == 8, "Reverse a bounded range");
        Array.Reverse(array);
        Require((int)list[-3] == 8 && (int)list[0] == 7, "Reverse entire bounded array");
        Array.Reverse(array, 1, 0);
        Throws<ArgumentOutOfRangeException>(() => Array.Reverse(array, -4, 0));
        Throws<ArgumentOutOfRangeException>(() => Array.Reverse(array, -3, -1));
        Throws<ArgumentException>(() => Array.Reverse(array, 0, 2));
        Throws<ArgumentException>(() => Array.Reverse(array, 2, 0));

        Array.Clear(array, -2, 2);
        Require((int)list[-3] == 8 && (int)list[-2] == 0 && (int)list[-1] == 0 && (int)list[0] == 7, "Clear relative to lower bound");
        list.Clear();
        Require((int)list[-3] == 0 && (int)list[0] == 0, "Clear bounded IList");
        Array.Clear(array, 1, 0);
        Throws<IndexOutOfRangeException>(() => Array.Clear(array, -4, 1));
        Throws<IndexOutOfRangeException>(() => Array.Clear(array, 0, 2));
        Throws<IndexOutOfRangeException>(() => Array.Clear(array, -3, -1));
        Throws<ArgumentNullException>(() => Array.Clear(null, 0, 0));

        Array empty = Array.CreateInstance(typeof(string), new int[] { 0 }, new int[] { 5 });
        Require(Array.IndexOf(empty, null) == 4, "Empty bounded array search");
        Array.Reverse(empty);
        ((IList)empty).Clear();

        Array low = Array.CreateInstance(typeof(int), new int[] { 2 }, new int[] { int.MinValue });
        low.SetValue(11, int.MinValue);
        Array.Reverse(low);
        Require((int)low.GetValue(int.MinValue + 1) == 11, "Reverse avoids overflow near minimum index");
        Array.Clear(low, int.MinValue, 2);
        Require((int)low.GetValue(int.MinValue + 1) == 0, "Clear minimum lower bound");
    }

    private static void TestCopy()
    {
        Array source = Array.CreateInstance(typeof(int), new int[] { 3 }, new int[] { -5 });
        source.SetValue(4, -5);
        source.SetValue(5, -4);
        source.SetValue(6, -3);
        Array destination = Array.CreateInstance(typeof(int), new int[] { 5 }, new int[] { 10 });

        Array.Copy(source, destination, 3);
        Require((int)destination.GetValue(10) == 4 && (int)destination.GetValue(12) == 6, "Copy starts at each lower bound");
        ((ICollection)source).CopyTo(destination, 12);
        Require((int)destination.GetValue(12) == 4 && (int)destination.GetValue(14) == 6, "ICollection.CopyTo uses absolute destination index");

        int[] vector = new int[4];
        source.CopyTo(vector, 1L);
        Require(vector[0] == 0 && vector[1] == 4 && vector[3] == 6, "Long CopyTo overload");
        Throws<ArgumentOutOfRangeException>(() => source.CopyTo(vector, (long)int.MaxValue + 1));
        Throws<ArgumentOutOfRangeException>(() => Array.Copy(source, -6, destination, 10, 0));
        Throws<ArgumentOutOfRangeException>(() => Array.Copy(source, -5, destination, 9, 0));
        Throws<ArgumentOutOfRangeException>(() => Array.Copy(source, destination, -1));
        Throws<ArgumentException>(() => Array.Copy(source, -2, destination, 10, 1));
        Throws<ArgumentException>(() => Array.Copy(source, -1, destination, 10, 0));
        Throws<ArgumentException>(() => source.CopyTo(vector, 2));
        Throws<ArgumentNullException>(() => source.CopyTo(null, 0));
        Array.Copy(source, -2, destination, 15, 0);

        Array.Copy(destination, 10, destination, 11, 4);
        Require((int)destination.GetValue(11) == 4 && (int)destination.GetValue(14) == 5, "Overlapping value copy backward");
        Array.Copy(destination, 11, destination, 10, 4);
        Require((int)destination.GetValue(10) == 4 && (int)destination.GetValue(13) == 5, "Overlapping value copy forward");

        string[] references = { "a", "b", "c", "d" };
        Array.Copy(references, 0, references, 1, 3);
        Require(references[1] == "a" && references[2] == "b" && references[3] == "c", "Overlapping reference copy");
        object[] mixed = { "first", new Item(9) };
        string[] narrowed = { "old", "untouched" };
        Throws<InvalidCastException>(() => ((ICollection)mixed).CopyTo(narrowed, 0));
        Require(narrowed[0] == "first" && narrowed[1] == "untouched", "Failed narrowing copy preserves successful prefix");
    }

    private static void TestMultidimensional()
    {
        Array source = Array.CreateInstance(typeof(int), new int[] { 2, 3 }, new int[] { -2, 5 });
        for (int row = -2; row < 0; row++)
        {
            for (int column = 5; column < 8; column++)
            {
                source.SetValue((row + 2) * 3 + column - 4, row, column);
            }
        }

        IList list = (IList)source;
        Require(list.Count == 6 && list.IsFixedSize && !list.IsReadOnly, "Multidimensional IList flags");
        Throws<ArgumentException>(() => { object ignored = list[0]; });
        Throws<ArgumentException>(() => list[0] = 1);
        Throws<RankException>(() => list.Contains(1));
        Throws<RankException>(() => list.IndexOf(1));
        Throws<RankException>(() => Array.IndexOf(source, 1));
        Throws<RankException>(() => Array.Reverse(source));
        Throws<RankException>(() => Array.Copy(source, new int[6], 0));
        Throws<RankException>(() => source.CopyTo(new int[6], 0));
        Throws<ArgumentException>(() => new int[6].CopyTo(source, 0));

        Array destination = Array.CreateInstance(typeof(int), new int[] { 3, 2 }, new int[] { 4, -8 });
        Array.Copy(source, destination, 6);
        Require((int)destination.GetValue(4, -8) == 1 && (int)destination.GetValue(6, -7) == 6, "Flat copy across different shapes of the same rank");
        Array.Clear(destination, 5, 4);
        Require((int)destination.GetValue(4, -8) == 1 && (int)destination.GetValue(4, -7) == 0 && (int)destination.GetValue(6, -7) == 6, "Flat clear across row boundaries");
        list.Clear();
        Require((int)source.GetValue(-2, 5) == 0 && (int)source.GetValue(-1, 7) == 0, "Multidimensional IList.Clear");
    }

    private static Array MakeClone()
    {
        Array source = Array.CreateInstance(typeof(Record), new int[] { 2, 2 }, new int[] { -4, 7 });
        source.SetValue(new Record { Item = new Item(41), Number = 0x1020304050607080L }, -3, 8);
        return (Array)((ICloneable)source).Clone();
    }

    private static void TestClone()
    {
        Item item = new Item(6);
        Item[] source = { item, null };
        Item[] clone = (Item[])source.Clone();
        Require(!object.ReferenceEquals(source, clone) && source.GetType() == clone.GetType(), "Clone type and independent array storage");
        Require(object.ReferenceEquals(source[0], clone[0]) && clone[1] == null, "Clone is shallow");
        clone[0].Value = 12;
        Require(source[0].Value == 12, "Cloned reference retains identity");
        source[0] = null;
        GC.Collect();
        Require(clone[0].Value == 12, "Clone references survive collection after original slot clears");

        Array bounded = MakeClone();
        GC.Collect();
        Require(bounded.Rank == 2 && bounded.GetLength(0) == 2 && bounded.GetLength(1) == 2, "Clone dimensions");
        Require(bounded.GetLowerBound(0) == -4 && bounded.GetLowerBound(1) == 7, "Clone lower bounds");
        Record record = (Record)bounded.GetValue(-3, 8);
        Require(record.Item.Value == 41 && record.Number == 0x1020304050607080L, "Clone struct fields and nested GC references");
        bounded.SetValue(null, -3, 8);
        record = (Record)bounded.GetValue(-3, 8);
        Require(record.Item == null && record.Number == 0, "Null SetValue resets all struct fields");
    }

    public static void Main()
    {
        TestInterfaces();
        TestBoundsAndSearch();
        TestCopy();
        TestMultidimensional();
        TestClone();
        Console.WriteLine("Array collection contracts passed.");
    }
}
