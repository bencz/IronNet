using System;
using System.Collections.Generic;
using System.Linq;

internal sealed class CollisionComparer : IEqualityComparer<int>
{
    public bool Equals(int left, int right)
    {
        return left == right;
    }

    public int GetHashCode(int value)
    {
        return value & 3;
    }
}

internal static class CollectionsCoreTest
{
    private static int _assertions;

    private static void Main()
    {
        TestList();
        TestDictionary();
        TestHashSet();
        TestQueue();
        TestStack();
        TestLinq();

        Console.Write("Collections assertions passed: ");
        Console.WriteLine(_assertions);
    }

    private static void TestList()
    {
        List<int> values = new List<int>(16);
        values.Add(1);
        values.Add(2);
        values.Add(3);
        values.Reverse();
        AssertSequence(values, new int[] { 3, 2, 1 }, "List.Reverse must ignore unused capacity");

        values.InsertRange(1, values);
        AssertSequence(values, new int[] { 3, 3, 2, 1, 2, 1 }, "List.InsertRange self insertion");
        Assert(values.RemoveAll(value => (value & 1) == 0) == 2, "List.RemoveAll count");
        AssertSequence(values, new int[] { 3, 3, 1, 1 }, "List.RemoveAll contents");

        IEnumerator<int> enumerator = values.GetEnumerator();
        Assert(enumerator.MoveNext(), "List enumerator first item");
        values.Add(9);
        AssertThrowsInvalidOperation(() => enumerator.MoveNext(), "List enumerator detects mutation");
    }

    private static void TestDictionary()
    {
        Dictionary<int, int> values = new Dictionary<int, int>(new CollisionComparer());
        for (int i = 0; i < 80; i++)
        {
            values.Add(i, i * 10);
        }

        for (int i = 0; i < 80; i += 2)
        {
            Assert(values.Remove(i), "Dictionary removes existing key");
        }

        for (int i = 80; i < 120; i++)
        {
            values.Add(i, i * 10);
        }

        Assert(values.Count == 80, "Dictionary count after free-list reuse");
        Assert(values[119] == 1190, "Dictionary value after resize and collisions");

        IEnumerator<KeyValuePair<int, int>> enumerator = values.GetEnumerator();
        Assert(enumerator.MoveNext(), "Dictionary enumerator first item");
        values[119] = 7;
        AssertThrowsInvalidOperation(() => enumerator.MoveNext(), "Dictionary enumerator detects value replacement");
    }

    private static void TestHashSet()
    {
        CollisionComparer comparer = new CollisionComparer();
        HashSet<int> values = new HashSet<int>(comparer);
        for (int i = 0; i < 100; i++)
        {
            Assert(values.Add(i), "HashSet adds unique item");
            Assert(!values.Add(i), "HashSet rejects duplicate item");
        }

        for (int i = 0; i < 50; i++)
        {
            Assert(values.Remove(i), "HashSet removes existing item");
        }

        Assert(values.Contains(50), "HashSet contains retained item");
        Assert(!values.Add(50), "HashSet rejects retained duplicate");
        int[] additions = new int[] { 25, 50, 100, 101 };
        IEnumerable<int> additionSequence = additions;
        values.UnionWith(additionSequence);
        Assert(values.Count == 53, "HashSet union count");
        values.IntersectWith(Enumerable.Range(40, 70));
        Assert(values.Count == 52, "HashSet intersection count");
        values.ExceptWith(new int[] { 50, 51, 52 });
        Assert(values.Count == 49, "HashSet except count");
        values.SymmetricExceptWith(new int[] { 52, 53, 110 });
        Assert(values.Contains(52) && !values.Contains(53) && values.Contains(110), "HashSet symmetric difference");
        Assert(values.IsSupersetOf(new int[] { 52, 54, 110 }), "HashSet superset");
        Assert(values.Overlaps(new int[] { -1, 110 }), "HashSet overlap");
        values.TrimExcess();
        Assert(values.Contains(99), "HashSet TrimExcess preserves items");
    }

