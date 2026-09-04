using System;

public static class NestedFinallyTest
{
    private static int _order;

    private static void Record(int value)
    {
        _order = _order * 10 + value;
    }

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static int ReturnThroughNestedFinally()
    {
        try
        {
            Record(1);
            return 42;
        }
        finally
        {
            Record(2);
            try
            {
                Record(3);
            }
            finally
            {
                Record(4);
                try
                {
                    Record(5);
                }
                finally
                {
                    Record(6);
                }
                Record(7);
            }
            Record(8);
        }
    }

    private static void TestNormalCompletion()
    {
        _order = 0;
        Require(ReturnThroughNestedFinally() == 42 && _order == 12345678, "Nested finally lost a return continuation.");

        _order = 0;
        try
        {
            Record(1);
        }
        finally
        {
            for (int i = 0; i < 2; i++)
            {
                try
                {
                    Record(2);
                    if (i == 0)
                    {
                        continue;
                    }
                    break;
                }
                finally
                {
                    Record(3);
                }
            }
            Record(4);
        }

        Require(_order == 123234, "Loop leave overwrote the enclosing finally continuation.");
    }

    private static void ThrowThroughNestedFinally()
    {
        try
        {
            // No managed local retains this exception during the nested handlers.
            throw new InvalidOperationException("original");
        }
        finally
        {
            Record(1);
            try
            {
                try
                {
                    throw new ArgumentException("handled-inside-finally");
                }
                catch (ArgumentException)
                {
                    Record(2);
                }
            }
            finally
            {
                GC.Collect();
                Record(3);
            }
            Record(4);
        }
    }

    private static void TestExceptionalCompletion()
    {
        _order = 0;
        Exception caught = null;
        try
        {
            ThrowThroughNestedFinally();
        }
        catch (InvalidOperationException exception)
        {
            caught = exception;
            Record(5);
        }

        Require(caught != null && caught.Message == "original" && _order == 12345, "Nested handlers lost the suspended exception or its GC root.");

        _order = 0;
        caught = null;
        try
        {
            try
            {
                throw new InvalidOperationException("replaced");
            }
            finally
            {
                try
                {
                    throw new ArgumentException("replacement");
                }
                finally
                {
                    try
                    {
                        Record(1);
                    }
                    finally
                    {
                        GC.Collect();
                        Record(2);
                    }
                }
            }
        }
        catch (Exception exception)
        {
            caught = exception;
            Record(3);
        }

        Require(caught is ArgumentException && caught.Message == "replacement" && _order == 123, "An escaping finally exception did not replace the original exception.");
    }

    private static void TestNestedCatchInFinally()
    {
        _order = 0;
        try
        {
            Record(1);
        }
        finally
        {
            try
            {
                try
                {
                    Record(2);
                }
                finally
                {
                    throw new ArgumentException("inner-finally");
                }
            }
            catch (ArgumentException)
            {
                Record(3);
            }
            Record(4);
        }
        Record(5);

        Require(_order == 12345, "Handling an inner finally exception discarded an enclosing leave.");
    }

    public static int Main()
    {
        TestNormalCompletion();
        TestExceptionalCompletion();
        TestNestedCatchInFinally();
        Console.WriteLine("NestedFinallyTest passed");
        return 0;
    }
}
