using System;
using System.Collections;
using System.Collections.Generic;

internal static class EnumeratorContractTest
{
    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static void RequireInvalidCurrent(IEnumerator enumerator)
    {
        try
        {
            object value = enumerator.Current;
            throw new Exception("Invalid IEnumerator.Current was accepted: " + value);
        }
        catch (InvalidOperationException)
        {
        }
    }

    private static void RequireFinished(IEnumerator enumerator)
    {
        while (enumerator.MoveNext())
        {
        }

        for (int i = 0; i < 5; i++)
        {
            Require(!enumerator.MoveNext(), "An exhausted enumerator restarted.");
        }

        RequireInvalidCurrent(enumerator);
    }

    private static void TestList()
    {
        List<int> list = new List<int>();
        list.Add(3);
        list.Add(7);
        List<int>.Enumerator enumerator = list.GetEnumerator();
        Require(enumerator.Current == 0, "List.Enumerator must expose default(T) before starting.");
        RequireInvalidCurrent(enumerator);
        Require(enumerator.MoveNext() && enumerator.Current == 3, "List enumeration failed.");
        List<int>.Enumerator copy = enumerator;
        Require(enumerator.MoveNext() && enumerator.Current == 7, "List enumeration did not advance.");
        Require(copy.Current == 3, "A struct enumerator copy shared its cursor.");
        RequireFinished(enumerator);

        IEnumerator<int> boxed = ((IEnumerable<int>)list).GetEnumerator();
        Require(boxed.MoveNext() && boxed.Current == 3, "Explicit IEnumerable<T> dispatch failed.");
        boxed.Reset();
        Require(boxed.MoveNext() && boxed.Current == 3, "Enumerator.Reset did not reset the cursor.");
        list.Add(11);
        bool rejected = false;
        try
        {
            boxed.MoveNext();
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }

        Require(rejected, "List enumeration accepted a mutation.");
    }

    private static void TestOtherCollections()
    {
        Queue<int> queue = new Queue<int>();
        queue.Enqueue(1);
        queue.Enqueue(2);
        Queue<int>.Enumerator queueEnumerator = queue.GetEnumerator();
        RequireInvalidCurrent(queueEnumerator);
        Require(queueEnumerator.MoveNext() && queueEnumerator.Current == 1, "Queue enumeration failed.");
        queueEnumerator.Dispose();
        RequireFinished(queueEnumerator);

        Stack<int> stack = new Stack<int>();
        stack.Push(1);
        stack.Push(2);
        Stack<int>.Enumerator stackEnumerator = stack.GetEnumerator();
        RequireInvalidCurrent(stackEnumerator);
        Require(stackEnumerator.MoveNext() && stackEnumerator.Current == 2, "Stack enumeration failed.");
        RequireFinished(stackEnumerator);
        stackEnumerator.Dispose();
        RequireFinished(stackEnumerator);

        HashSet<int> set = new HashSet<int>();
        set.Add(19);
        HashSet<int>.Enumerator setEnumerator = set.GetEnumerator();
        RequireInvalidCurrent(setEnumerator);
        Require(setEnumerator.MoveNext() && setEnumerator.Current == 19, "HashSet enumeration failed.");
        RequireFinished(setEnumerator);
    }

    public static void Main()
    {
        TestList();
        TestOtherCollections();
        Console.WriteLine("Enumerator contract test passed");
    }
}
