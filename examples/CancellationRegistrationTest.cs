using System;
using System.Threading;

public static class CancellationRegistrationTest
{
    private const int Timeout = 5000;

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static void TestPendingRemoval()
    {
        using (CancellationTokenSource source = new CancellationTokenSource())
        {
            int order = 0;
            CancellationTokenRegistration first = source.Token.Register(() => order = order * 10 + 1);
            CancellationTokenRegistration removed = source.Token.Register(() => order = order * 10 + 2);
            CancellationTokenRegistration last = source.Token.Register(() =>
            {
                order = order * 10 + 3;
                removed.Dispose();
            });

            source.Cancel();
            source.Cancel();
            Require(order == 31, "A removed callback ran, or cancellation did not preserve LIFO order.");
            first.Dispose();
            removed.Dispose();
            last.Dispose();
        }
    }

    private static void TestIdentityAndSelfDisposal()
    {
        using (CancellationTokenSource source = new CancellationTokenSource())
        {
            int calls = 0;
            Action callback = () => calls++;
            CancellationTokenRegistration first = source.Token.Register(callback);
            CancellationTokenRegistration copy = first;
            CancellationTokenRegistration second = source.Token.Register(callback);
            Require(first == copy && first.Equals((object)copy), "Copies lost their registration identity.");
            Require(first.GetHashCode() == copy.GetHashCode() && first != second, "Distinct registrations share identity.");
            copy.Dispose();
            first.Dispose();
            Require(first == copy, "Disposal changed registration identity.");

            CancellationTokenRegistration self = default(CancellationTokenRegistration);
            self = source.Token.Register(() =>
            {
                self.Dispose();
                source.Cancel();
                calls += 10;
            });

            source.Cancel();
            Require(calls == 11, "Self disposal or recursive cancellation failed.");
            self.Dispose();
            second.Dispose();
        }

        default(CancellationTokenRegistration).Dispose();
    }

    private static void TestConcurrentPendingRemoval()
    {
        using (CancellationTokenSource source = new CancellationTokenSource())
        using (ManualResetEvent entered = new ManualResetEvent(false))
        using (ManualResetEvent release = new ManualResetEvent(false))
        {
            int pendingCalls = 0;
            Exception failure = null;
            CancellationTokenRegistration pending = source.Token.Register(() => pendingCalls++);
            source.Token.Register(() =>
            {
                entered.Set();
                Require(release.WaitOne(Timeout), "Pending removal release timed out.");
            });
            Thread canceler = new Thread(() =>
            {
                try
                {
                    source.Cancel();
                }
                catch (Exception exception)
                {
                    failure = exception;
                }
            });
            canceler.IsBackground = true;
            canceler.Start();
            bool joined;
            try
            {
                Require(entered.WaitOne(Timeout), "Concurrent cancellation did not start.");
                pending.Dispose();
                source.Cancel();
                bool inline = false;
                source.Token.Register(() => inline = true);
                Require(inline, "Registration waited for an already running cancellation to finish.");
            }
            finally
            {
                release.Set();
                joined = canceler.Join(Timeout);
            }

            Require(joined && failure == null && pendingCalls == 0, "Concurrent disposal did not suppress the pending callback.");
        }
    }