    private static void TestQueue()
    {
        Queue<int> queue = new Queue<int>(4);
        queue.Enqueue(1);
        queue.Enqueue(2);
        queue.Enqueue(3);
        queue.Enqueue(4);
        Assert(queue.Dequeue() == 1, "Queue dequeue first");
        Assert(queue.Dequeue() == 2, "Queue dequeue second");
        queue.Enqueue(5);
        queue.Enqueue(6);
        queue.Enqueue(7);
        AssertSequence(queue, new int[] { 3, 4, 5, 6, 7 }, "Queue wrap and resize ordering");

        int item;
        Assert(queue.TryPeek(out item) && item == 3, "Queue.TryPeek");
        Assert(queue.TryDequeue(out item) && item == 3, "Queue.TryDequeue");
    }

    private static void TestStack()
    {
        Stack<int> stack = new Stack<int>();
        for (int i = 1; i <= 8; i++)
        {
            stack.Push(i);
        }

        AssertSequence(stack, new int[] { 8, 7, 6, 5, 4, 3, 2, 1 }, "Stack enumerates from top");
        Assert(stack.Pop() == 8, "Stack.Pop");

        int item;
        Assert(stack.TryPeek(out item) && item == 7, "Stack.TryPeek");
        stack.TrimExcess();
        Assert(stack.Count == 7, "Stack.TrimExcess preserves count");
    }

    private static void TestLinq()
    {
        int[] pipeline = Enumerable.Range(1, 12)
            .Where(value => (value & 1) == 0)
            .Skip(1)
            .Take(4)
            .Select(value => value * 3)
            .Reverse()
            .ToArray();
        AssertSequence(pipeline, new int[] { 30, 24, 18, 12 }, "LINQ pipeline");

        int[] distinct = new int[] { 1, 1, 2, 3, 2, 4 }.Distinct().ToArray();
        AssertSequence(distinct, new int[] { 1, 2, 3, 4 }, "LINQ Distinct ordering");
        AssertSequence(new int[] { 1, 2, 2 }.Union(new int[] { 2, 3 }), new int[] { 1, 2, 3 }, "LINQ Union");
        AssertSequence(new int[] { 1, 2, 2, 3 }.Intersect(new int[] { 2, 3, 4 }), new int[] { 2, 3 }, "LINQ Intersect");
        AssertSequence(new int[] { 1, 2, 2, 3 }.Except(new int[] { 2 }), new int[] { 1, 3 }, "LINQ Except");

        Assert(Enumerable.Range(3, 4).SequenceEqual(new int[] { 3, 4, 5, 6 }), "LINQ SequenceEqual");
        Assert(Enumerable.Range(1, 5).Aggregate(0, (sum, value) => sum + value) == 15, "LINQ Aggregate");
        Assert(Enumerable.Range(1, 5).Min() == 1, "LINQ Min");
        Assert(Enumerable.Range(1, 5).Max() == 5, "LINQ Max");
        Assert(Enumerable.Range(1, 4).Average() == 2.5, "LINQ Average");
        Assert(Enumerable.Repeat(7, 3).All(value => value == 7), "LINQ Repeat");
        Assert(!Enumerable.Empty<int>().Any(), "LINQ Empty");

        Dictionary<int, int> dictionary = Enumerable.Range(1, 5).ToDictionary(value => value, value => value * value);
        Assert(dictionary.Count == 5 && dictionary[4] == 16, "LINQ ToDictionary");
    }

    private static void AssertSequence(IEnumerable<int> actual, int[] expected, string message)
    {
        Assert(actual.SequenceEqual(expected), message);
    }

    private static void AssertThrowsInvalidOperation(Action action, string message)
    {
        try
        {
            action();
        }
        catch (InvalidOperationException)
        {
            _assertions++;
            return;
        }

        throw new Exception(message);
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            Console.Write("Assertion failed: ");
            Console.WriteLine(message);
            throw new Exception(message);
        }

        _assertions++;
    }
}
