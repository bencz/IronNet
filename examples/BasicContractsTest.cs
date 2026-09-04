using System;
using System.Threading;

public static class BasicContractsTest
{
    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
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

        Require(caught != null && caught.GetType() == typeof(TException), "Expected exception: " + typeof(TException).FullName);
    }

    private static void TestStringsAndTypes()
    {
        Require("".Contains("") && "abc".Contains("") && "abc".Contains("abc"), "Empty or whole-string Contains failed.");
        Require("abcabc".Contains("bca") && !"abc".Contains("abcd") && !"abc".Contains("B"), "Contains did not use ordinal matching.");
        Require("a\0b".Contains("\0b") && "a\uD83D\uDE00z".Contains("\uD83D\uDE00"), "Contains truncated UTF-16 data.");
        Require(!"\u00E9".Contains("e\u0301") && !"I".Contains("\u0131"), "Contains applied cultural normalization.");
        Throws<ArgumentNullException>(() => "abc".Contains(null));

        Type integer = typeof(int);
        Type same = typeof(int);
        Type different = typeof(long);
        Type absent = null;
        Require(integer == same && integer.Equals(same) && integer.Equals((object)same), "Type equality lost identity.");
        Require(integer != different && integer != absent && absent != integer, "Type inequality failed.");
        Require(absent == (Type)null && !integer.Equals((Type)null), "Type null equality failed.");
        Exception exception = new InvalidOperationException("type");
        Require(exception.GetType() == typeof(InvalidOperationException), "Exception.GetType did not return the dynamic type.");
        Require(Environment.CurrentManagedThreadId == Thread.CurrentThread.ManagedThreadId, "Environment thread identity is inconsistent.");
    }

    private static void TestEventState()
    {
        using (EventWaitHandle manual = new ManualResetEvent(false))
        {
            Require(!manual.WaitOne(0), "Manual event started signaled.");
            Require(manual.Set() && manual.WaitOne(0) && manual.WaitOne(0), "Manual event did not remain signaled.");
            Require(manual.Reset() && !manual.WaitOne(0), "Manual reset failed.");
            Throws<ArgumentOutOfRangeException>(() => manual.WaitOne(-2));
        }

        using (EventWaitHandle automatic = new AutoResetEvent(true))
        {
            Require(automatic.WaitOne(0) && !automatic.WaitOne(0), "Auto event did not consume its initial signal.");
            automatic.Set();
            automatic.Set();
            Require(automatic.WaitOne(0) && !automatic.WaitOne(0), "Auto event counted duplicate pending signals.");
        }

        using (EventWaitHandle manual = new EventWaitHandle(true, EventResetMode.ManualReset))
        {
            Require(manual.WaitOne(0) && manual.WaitOne(0), "Base EventWaitHandle constructor failed.");
        }

        Throws<ArgumentException>(() => new EventWaitHandle(false, (EventResetMode)10));
        EventWaitHandle disposed = new EventWaitHandle(false, EventResetMode.AutoReset);
        disposed.Dispose();
        disposed.Dispose();
        Throws<ObjectDisposedException>(() => disposed.Set());
        Throws<ObjectDisposedException>(() => disposed.Reset());
        Throws<ObjectDisposedException>(() => disposed.WaitOne(0));
    }

    private static void TestEventWaiters(EventResetMode mode)
    {
        using (EventWaitHandle signal = new EventWaitHandle(false, mode))
        using (ManualResetEvent ready = new ManualResetEvent(false))
        using (ManualResetEvent firstFinished = new ManualResetEvent(false))
        using (ManualResetEvent allFinished = new ManualResetEvent(false))
        {
            int waiting = 0;
            int completed = 0;
            int timedOut = 0;
            ThreadStart wait = () =>
            {
                if (Interlocked.Increment(ref waiting) == 2)
                {
                    ready.Set();
                }
                if (!signal.WaitOne(5000))
                {
                    Interlocked.Increment(ref timedOut);
                }
                if (Interlocked.Increment(ref completed) == 2)
                {
                    allFinished.Set();
                }

                firstFinished.Set();
            };
            Thread first = new Thread(wait);
            Thread second = new Thread(wait);
            first.IsBackground = true;
            second.IsBackground = true;
            first.Start();
            second.Start();
            bool joinedFirst;
            bool joinedSecond;
            try
            {
                Require(ready.WaitOne(5000), "Event waiters failed to start.");
                signal.Set();
                Require(firstFinished.WaitOne(5000), "Event did not wake a waiter.");
                if (mode == EventResetMode.AutoReset)
                {
                    Require(!allFinished.WaitOne(50), "One auto-reset signal released both waiters.");
                    signal.Set();
                }

                Require(allFinished.WaitOne(5000), "Event did not release the remaining waiter.");
            }
            finally
            {
                signal.Set();
                joinedFirst = first.Join(6000);
                joinedSecond = second.Join(6000);
            }

            Require(joinedFirst && joinedSecond && timedOut == 0, "Event waiters timed out or failed to terminate.");
        }
    }

    public static int Main()
    {
        TestStringsAndTypes();
        TestEventState();
        TestEventWaiters(EventResetMode.ManualReset);
        TestEventWaiters(EventResetMode.AutoReset);
        Console.WriteLine("BasicContractsTest passed");
        return 0;
    }
}