    private static void TestExecutingDisposal(bool throwFromCallback, bool throwOnFirstException)
    {
        using (CancellationTokenSource source = new CancellationTokenSource())
        using (ManualResetEvent entered = new ManualResetEvent(false))
        using (ManualResetEvent release = new ManualResetEvent(false))
        using (ManualResetEvent disposing = new ManualResetEvent(false))
        using (ManualResetEvent secondDisposing = new ManualResetEvent(false))
        using (ManualResetEvent disposed = new ManualResetEvent(false))
        using (ManualResetEvent secondDisposed = new ManualResetEvent(false))
        {
            bool completed = false;
            bool returnedEarly = false;
            bool secondReturnedEarly = false;
            Exception cancellationFailure = null;
            CancellationTokenRegistration registration = source.Token.Register(() =>
            {
                entered.Set();
                Require(release.WaitOne(Timeout), "Callback release timed out.");
                completed = true;
                if (throwFromCallback)
                {
                    throw new InvalidOperationException("callback-failure");
                }
            });
            CancellationTokenRegistration copy = registration;

            Thread canceler = new Thread(() =>
            {
                try
                {
                    source.Cancel(throwOnFirstException);
                }
                catch (Exception exception)
                {
                    cancellationFailure = exception;
                }
            });
            Thread disposer = new Thread(() =>
            {
                disposing.Set();
                registration.Dispose();
                returnedEarly = !completed;
                disposed.Set();
            });
            Thread secondDisposer = new Thread(() =>
            {
                secondDisposing.Set();
                copy.Dispose();
                secondReturnedEarly = !completed;
                secondDisposed.Set();
            });
            canceler.IsBackground = true;
            disposer.IsBackground = true;
            secondDisposer.IsBackground = true;

            canceler.Start();
            Require(entered.WaitOne(Timeout), "Cancellation callback did not start.");
            disposer.Start();
            secondDisposer.Start();
            bool joinedCanceler;
            bool joinedDisposer;
            bool joinedSecondDisposer;
            try
            {
                Require(disposing.WaitOne(Timeout) && secondDisposing.WaitOne(Timeout), "Disposer threads did not start.");
                Require(!disposed.WaitOne(100) && !secondDisposed.WaitOne(100), "Dispose returned while its callback was still blocked.");
            }
            finally
            {
                release.Set();
                joinedCanceler = canceler.Join(Timeout);
                joinedDisposer = disposer.Join(Timeout);
                joinedSecondDisposer = secondDisposer.Join(Timeout);
            }

            Require(joinedCanceler && joinedDisposer && joinedSecondDisposer, "Cancellation or disposal deadlocked.");
            Require(!returnedEarly && !secondReturnedEarly, "Dispose did not wait for callback completion.");
            Require(disposed.WaitOne(0) && secondDisposed.WaitOne(0), "Callback completion did not wake all disposers.");
            if (throwFromCallback)
            {
                if (throwOnFirstException)
                {
                    Require(cancellationFailure is InvalidOperationException && cancellationFailure.Message == "callback-failure", "Cancel(true) wrapped the callback exception.");
                }
                else
                {
                    AggregateException aggregate = cancellationFailure as AggregateException;
                    Require(aggregate != null && aggregate.InnerExceptions[0].Message == "callback-failure", "Callback exception was not propagated by Cancel.");
                }
            }
            else
            {
                Require(cancellationFailure == null, "Cancellation unexpectedly failed.");
            }
        }
    }

    private static void TestExceptionPolicy()
    {
        using (CancellationTokenSource source = new CancellationTokenSource())
        {
            int calls = 0;
            source.Token.Register(() =>
            {
                calls++;
                throw new ArgumentException("first");
            });
            source.Token.Register(() =>
            {
                calls++;
                throw new InvalidOperationException("last");
            });

            AggregateException failure = null;
            try
            {
                source.Cancel(false);
            }
            catch (AggregateException exception)
            {
                failure = exception;
            }

            Require(calls == 2 && failure != null, "Cancel(false) did not execute all callbacks.");
            Require(failure.InnerExceptions[0].Message == "last" && failure.InnerExceptions[1].Message == "first", "Exception order differs from callback order.");
        }

        using (CancellationTokenSource source = new CancellationTokenSource())
        {
            bool invoked = false;
            CancellationTokenRegistration skipped = source.Token.Register(() => invoked = true);
            InvalidOperationException expected = new InvalidOperationException("stop");
            CancellationTokenRegistration throwing = source.Token.Register(() =>
            {
                throw expected;
            });
            Exception failure = null;
            try
            {
                source.Cancel(true);
            }
            catch (Exception exception)
            {
                failure = exception;
            }

            Require(object.ReferenceEquals(expected, failure) && !invoked, "Cancel(true) did not stop at the first exception.");
            throwing.Dispose();
            skipped.Dispose();
            source.Cancel();
            Require(!invoked, "A later Cancel resumed callbacks abandoned by Cancel(true).");
        }
    }

    private static void TestImmediateRegistration()
    {
        using (CancellationTokenSource source = new CancellationTokenSource())
        {
            source.Cancel();
            Thread caller = Thread.CurrentThread;
            bool inline = false;
            CancellationTokenRegistration registration = source.Token.Register(state =>
            {
                inline = object.ReferenceEquals(caller, Thread.CurrentThread) && (int)state == 42;
            }, 42);
            Require(inline && registration == default(CancellationTokenRegistration), "Registration after cancellation was not invoked inline.");

            bool propagated = false;
            try
            {
                source.Token.Register(() =>
                {
                    throw new InvalidOperationException("inline");
                });
            }
            catch (InvalidOperationException exception)
            {
                propagated = exception.Message == "inline";
            }

            Require(propagated, "Immediate registration did not propagate the callback exception directly.");
        }
    }

