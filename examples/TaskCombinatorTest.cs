using System;
using System.Threading;
using System.Threading.Tasks;

public static class TaskCombinatorTest
{
    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static void TestInfiniteDelay()
    {
        Task infinite = Task.Delay(-1);
        Require(!infinite.Wait(2), "An infinite delay completed without cancellation.");

        CancellationTokenSource source = new CancellationTokenSource();
        Task cancellable = Task.Delay(-1, source.Token);
        source.CancelAfter(2);

        bool canceled = false;
        try
        {
            cancellable.Wait();
        }
        catch (AggregateException exception)
        {
            canceled = exception.InnerExceptions.Length == 1 && exception.InnerExceptions[0] is TaskCanceledException;
        }

        Require(canceled && cancellable.IsCanceled, "An infinite cancellable delay did not transition to Canceled.");
        source.Dispose();
    }

    private static void TestCompletionSources()
    {
        object state = new object();
        TaskCompletionSource<int> source = new TaskCompletionSource<int>(state);
        source.SetResult(10);

        Require(object.ReferenceEquals(source.Task.AsyncState, state), "TaskCompletionSource did not preserve AsyncState.");
        Require(!source.TrySetResult(20), "TaskCompletionSource accepted a second result.");
        Require(source.Task.Result == 10, "A failed TrySetResult overwrote the published result.");

        int observedState = 0;
        Task stateTask = new Task(value => observedState = (int)value, 42);
        Require((int)stateTask.AsyncState == 42, "Task did not preserve constructor state.");
        stateTask.RunSynchronously();
        Require(observedState == 42, "Task(Action<object>, object) did not receive its state.");
    }

    private static void TestWhenAllResults()
    {
        TaskCompletionSource<int> first = new TaskCompletionSource<int>();
        TaskCompletionSource<int> second = new TaskCompletionSource<int>();
        Task<int[]> combined = Task.WhenAll(first.Task, second.Task);

        second.SetResult(2);
        Require(!combined.IsCompleted, "WhenAll completed before all input tasks.");
        first.SetResult(1);

        int[] results = combined.Result;
        Require(results.Length == 2 && results[0] == 1 && results[1] == 2, "WhenAll<TResult> did not preserve input order.");
        Require(Task.WhenAll(new Task<int>[0]).Result.Length == 0, "Empty WhenAll<TResult> returned an invalid result.");
    }

    private static void TestWhenAllFailures()
    {
        InvalidOperationException firstFailure = new InvalidOperationException("first");
        ArgumentException secondFailure = new ArgumentException("second");
        TaskCompletionSource<object> first = new TaskCompletionSource<object>();
        TaskCompletionSource<object> second = new TaskCompletionSource<object>();
        Task combined = Task.WhenAll(first.Task, second.Task);

        second.SetException(secondFailure);
        first.SetException(firstFailure);

        bool aggregateWasFlat = false;
        try
        {
            combined.Wait();
        }
        catch (AggregateException exception)
        {
            aggregateWasFlat = exception.InnerExceptions.Length == 2 &&
                               object.ReferenceEquals(exception.InnerExceptions[0], firstFailure) &&
                               object.ReferenceEquals(exception.InnerExceptions[1], secondFailure);
        }

        Require(aggregateWasFlat, "WhenAll did not retain all failures without nested aggregation.");
        Require(combined.Exception.InnerExceptions.Length == 2, "Task.Exception lost WhenAll failures.");
        Require(ObserveAwaitedFailure(combined).Result == "first", "Awaiting WhenAll did not throw its first underlying exception.");

        CancellationTokenSource source = new CancellationTokenSource();
        source.Cancel();
        Task canceled = Task.WhenAll(Task.FromCanceled(source.Token), Task.CompletedTask);

        bool cancellationTokenPreserved = false;
        try
        {
            canceled.Wait();
        }
        catch (AggregateException exception)
        {
            TaskCanceledException canceledException = (TaskCanceledException)exception.InnerExceptions[0];
            cancellationTokenPreserved = canceledException.CancellationToken == source.Token;
        }

        Require(canceled.IsCanceled && cancellationTokenPreserved, "WhenAll did not propagate cancellation and its token.");
        source.Dispose();
    }

    private static async Task<string> ObserveAwaitedFailure(Task task)
    {
        try
        {
            await task;
            return null;
        }
        catch (Exception exception)
        {
            return exception.Message;
        }
    }

