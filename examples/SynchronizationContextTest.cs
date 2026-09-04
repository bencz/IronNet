using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

internal sealed class PumpContext : SynchronizationContext
{
    private readonly Queue<Action> _callbacks = new Queue<Action>();
    private readonly AutoResetEvent _ready = new AutoResetEvent(false);
    public int Posts;
    public int Started;
    public int Completed;
    public Exception Failure;

    public override void Post(SendOrPostCallback callback, object state)
    {
        lock (_callbacks)
        {
            _callbacks.Enqueue(() => callback(state));
            Posts++;
        }

        _ready.Set();
    }

    public override void OperationStarted()
    {
        Started++;
    }

    public override void OperationCompleted()
    {
        Completed++;
    }

    public void RunOne()
    {
        Action callback = null;
        while (callback == null)
        {
            lock (_callbacks)
            {
                if (_callbacks.Count != 0)
                {
                    callback = _callbacks.Dequeue();
                }
            }

            if (callback == null && !_ready.WaitOne(5000))
            {
                throw new Exception("The continuation was not posted to the captured context.");
            }
        }

        try
        {
            callback();
        }
        catch (Exception exception)
        {
            Failure = exception;
        }
    }
}

internal static class SynchronizationContextTest
{
    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static async Task<bool> AwaitTask(Task<int> task, PumpContext context, bool capture, bool generic)
    {
        if (generic)
        {
            Require(await task.ConfigureAwait(capture) == 42, "The generic Task lost its result.");
        }
        else
        {
            await ((Task)task).ConfigureAwait(capture);
        }

        return ReferenceEquals(SynchronizationContext.Current, context);
    }

    private static async ValueTask<bool> AwaitValueTask(Task<int> task, PumpContext context, bool capture, bool generic)
    {
        if (generic)
        {
            Require(await new ValueTask<int>(task).ConfigureAwait(capture) == 42, "The generic ValueTask lost its result.");
        }
        else
        {
            await new ValueTask(task).ConfigureAwait(capture);
        }

        return ReferenceEquals(SynchronizationContext.Current, context);
    }

    private static void TestConfiguredAwaits(PumpContext context)
    {
        for (int kind = 0; kind < 4; kind++)
        {
            for (int captureIndex = 0; captureIndex < 2; captureIndex++)
            {
                bool capture = captureIndex != 0;
                TaskCompletionSource<int> source = new TaskCompletionSource<int>();
                int posts = context.Posts;
                Task<bool> observer = kind < 2
                    ? AwaitTask(source.Task, context, capture, kind == 1)
                    : AwaitValueTask(source.Task, context, capture, kind == 3).AsTask();

                // Completion occurs under a different context than await registration.
                SynchronizationContext.SetSynchronizationContext(new SynchronizationContext());
                source.SetResult(42);
                SynchronizationContext.SetSynchronizationContext(context);
                if (capture)
                {
                    Require(!observer.IsCompleted, "A captured continuation bypassed the dispatcher queue.");
                    context.RunOne();
                }

                Require(observer.Wait(5000), "An await continuation did not complete.");
                Require(observer.Result == capture, "ConfigureAwait did not respect the capture argument.");
                Require(context.Posts == posts + (capture ? 1 : 0), "An await posted to the wrong context.");
                Require(ReferenceEquals(SynchronizationContext.Current, context), "A continuation leaked its context into the caller.");
            }
        }
    }

    private static async Task<bool> AwaitNormally(Task task, PumpContext context)
    {
        await task;
        return ReferenceEquals(SynchronizationContext.Current, context);
    }

    private static async Task<bool> YieldNormally(PumpContext context)
    {
        await Task.Yield();
        return ReferenceEquals(SynchronizationContext.Current, context);
    }

    private static async Task ChangeContext(Task task)
    {
        SynchronizationContext.SetSynchronizationContext(null);
        await task.ConfigureAwait(false);
    }

    private static async void VoidOperation(Task task, Exception failure)
    {
        await task;
        if (failure != null)
        {
            throw failure;
        }
    }

    private static void TestDefaultAwaits(PumpContext context)
    {
        TaskCompletionSource<int> source = new TaskCompletionSource<int>();
        Task<bool> observer = AwaitNormally(source.Task, context);
        CompleteOffContext(source);
        context.RunOne();
        Require(observer.Result, "An ordinary await did not capture the context.");

        observer = YieldNormally(context);
        context.RunOne();
        Require(observer.Result, "Task.Yield did not capture the context.");

        int posts = context.Posts;
        observer = AwaitTask(Task.FromResult(42), context, false, true);
        Require(observer.IsCompletedSuccessfully && observer.Result, "A synchronous await changed the current context.");
        Require(posts == context.Posts, "A synchronous await posted a continuation.");

        source = new TaskCompletionSource<int>();
        Task changed = ChangeContext(source.Task);
        Require(ReferenceEquals(SynchronizationContext.Current, context), "Builder.Start leaked a context change into its caller.");
        source.SetResult(1);
        Require(changed.Wait(5000), "The context-changing async method did not finish.");
    }

    private static void TestAsyncVoid(PumpContext context)
    {
        TaskCompletionSource<int> source = new TaskCompletionSource<int>();
        VoidOperation(source.Task, null);
        Require(context.Started == 1 && context.Completed == 0, "async void did not notify operation start.");
        CompleteOffContext(source);
        context.RunOne();
        Require(context.Completed == 1, "async void did not notify successful completion.");

        InvalidOperationException failure = new InvalidOperationException("async-void-failure");
        source = new TaskCompletionSource<int>();
        VoidOperation(source.Task, failure);
        CompleteOffContext(source);
        context.RunOne();
        Require(context.Started == 2 && context.Completed == 2, "async void did not balance faulted operation notifications.");
        context.RunOne();
        Require(ReferenceEquals(context.Failure, failure), "async void did not deliver its exception to the captured context.");
    }

    private static void CompleteOffContext(TaskCompletionSource<int> source)
    {
        SynchronizationContext previous = SynchronizationContext.Current;
        try
        {
            SynchronizationContext.SetSynchronizationContext(null);
            source.SetResult(1);
        }
        finally
        {
            SynchronizationContext.SetSynchronizationContext(previous);
        }
    }

    public static void Main()
    {
        PumpContext context = new PumpContext();
        SynchronizationContext.SetSynchronizationContext(context);
        try
        {
            bool isolated = false;
            Thread thread = new Thread(() => isolated = SynchronizationContext.Current == null);
            thread.Start();
            Require(thread.Join(5000) && isolated, "SynchronizationContext.Current was not isolated per thread.");
            TestConfiguredAwaits(context);
            TestDefaultAwaits(context);
            Require(context.Failure == null, "The dispatcher observed an unexpected failure.");
            TestAsyncVoid(context);
        }
        finally
        {
            SynchronizationContext.SetSynchronizationContext(null);
        }

        Console.WriteLine("SynchronizationContext test passed");
    }
}