    private static void TestLinkedDisposal()
    {
        using (CancellationTokenSource parent = new CancellationTokenSource())
        using (CancellationTokenSource other = new CancellationTokenSource())
        using (CancellationTokenSource linked = CancellationTokenSource.CreateLinkedTokenSource(parent.Token, other.Token))
        {
            CancellationToken linkedToken = linked.Token;
            int calls = 0;
            linkedToken.Register(() => calls++);
            parent.Token.Register(() => linked.Dispose());
            parent.Cancel();
            other.Cancel();
            Require(!linkedToken.IsCancellationRequested && calls == 0, "Disposed linked source was canceled by a pending parent callback.");
        }

        using (CancellationTokenSource parent = new CancellationTokenSource())
        using (CancellationTokenSource linked = CancellationTokenSource.CreateLinkedTokenSource(parent.Token, CancellationToken.None))
        {
            int calls = 0;
            linked.Token.Register(() =>
            {
                calls++;
                linked.Dispose();
            });
            parent.Cancel();
            Require(calls == 1, "Disposing a linked source from its own callback failed.");
        }
    }

    private static void TestRegistrationRaces()
    {
        for (int iteration = 0; iteration < 32; iteration++)
        {
            using (CancellationTokenSource source = new CancellationTokenSource())
            using (ManualResetEvent start = new ManualResetEvent(false))
            {
                int calls = 0;
                Exception failure = null;
                CancellationTokenRegistration registration = default(CancellationTokenRegistration);
                Thread registrar = new Thread(() =>
                {
                    try
                    {
                        Require(start.WaitOne(Timeout), "Registration race start timed out.");
                        registration = source.Token.Register(() => Interlocked.Increment(ref calls));
                    }
                    catch (Exception exception)
                    {
                        failure = exception;
                    }
                });
                registrar.IsBackground = true;
                registrar.Start();
                start.Set();
                source.Cancel();
                Require(registrar.Join(Timeout), "Concurrent registration deadlocked.");
                registration.Dispose();
                source.Cancel();
                Require(failure == null && calls == 1, "Register racing Cancel lost or duplicated its callback.");
            }
        }
    }

    private static void TestLinkedDisposalRaces()
    {
        for (int iteration = 0; iteration < 32; iteration++)
        {
            using (CancellationTokenSource parent = new CancellationTokenSource())
            using (CancellationTokenSource linked = CancellationTokenSource.CreateLinkedTokenSource(parent.Token, CancellationToken.None))
            using (ManualResetEvent start = new ManualResetEvent(false))
            {
                Exception failure = null;
                Thread canceler = new Thread(() =>
                {
                    try
                    {
                        Require(start.WaitOne(Timeout), "Linked cancellation race start timed out.");
                        parent.Cancel();
                    }
                    catch (Exception exception)
                    {
                        failure = exception;
                    }
                });
                canceler.IsBackground = true;
                canceler.Start();
                start.Set();
                linked.Dispose();
                Require(canceler.Join(Timeout), "Disposal racing linked cancellation deadlocked.");
                Require(failure == null, "Linked disposal made parent cancellation throw.");
            }
        }
    }

    public static int Main()
    {
        TestPendingRemoval();
        Console.WriteLine("CancellationRegistrationTest: pending removal passed");
        TestIdentityAndSelfDisposal();
        Console.WriteLine("CancellationRegistrationTest: identity and self disposal passed");
        TestConcurrentPendingRemoval();
        TestExecutingDisposal(false, false);
        TestExecutingDisposal(true, false);
        TestExecutingDisposal(true, true);
        Console.WriteLine("CancellationRegistrationTest: executing disposal passed");
        TestExceptionPolicy();
        Console.WriteLine("CancellationRegistrationTest: exception policy passed");
        TestImmediateRegistration();
        TestLinkedDisposal();
        TestRegistrationRaces();
        TestLinkedDisposalRaces();
        Console.WriteLine("CancellationRegistrationTest passed");
        return 0;
    }
}
