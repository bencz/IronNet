using System;
using System.Threading;
using System.Threading.Tasks;

public static class TaskTest
{
    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static async Task<int> ComputeAsync(int value)
    {
        await Task.Yield();
        await Task.Delay(1);
        int doubled = await Task.Run(() => value * 2);
        return doubled + 1;
    }

    private static async Task<bool> CatchAwaitedFailureAsync()
    {
        try
        {
            await Task.FromException(new InvalidOperationException("awaited-failure"));
            return false;
        }
        catch (InvalidOperationException exception)
        {
            return exception.Message == "awaited-failure";
        }
    }

    private static void TestTaskExecution()
    {
        Task<int> task = Task.Run(() =>
        {
            Console.WriteLine("TaskTest: worker running");
            return 21 * 2;
        });
        Console.WriteLine("TaskTest: worker started");
        Require(task.Result == 42, "Task<TResult>.Result returned the wrong value.");
        Console.WriteLine("TaskTest: worker completed");
        Require(task.Status == TaskStatus.RanToCompletion, "Successful task has an incorrect status.");

        int continuationValue = 0;
        Task continuation = task.ContinueWith(completed => continuationValue = completed.Result + 1);
        continuation.Wait();
        Console.WriteLine("TaskTest: continuation completed");
        Require(continuationValue == 43, "ContinueWith did not observe the completed result.");

        TaskCompletionSource<int> source = new TaskCompletionSource<int>();
        Thread producer = new Thread(() => source.SetResult(7));
        producer.Start();
        Require(source.Task.Result == 7, "TaskCompletionSource did not publish its result.");
        producer.Join();
        Console.WriteLine("TaskTest: completion source completed");
    }

    private static void TestFailureAndCancellation()
    {
        Task failed = Task.Run(() =>
        {
            throw new InvalidOperationException("worker-failure");
        });

        bool failedAsExpected = false;
        try
        {
            failed.Wait();
        }
        catch (AggregateException exception)
        {
            failedAsExpected = exception.InnerExceptions.Length == 1 && exception.InnerExceptions[0].Message == "worker-failure";
        }

        Require(failedAsExpected && failed.IsFaulted, "Faulted task did not preserve its exception.");

        CancellationTokenSource source = new CancellationTokenSource();
        int callbackCount = 0;
        CancellationTokenRegistration registration = source.Token.Register(() => callbackCount++);
        source.Cancel();
        registration.Dispose();
        Require(source.IsCancellationRequested && callbackCount == 1, "Cancellation callback did not execute exactly once.");

        Task canceled = Task.Delay(-1, source.Token);
        bool canceledAsExpected = false;
        try
        {
            canceled.Wait();
        }
        catch (AggregateException exception)
        {
            canceledAsExpected = exception.InnerExceptions[0] is TaskCanceledException;
        }

        Require(canceledAsExpected && canceled.IsCanceled, "Canceled delay has an incorrect final state.");

        CancellationTokenSource first = new CancellationTokenSource();
        CancellationTokenSource second = new CancellationTokenSource();
        CancellationTokenSource linked = CancellationTokenSource.CreateLinkedTokenSource(first.Token, second.Token);
        second.Cancel();
        Require(linked.IsCancellationRequested, "Linked cancellation was not propagated.");
        linked.Dispose();
        first.Dispose();
        second.Dispose();
        source.Dispose();
    }

    private static void TestCancellationScheduling()
    {
        CancellationTokenSource disabled = new CancellationTokenSource();
        disabled.CancelAfter(10);
        disabled.CancelAfter(-1);
        Thread.Sleep(30);
        Require(!disabled.IsCancellationRequested, "CancelAfter(-1) did not disable scheduled cancellation.");
        disabled.Dispose();

        CancellationTokenSource rescheduled = new CancellationTokenSource();
        rescheduled.CancelAfter(10);
        rescheduled.CancelAfter(100);
        Thread.Sleep(30);
        Require(!rescheduled.IsCancellationRequested, "A stale CancelAfter timer canceled a rescheduled source.");
        rescheduled.Cancel();
        rescheduled.Dispose();

        CancellationTokenSource disposed = new CancellationTokenSource();
        disposed.CancelAfter(5);
        disposed.Dispose();
        Thread.Sleep(15);

        CancellationTokenSource failures = new CancellationTokenSource();
        InvalidOperationException firstFailure = new InvalidOperationException("first callback");
        ArgumentException secondFailure = new ArgumentException("second callback");
        failures.Token.Register(() => throw firstFailure);
        failures.Token.Register(() => throw secondFailure);

        bool aggregatePreserved = false;
        try
        {
            failures.Cancel();
        }
        catch (AggregateException exception)
        {
            aggregatePreserved = exception.InnerExceptions.Length == 2 &&
                                 object.ReferenceEquals(exception.InnerExceptions[0], secondFailure) &&
                                 object.ReferenceEquals(exception.InnerExceptions[1], firstFailure);
        }

        Require(aggregatePreserved, "Cancellation did not aggregate callback failures in reverse registration order.");
        failures.Dispose();
    }

    private static void TestAsyncStateMachines()
    {
        Require(ComputeAsync(20).Result == 41, "async/await state machine returned the wrong result.");
        Require(CatchAwaitedFailureAsync().Result, "await did not rethrow the original exception.");

        Task first = Task.Delay(1);
        Task second = Task.Run(() => { });
        Task.WhenAll(first, second).Wait();
        Task.WaitAll(first, second);
    }

    public static void Main()
    {
        Console.WriteLine("TaskTest: execution");
        TestTaskExecution();
        Console.WriteLine("TaskTest: cancellation");
        TestFailureAndCancellation();
        TestCancellationScheduling();
        Console.WriteLine("TaskTest: async state machines");
        TestAsyncStateMachines();
        Console.WriteLine("TaskTest: PASS");
    }
}
