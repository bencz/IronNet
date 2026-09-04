using System;
using System.Threading;
using System.Threading.Tasks;

internal sealed class AsyncResource : IAsyncDisposable
{
    public bool IsDisposed { get; private set; }

    public async ValueTask DisposeAsync()
    {
        await Task.Yield();
        IsDisposed = true;
    }
}

internal static class ValueTaskTest
{
    private static int _assertions;

    private static async ValueTask<int> ComputeAsync(int value, bool yield)
    {
        if (yield)
        {
            await Task.Yield();
        }

        return value * 2;
    }

    private static async ValueTask CompleteAsync()
    {
        await Task.Delay(1).ConfigureAwait(false);
    }

    private static async ValueTask<int> FailAsync(Exception exception)
    {
        await Task.Yield();
        throw exception;
    }

    private static async ValueTask<int> CancelAsync(CancellationToken cancellationToken)
    {
        await Task.Yield();
        cancellationToken.ThrowIfCancellationRequested();
        return 1;
    }

    private static async Task TestAwaitUsing()
    {
        AsyncResource resource = new AsyncResource();
        await using (resource)
        {
            Require(!resource.IsDisposed, "The async resource was disposed before its scope ended.");
        }

        Require(resource.IsDisposed, "await using did not await IAsyncDisposable.DisposeAsync.");
    }

    private static void TestCompletedValues()
    {
        ValueTask completed = ValueTask.CompletedTask;
        Require(completed.IsCompletedSuccessfully, "ValueTask.CompletedTask was not completed successfully.");
        completed.GetAwaiter().GetResult();
        Require(object.ReferenceEquals(completed.AsTask(), Task.CompletedTask), "A completed ValueTask did not map to Task.CompletedTask.");

        ValueTask<int> value = new ValueTask<int>(42);
        Require(value.IsCompletedSuccessfully && value.Result == 42, "An inline ValueTask<TResult> lost its result.");
        Require(value.AsTask().Result == 42, "ValueTask<TResult>.AsTask lost an inline result.");
        Require(value == new ValueTask<int>(42), "Inline ValueTask<TResult> equality is incorrect.");
    }

    private static void TestTaskBackedValues()
    {
        TaskCompletionSource<int> source = new TaskCompletionSource<int>();
        ValueTask<int> value = new ValueTask<int>(source.Task);
        Require(!value.IsCompleted, "A pending task-backed ValueTask was reported as completed.");
        source.SetResult(17);
        Require(value.Result == 17 && value.IsCompletedSuccessfully, "A task-backed ValueTask lost its completion.");

        ValueTask<int> configured = ComputeAsync(9, true);
        Require(configured.ConfigureAwait(false).GetAwaiter().GetResult() == 18, "ConfiguredValueTaskAwaitable<TResult> lost its result.");
        CompleteAsync().ConfigureAwait(false).GetAwaiter().GetResult();
    }

    private static void TestFailuresAndCancellation()
    {
        InvalidOperationException failure = new InvalidOperationException("value-task-failure");
        bool failurePreserved = false;
        try
        {
            FailAsync(failure).AsTask().Wait();
        }
        catch (AggregateException exception)
        {
            failurePreserved = exception.InnerExceptions.Length == 1 && object.ReferenceEquals(exception.InnerExceptions[0], failure);
        }

        Require(failurePreserved, "An async ValueTask did not preserve its exception.");

        CancellationTokenSource source = new CancellationTokenSource();
        source.Cancel();
        ValueTask<int> canceled = CancelAsync(source.Token);
        bool tokenPreserved = false;
        try
        {
            canceled.AsTask().Wait();
        }
        catch (AggregateException exception)
        {
            TaskCanceledException canceledException = exception.InnerExceptions[0] as TaskCanceledException;
            tokenPreserved = canceledException != null && canceledException.CancellationToken == source.Token;
        }

        Require(canceled.IsCanceled && tokenPreserved, "An async ValueTask did not preserve cancellation and its token.");
        source.Dispose();
    }

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }

        _assertions++;
    }

    public static void Main()
    {
        TestCompletedValues();
        TestTaskBackedValues();
        TestFailuresAndCancellation();
        TestAwaitUsing().Wait();
        Console.Write("ValueTask assertions passed: ");
        Console.WriteLine(_assertions);
    }
}