    private static void TestWhenAnyAndWaitAny()
    {
        TaskCompletionSource<int> first = new TaskCompletionSource<int>();
        TaskCompletionSource<int> second = new TaskCompletionSource<int>();
        Task<Task<int>> any = Task.WhenAny(first.Task, second.Task);

        second.SetResult(22);
        Require(object.ReferenceEquals(any.Result, second.Task), "WhenAny<TResult> returned the wrong completed task.");

        Task completedFirst = Task.FromResult(1);
        Task completedSecond = Task.FromResult(2);
        Require(object.ReferenceEquals(Task.WhenAny(completedFirst, completedSecond).Result, completedFirst), "WhenAny did not use input order for already completed tasks.");

        Task never = Task.Delay(-1);
        Require(Task.WaitAny(new Task[] { never }, 2) == -1, "WaitAny did not report a timeout.");

        TaskCompletionSource<object> delayed = new TaskCompletionSource<object>();
        Thread producer = new Thread(() =>
        {
            Thread.Sleep(2);
            delayed.SetResult(null);
        });
        producer.Start();
        int completedIndex = Task.WaitAny(new Task[] { never, delayed.Task }, 1000);
        producer.Join();
        Require(completedIndex == 1, "WaitAny returned the wrong task index.");
        Require(!Task.WaitAll(new Task[] { never }, 2), "WaitAll did not report a timeout.");
    }

    private static void TestCancelableWaits()
    {
        Task never = Task.Delay(-1);

        CancellationTokenSource singleSource = new CancellationTokenSource();
        singleSource.CancelAfter(2);
        bool singleCanceled = false;
        try
        {
            never.Wait(singleSource.Token);
        }
        catch (OperationCanceledException exception)
        {
            singleCanceled = exception.CancellationToken == singleSource.Token;
        }
        Require(singleCanceled, "Task.Wait did not observe its cancellation token.");
        singleSource.Dispose();

        CancellationTokenSource allSource = new CancellationTokenSource();
        allSource.CancelAfter(2);
        bool allCanceled = false;
        try
        {
            Task.WaitAll(new Task[] { never }, allSource.Token);
        }
        catch (OperationCanceledException exception)
        {
            allCanceled = exception.CancellationToken == allSource.Token;
        }
        Require(allCanceled, "Task.WaitAll did not observe its cancellation token.");
        allSource.Dispose();

        CancellationTokenSource anySource = new CancellationTokenSource();
        anySource.CancelAfter(2);
        bool anyCanceled = false;
        try
        {
            Task.WaitAny(new Task[] { never }, anySource.Token);
        }
        catch (OperationCanceledException exception)
        {
            anyCanceled = exception.CancellationToken == anySource.Token;
        }
        Require(anyCanceled, "Task.WaitAny did not observe its cancellation token.");
        anySource.Dispose();
    }

    private static void TestUnwrapAndResultContinuations()
    {
        Task<int> asyncRun = Task.Run(async () =>
        {
            await Task.Yield();
            return 42;
        });
        Require(asyncRun.Result == 42, "Task.Run(Func<Task<TResult>>) did not unwrap its result.");

        TaskCompletionSource<Task<int>> outer = new TaskCompletionSource<Task<int>>();
        Task<int> unwrapped = outer.Task.Unwrap();
        TaskCompletionSource<int> inner = new TaskCompletionSource<int>();
        outer.SetResult(inner.Task);
        Require(!unwrapped.IsCompleted, "Unwrap completed before the inner task.");
        inner.SetResult(7);
        Require(unwrapped.Result == 7, "TaskExtensions.Unwrap lost the inner result.");

        InvalidOperationException failure = new InvalidOperationException("inner-failure");
        Task failedUnwrap = Task.FromResult(Task.FromException(failure)).Unwrap();
        bool failurePreserved = false;
        try
        {
            failedUnwrap.Wait();
        }
        catch (AggregateException exception)
        {
            failurePreserved = exception.InnerExceptions.Length == 1 && object.ReferenceEquals(exception.InnerExceptions[0], failure);
        }
        Require(failurePreserved, "Unwrap did not preserve the inner exception.");

        TaskCompletionSource<Task> nullOuter = new TaskCompletionSource<Task>();
        Task nullUnwrap = nullOuter.Task.Unwrap();
        nullOuter.SetResult(null);
        bool nullRejected = false;
        try
        {
            nullUnwrap.Wait();
        }
        catch (AggregateException exception)
        {
            nullRejected = exception.InnerExceptions.Length == 1 && exception.InnerExceptions[0] is InvalidOperationException;
        }
        Require(nullRejected, "Unwrap accepted a null inner task.");

        Task<int> continuation = Task.FromResult(5).ContinueWith(completed => completed.Result * 3);
        Require(continuation.Result == 15, "Result-producing ContinueWith failed.");
    }

    public static int Main()
    {
        TestInfiniteDelay();
        TestCompletionSources();
        TestWhenAllResults();
        TestWhenAllFailures();
        TestWhenAnyAndWaitAny();
        TestCancelableWaits();
        TestUnwrapAndResultContinuations();
        Console.WriteLine("Task combinator test passed");
        return 0;
    }
}
