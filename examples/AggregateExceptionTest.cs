using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

public static class AggregateExceptionTest
{
    private sealed class CustomAggregate : AggregateException
    {
        public CustomAggregate(Exception exception) : base("base", exception)
        {
        }

        public override string Message => "custom";
    }

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static TException Throws<TException>(Action action) where TException : Exception
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
        return (TException)caught;
    }

    private static IEnumerable<Exception> Enumerate(Exception[] values, Action disposed, bool fail)
    {
        try
        {
            foreach (Exception value in values)
            {
                yield return value;
            }
            if (fail)
            {
                throw new InvalidOperationException("enumeration-failed");
            }
        }
        finally
        {
            disposed();
        }
    }

    private static void TestConstruction()
    {
        Exception first = new InvalidOperationException("first");
        Exception second = new ArgumentException("second");
        Exception[] values = new Exception[] { first, second };
        AggregateException aggregate = new AggregateException("root", values);
        ReadOnlyCollection<Exception> view = aggregate.InnerExceptions;
        Require(object.ReferenceEquals(view, aggregate.InnerExceptions), "InnerExceptions reallocates its view.");
        Require(view.Count == 2 && object.ReferenceEquals(aggregate.InnerException, first), "Constructor lost its first inner exception.");
        values[0] = null;
        Require(object.ReferenceEquals(view[0], first), "Constructor did not snapshot the supplied array.");
        Throws<NotSupportedException>(() => ((IList<Exception>)view)[0] = second);
        Require(aggregate.Message == "root (first) (second)", "Aggregate message did not include each inner message.");

        AggregateException empty = new AggregateException();
        Require(empty.InnerException == null && empty.InnerExceptions.Count == 0 && empty.Message == "One or more errors occurred.", "Default aggregate is not empty.");
        Require(new AggregateException("custom").Message == "custom", "Message-only constructor failed.");
        Require(new AggregateException((string)null).Message.Contains("System.AggregateException"), "Null message did not use the exception type.");
        Require(new AggregateException("single", first).InnerExceptions.Count == 1, "Single exception constructor failed.");
        Require(new AggregateException(first, second).InnerExceptions.Count == 2, "Params constructor failed.");

        int disposed = 0;
        AggregateException enumerated = new AggregateException("sequence", Enumerate(new Exception[] { first, second }, () => disposed++, false));
        Require(disposed == 1 && enumerated.InnerExceptions.Count == 2, "Enumerable constructor did not dispose its enumerator.");
        AggregateException enumerableDefault = new AggregateException((IEnumerable<Exception>)new Exception[] { first });
        Require(object.ReferenceEquals(enumerableDefault.InnerException, first), "Enumerable-only constructor failed.");
        Throws<InvalidOperationException>(() => new AggregateException(Enumerate(new Exception[] { first }, () => disposed++, true)));
        Require(disposed == 2, "Failing enumeration was not disposed.");
        Throws<ArgumentNullException>(() => new AggregateException((Exception[])null));
        Throws<ArgumentNullException>(() => new AggregateException((IEnumerable<Exception>)null));
        Throws<ArgumentNullException>(() => new AggregateException("single", (Exception)null));
        Throws<ArgumentNullException>(() => new AggregateException("array", (Exception[])null));
        Throws<ArgumentNullException>(() => new AggregateException("sequence", (IEnumerable<Exception>)null));
        Throws<ArgumentException>(() => new AggregateException(new Exception[] { first, null }));
    }

    private static void TestFlattenAndBaseException()
    {
        Exception first = new Exception("first");
        Exception second = new Exception("second");
        Exception third = new Exception("third");
        AggregateException nested = new AggregateException("nested", first, new AggregateException(second));
        AggregateException aggregate = new AggregateException("root", nested, third, first);
        AggregateException flattened = aggregate.Flatten();
        Require(flattened.InnerExceptions.Count == 4, "Flatten lost duplicate leaves.");
        Require(object.ReferenceEquals(flattened.InnerExceptions[0], third) && object.ReferenceEquals(flattened.InnerExceptions[1], first), "Flatten did not emit direct leaves first.");
        Require(object.ReferenceEquals(flattened.InnerExceptions[2], first) && object.ReferenceEquals(flattened.InnerExceptions[3], second), "Flatten did not preserve breadth-first order.");
        Require(flattened.Message == "root (third) (first) (first) (second)", "Flatten duplicated nested aggregate messages.");
        Require(flattened.Flatten().Message == flattened.Message, "Repeated Flatten changed the message.");
        Require(object.ReferenceEquals(aggregate.GetBaseException(), aggregate), "Multiple inner exceptions should stop GetBaseException.");

        AggregateException empty = new AggregateException("empty");
        Require(object.ReferenceEquals(empty.GetBaseException(), empty) && empty.Flatten().InnerExceptions.Count == 0, "Empty aggregate behavior is incorrect.");
        Exception ordinary = new Exception("outer", first);
        AggregateException chain = new AggregateException(new AggregateException(ordinary));
        Require(object.ReferenceEquals(chain.GetBaseException(), ordinary), "Aggregate GetBaseException traversed a non-aggregate exception.");
        Require(object.ReferenceEquals(ordinary.GetBaseException(), first), "Exception.GetBaseException did not traverse inner exceptions.");
        Require(new CustomAggregate(first).Flatten().Message == "custom (first)", "Flatten lost an overridden message.");

        AggregateException deep = new AggregateException(first);
        for (int i = 0; i < 512; i++)
        {
            deep = new AggregateException(deep);
        }

        Require(object.ReferenceEquals(deep.GetBaseException(), first), "Deep GetBaseException failed.");
        Require(object.ReferenceEquals(deep.Flatten().InnerExceptions[0], first), "Iterative Flatten failed on a deep exception tree.");
    }

    private static void TestHandleAndText()
    {
        Exception first = new Exception("first");
        Exception second = new Exception("second");
        AggregateException nested = new AggregateException("nested", second);
        AggregateException aggregate = new AggregateException("root", first, nested, second);
        int calls = 0;
        aggregate.Handle(exception =>
        {
            calls++;
            return true;
        });
        Require(calls == 3, "Handle did not visit each direct inner exception.");

        AggregateException unhandled = Throws<AggregateException>(() => aggregate.Handle(exception => object.ReferenceEquals(exception, first)));
        Require(unhandled.InnerExceptions.Count == 2 && object.ReferenceEquals(unhandled.InnerExceptions[0], nested), "Handle flattened or reordered unhandled exceptions.");
        Require(object.ReferenceEquals(unhandled.InnerExceptions[1], second), "Handle lost an unhandled exception.");
        Require(unhandled.Message == aggregate.Message + " (" + nested.Message + ") (second)", "Handle did not preserve the original message.");
        Throws<ArgumentNullException>(() => aggregate.Handle(null));

        InvalidOperationException failure = new InvalidOperationException("predicate-failed");
        calls = 0;
        InvalidOperationException caught = Throws<InvalidOperationException>(() => aggregate.Handle(exception =>
        {
            calls++;
            throw failure;
        }));
        Require(object.ReferenceEquals(caught, failure) && calls == 1, "Handle wrapped the predicate failure or continued executing.");
        new AggregateException().Handle(exception =>
        {
            throw new Exception("Empty aggregate invoked its handler.");
        });

        string text = aggregate.ToString();
        Require(text.Contains("System.AggregateException") && text.Contains("Inner Exception #1") && text.Contains("Inner Exception #2"), "ToString omitted secondary exceptions.");
        Require(text.Contains(first.ToString()) && text.Contains(second.ToString()), "ToString omitted an inner exception's text.");
    }

    public static int Main()
    {
        TestConstruction();
        TestFlattenAndBaseException();
        TestHandleAndText();
        Console.WriteLine("AggregateExceptionTest passed");
        return 0;
    }
}
