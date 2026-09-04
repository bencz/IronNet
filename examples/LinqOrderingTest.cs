using System;
using System.Collections.Generic;
using System.Linq;

public sealed class OrderingRecord
{
    public int Group;
    public int Score;
    public string Name;

    public OrderingRecord(int group, int score, string name)
    {
        Group = group;
        Score = score;
        Name = name;
    }
}

public static class LinqOrderingTest
{
    private static int _selectorCalls;

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static IEnumerable<int> Expand(int value)
    {
        _selectorCalls++;
        return new int[] { value, value * 10 };
    }

    private static int Descending(int left, int right)
    {
        return right.CompareTo(left);
    }

    public static int Main()
    {
        Require(Comparer<int>.Default.Compare(2, 7) < 0, "Default integer comparer failed.");
        Require(Comparer<string>.Default.Compare(null, "a") < 0, "Default comparer null ordering failed.");
        Require(Comparer<double>.Default.Compare(double.NaN, double.NaN) == 0, "NaN equality ordering failed.");
        Require(Comparer<double>.Default.Compare(double.NaN, 0.0) < 0, "NaN ordering failed.");
        Require(((IComparable)12).CompareTo(3) > 0, "Nongeneric primitive comparison failed.");
        Require(((IComparable<string>)"alpha").CompareTo("beta") < 0, "Generic string comparison failed.");

        Comparer<int> descendingComparer = Comparer<int>.Create(Descending);
        Require(descendingComparer.Compare(2, 7) > 0, "Delegate-backed comparer failed.");

        OrderingRecord[] records = new OrderingRecord[]
        {
            new OrderingRecord(2, 1, "a"),
            new OrderingRecord(1, 2, "b"),
            new OrderingRecord(1, 2, "c"),
            new OrderingRecord(1, 1, "d")
        };

        OrderingRecord[] ordered = records.OrderBy(record => record.Group).ThenByDescending(record => record.Score).ToArray();
        Require(ordered.Length == 4, "Ordered sequence length failed.");
        Require(ordered[0].Name == "b" && ordered[1].Name == "c" && ordered[2].Name == "d" && ordered[3].Name == "a", "Compound stable ordering failed.");

        int[] customOrdered = new int[] { 4, 1, 3, 2 }.OrderBy(value => value, descendingComparer).ToArray();
        Require(customOrdered.SequenceEqual(new int[] { 4, 3, 2, 1 }), "Custom comparer ordering failed.");

        IEnumerable<int> expandedSequence = new int[] { 1, 2, 3 }.SelectMany(Expand);
        Require(_selectorCalls == 0, "SelectMany executed eagerly.");
        int[] expanded = expandedSequence.ToArray();
        Require(_selectorCalls == 3, "SelectMany selector invocation count failed.");
        Require(expanded.SequenceEqual(new int[] { 1, 10, 2, 20, 3, 30 }), "SelectMany flattening failed.");

        string[] projected = records.Take(2).SelectMany(record => new int[] { record.Score, record.Group }, (record, value) => record.Name + value).ToArray();
        Require(EqualityComparer<string>.Default.Equals("a1", "a1"), "String comparer failed.");
        Require(projected.SequenceEqual(new string[] { "a1", "a2", "b2", "b1" }), "SelectMany result selector failed.");

        int[] skipped = new int[] { 1, 2, 3, 1 }.SkipWhile(value => value < 3).ToArray();
        Require(skipped.SequenceEqual(new int[] { 3, 1 }), "SkipWhile failed.");

        int[] taken = new int[] { 1, 2, 3, 1 }.TakeWhile(value => value < 3).ToArray();
        Require(taken.SequenceEqual(new int[] { 1, 2 }), "TakeWhile failed.");

        int[] zipped = new int[] { 1, 2, 3 }.Zip(new int[] { 10, 20 }, (left, right) => left + right).ToArray();
        Require(zipped.SequenceEqual(new int[] { 11, 22 }), "Zip failed or did not stop at the shorter sequence.");

        Console.WriteLine("LINQ ordering test passed");
        return 0;
    }
}
