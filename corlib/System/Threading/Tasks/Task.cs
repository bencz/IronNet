using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace System.Threading.Tasks
{
    public enum TaskStatus
    {
        Created = 0,
        WaitingForActivation = 1,
        WaitingToRun = 2,
        Running = 3,
        WaitingForChildrenToComplete = 4,
        RanToCompletion = 5,
        Canceled = 6,
        Faulted = 7
    }

    [Flags]
    public enum TaskCreationOptions
    {
        None = 0,
        PreferFairness = 1,
        LongRunning = 2,
        AttachedToParent = 4,
        DenyChildAttach = 8,
        HideScheduler = 16,
        RunContinuationsAsynchronously = 64
    }

    public class TaskCanceledException : OperationCanceledException
    {
        private readonly Task _task;

        public TaskCanceledException() : base("A task was canceled.")
        {
        }

        public TaskCanceledException(string message) : base(message)
        {
        }

        public TaskCanceledException(Task task) : base("A task was canceled.", task == null ? default(CancellationToken) : task.CancellationToken)
        {
            _task = task;
        }

        public Task Task => _task;
    }

    public class Task : IDisposable
    {
        private static readonly Task _completedTask = CreateCompletedTask();

        private readonly object _gate = new object();
        private readonly Action _action;
        private readonly object _asyncState;
        private CancellationToken _cancellationToken;
        private List<Action> _continuations;
        private volatile TaskStatus _status;
        private Exception[] _exceptions;
        private bool _disposed;

        public Task(Action action) : this(action, CancellationToken.None)
        {
        }

        public Task(Action action, CancellationToken cancellationToken) : this(action, cancellationToken, null)
        {
        }

        public Task(Action<object> action, object state) : this(action, state, CancellationToken.None)
        {
        }

        public Task(Action<object> action, object state, CancellationToken cancellationToken)
            : this(CreateAction(action, state), cancellationToken, state)
        {
        }

        private Task(Action action, CancellationToken cancellationToken, object asyncState)
        {
            if (action == null)
            {
                throw new ArgumentNullException("action");
            }

            _action = action;
            _asyncState = asyncState;
            _cancellationToken = cancellationToken;
            _status = TaskStatus.Created;
        }

        protected Task(bool promiseStyle) : this(promiseStyle, null)
        {
        }

        protected Task(bool promiseStyle, object asyncState)
        {
            _asyncState = asyncState;
            _status = promiseStyle ? TaskStatus.WaitingForActivation : TaskStatus.Created;
        }

        public static Task CompletedTask => _completedTask;
        public object AsyncState => _asyncState;
        public TaskStatus Status => _status;
        public bool IsCompleted => IsFinalStatus(_status);
        public bool IsCompletedSuccessfully => _status == TaskStatus.RanToCompletion;
        public bool IsCanceled => _status == TaskStatus.Canceled;
        public bool IsFaulted => _status == TaskStatus.Faulted;
        public AggregateException Exception => _exceptions == null ? null : new AggregateException(_exceptions);
        internal CancellationToken CancellationToken => _cancellationToken;

        public void Start()
        {
            ThrowIfDisposed();

            Monitor.Enter(_gate);
            try
            {
                if (_status != TaskStatus.Created)
                {
                    throw new InvalidOperationException("Start may not be called on a task that has already started or is promise-based.");
                }

                _status = TaskStatus.WaitingToRun;
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            Thread thread = new Thread(ExecuteEntry);
            thread.IsBackground = true;
            thread.Start();
        }

        public void RunSynchronously()
        {
            ThrowIfDisposed();

            Monitor.Enter(_gate);
            try
            {
                if (_status != TaskStatus.Created)
                {
                    throw new InvalidOperationException("RunSynchronously may only be called on a task in the Created state.");
                }

                _status = TaskStatus.WaitingToRun;
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            ExecuteEntry();
        }

        public void Wait()
        {
            Wait(-1);
        }

        public void Wait(CancellationToken cancellationToken)
        {
            Wait(-1, cancellationToken);
        }

        public bool Wait(int millisecondsTimeout)
        {
            return Wait(millisecondsTimeout, CancellationToken.None);
        }

        public bool Wait(int millisecondsTimeout, CancellationToken cancellationToken)
        {
            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsTimeout");
            }

            ThrowIfDisposed();
            bool completed = WaitWithoutThrow(millisecondsTimeout, cancellationToken);
            if (!completed)
            {
                return false;
            }

            ThrowIfExceptional(true);
            return true;
        }

        public bool Wait(TimeSpan timeout)
        {
            return Wait(timeout, CancellationToken.None);
        }

        public bool Wait(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Wait(GetMillisecondsTimeout(timeout, "timeout"), cancellationToken);
        }

        public Task ContinueWith(Action<Task> continuationAction)
        {
            if (continuationAction == null)
            {
                throw new ArgumentNullException("continuationAction");
            }

            Task continuationTask = new Task(() => continuationAction(this));
            RegisterContinuation(continuationTask.Start);
            return continuationTask;
        }

        public Task<TResult> ContinueWith<TResult>(Func<Task, TResult> continuationFunction)
        {
            if (continuationFunction == null)
            {
                throw new ArgumentNullException("continuationFunction");
            }

            Task<TResult> continuationTask = new Task<TResult>(() => continuationFunction(this));
            RegisterContinuation(continuationTask.Start);
            return continuationTask;
        }

        public TaskAwaiter GetAwaiter()
        {
            return new TaskAwaiter(this);
        }

        public ConfiguredTaskAwaitable ConfigureAwait(bool continueOnCapturedContext)
        {
            return new ConfiguredTaskAwaitable(this, continueOnCapturedContext);
        }

        public static Task Run(Action action)
        {
            return Run(action, CancellationToken.None);
        }

        public static Task Run(Action action, CancellationToken cancellationToken)
        {
            Task task = new Task(action, cancellationToken);
            task.Start();
            return task;
        }

        public static Task<TResult> Run<TResult>(Func<TResult> function)
        {
            return Run(function, CancellationToken.None);
        }

        public static Task<TResult> Run<TResult>(Func<TResult> function, CancellationToken cancellationToken)
        {
            Task<TResult> task = new Task<TResult>(function, cancellationToken);
            task.Start();
            return task;
        }

        public static Task Run(Func<Task> function)
        {
            return RunUnwrapped(function, CancellationToken.None);
        }

        public static Task Run(Func<Task> function, CancellationToken cancellationToken)
        {
            return RunUnwrapped(function, cancellationToken);
        }

        private static Task RunUnwrapped(Func<Task> function, CancellationToken cancellationToken)
        {
            if (function == null)
            {
                throw new ArgumentNullException("function");
            }
            if (cancellationToken.IsCancellationRequested)
            {
                return FromCanceled(cancellationToken);
            }

            TaskCompletionSource<object> source = new TaskCompletionSource<object>();
            Task launcher = new Task(() =>
            {
                try
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    Task inner = function();
                    if (inner == null)
                    {
                        source.TrySetException(new InvalidOperationException("The asynchronous delegate returned a null task."));
                    }
                    else
                    {
                        inner.RegisterContinuation(() => PropagateCompletion(inner, source));
                    }
                }
                catch (OperationCanceledException exception)
                {
                    CancellationToken token = exception.CancellationToken.CanBeCanceled ? exception.CancellationToken : cancellationToken;
                    source.TrySetCanceled(token);
                }
                catch (Exception exception)
                {
                    source.TrySetException(exception);
                }
            });
            launcher.Start();
            return source.Task;
        }

        public static Task<TResult> Run<TResult>(Func<Task<TResult>> function)
        {
            return RunUnwrapped(function, CancellationToken.None);
        }

        public static Task<TResult> Run<TResult>(Func<Task<TResult>> function, CancellationToken cancellationToken)
        {
            return RunUnwrapped(function, cancellationToken);
        }

        private static Task<TResult> RunUnwrapped<TResult>(Func<Task<TResult>> function, CancellationToken cancellationToken)
        {
            if (function == null)
            {
                throw new ArgumentNullException("function");
            }
            if (cancellationToken.IsCancellationRequested)
            {
                return FromCanceled<TResult>(cancellationToken);
            }

            TaskCompletionSource<TResult> source = new TaskCompletionSource<TResult>();
            Task launcher = new Task(() =>
            {
                try
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    Task<TResult> inner = function();
                    if (inner == null)
                    {
                        source.TrySetException(new InvalidOperationException("The asynchronous delegate returned a null task."));
                    }
                    else
                    {
                        inner.RegisterContinuation(() => PropagateCompletion(inner, source));
                    }
                }
                catch (OperationCanceledException exception)
                {
                    CancellationToken token = exception.CancellationToken.CanBeCanceled ? exception.CancellationToken : cancellationToken;
                    source.TrySetCanceled(token);
                }
                catch (Exception exception)
                {
                    source.TrySetException(exception);
                }
            });
            launcher.Start();
            return source.Task;
        }

        public static Task<TResult> FromResult<TResult>(TResult result)
        {
            Task<TResult> task = new Task<TResult>(true);
            task.TrySetResult(result);
            return task;
        }

        public static Task FromException(Exception exception)
        {
            if (exception == null)
            {
                throw new ArgumentNullException("exception");
            }

            Task task = new Task(true);
            task.TrySetException(exception);
            return task;
        }

        public static Task<TResult> FromException<TResult>(Exception exception)
        {
            if (exception == null)
            {
                throw new ArgumentNullException("exception");
            }

            Task<TResult> task = new Task<TResult>(true);
            task.TrySetException(exception);
            return task;
        }

        public static Task FromCanceled(CancellationToken cancellationToken)
        {
            if (!cancellationToken.IsCancellationRequested)
            {
                throw new ArgumentOutOfRangeException("cancellationToken");
            }

            Task task = new Task(true);
            task.TrySetCanceled(cancellationToken);
            return task;
        }

        public static Task<TResult> FromCanceled<TResult>(CancellationToken cancellationToken)
        {
            if (!cancellationToken.IsCancellationRequested)
            {
                throw new ArgumentOutOfRangeException("cancellationToken");
            }

            Task<TResult> task = new Task<TResult>(true);
            task.TrySetCanceled(cancellationToken);
            return task;
        }

        public static Task Delay(int millisecondsDelay)
        {
            return Delay(millisecondsDelay, CancellationToken.None);
        }

        public static Task Delay(TimeSpan delay)
        {
            return Delay(delay, CancellationToken.None);
        }

        public static Task Delay(TimeSpan delay, CancellationToken cancellationToken)
        {
            double milliseconds = delay.TotalMilliseconds;
            if (milliseconds < -1 || milliseconds > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException("delay");
            }

            return Delay((int)milliseconds, cancellationToken);
        }

        public static Task Delay(int millisecondsDelay, CancellationToken cancellationToken)
        {
            if (millisecondsDelay < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsDelay");
            }
            if (cancellationToken.IsCancellationRequested)
            {
                return FromCanceled(cancellationToken);
            }
            if (millisecondsDelay == 0)
            {
                return CompletedTask;
            }

            if (millisecondsDelay == -1 && !cancellationToken.CanBeCanceled)
            {
                return new TaskCompletionSource<object>().Task;
            }

            TaskCompletionSource<object> source = new TaskCompletionSource<object>();
            Thread timerThread = new Thread(() =>
            {
                if (cancellationToken.CanBeCanceled && cancellationToken.WaitHandle.WaitOne(millisecondsDelay))
                {
                    source.TrySetCanceled(cancellationToken);
                }
                else
                {
                    if (millisecondsDelay >= 0 && !cancellationToken.CanBeCanceled)
                    {
                        Thread.Sleep(millisecondsDelay);
                    }

                    source.TrySetResult(null);
                }
            });
            timerThread.IsBackground = true;
            timerThread.Start();
            return source.Task;
        }

        public static void WaitAll(params Task[] tasks)
        {
            WaitAllCore(tasks, -1);
        }

        public static bool WaitAll(Task[] tasks, int millisecondsTimeout)
        {
            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsTimeout");
            }

            return WaitAllCore(tasks, millisecondsTimeout);
        }

        public static bool WaitAll(Task[] tasks, TimeSpan timeout)
        {
            return WaitAll(tasks, GetMillisecondsTimeout(timeout, "timeout"));
        }

        public static void WaitAll(Task[] tasks, CancellationToken cancellationToken)
        {
            WaitAllCore(tasks, -1, cancellationToken);
        }

        public static bool WaitAll(Task[] tasks, int millisecondsTimeout, CancellationToken cancellationToken)
        {
            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsTimeout");
            }

            return WaitAllCore(tasks, millisecondsTimeout, cancellationToken);
        }

        private static bool WaitAllCore(Task[] tasks, int millisecondsTimeout)
        {
            return WaitAllCore(tasks, millisecondsTimeout, CancellationToken.None);
        }

        private static bool WaitAllCore(Task[] tasks, int millisecondsTimeout, CancellationToken cancellationToken)
        {
            Task[] copy = ValidateTaskArray(tasks, true);
            List<Exception> exceptions = null;
            bool canceled = false;
            int start = Environment.TickCount;
            for (int i = 0; i < copy.Length; i++)
            {
                int remaining = RemainingTimeout(start, millisecondsTimeout);
                if (!copy[i].WaitWithoutThrow(remaining, cancellationToken))
                {
                    return false;
                }

                Exception[] taskExceptions = copy[i]._exceptions;
                if (taskExceptions != null)
                {
                    if (exceptions == null)
                    {
                        exceptions = new List<Exception>();
                    }

                    for (int exceptionIndex = 0; exceptionIndex < taskExceptions.Length; exceptionIndex++)
                    {
                        exceptions.Add(taskExceptions[exceptionIndex]);
                    }
                }

                canceled |= copy[i].IsCanceled;
            }

            if (exceptions != null)
            {
                throw new AggregateException(exceptions.ToArray());
            }
            if (canceled)
            {
                throw new AggregateException(new TaskCanceledException());
            }

            return true;
        }

        public static Task WhenAll(params Task[] tasks)
        {
            Task[] copy = ValidateTaskArray(tasks, true);

            if (copy.Length == 0)
            {
                return CompletedTask;
            }

            TaskCompletionSource<object> source = new TaskCompletionSource<object>();
            WhenAllCoordinator coordinator = new WhenAllCoordinator(copy.Length, source);
            for (int i = 0; i < copy.Length; i++)
            {
                int taskIndex = i;
                Task observedTask = copy[i];
                observedTask.RegisterContinuation(() => coordinator.Complete(observedTask, taskIndex));
            }

            return source.Task;
        }

        public static Task<TResult[]> WhenAll<TResult>(params Task<TResult>[] tasks)
        {
            Task<TResult>[] copy = ValidateTaskArray(tasks, true);
            if (copy.Length == 0)
            {
                return FromResult(new TResult[0]);
            }

            TaskCompletionSource<TResult[]> source = new TaskCompletionSource<TResult[]>();
            WhenAllCoordinator<TResult> coordinator = new WhenAllCoordinator<TResult>(copy, source);
            for (int i = 0; i < copy.Length; i++)
            {
                int resultIndex = i;
                Task<TResult> observedTask = copy[i];
                observedTask.RegisterContinuation(() => coordinator.Complete(observedTask, resultIndex));
            }

            return source.Task;
        }

        public static Task<Task> WhenAny(params Task[] tasks)
        {
            Task[] copy = ValidateTaskArray(tasks, false);
            TaskCompletionSource<Task> source = new TaskCompletionSource<Task>();
            WhenAnyCoordinator coordinator = new WhenAnyCoordinator(source);
            for (int i = 0; i < copy.Length; i++)
            {
                Task observedTask = copy[i];
                observedTask.RegisterContinuation(() => coordinator.Complete(observedTask));
            }

            return source.Task;
        }

        public static Task<Task<TResult>> WhenAny<TResult>(params Task<TResult>[] tasks)
        {
            Task<TResult>[] copy = ValidateTaskArray(tasks, false);
            TaskCompletionSource<Task<TResult>> source = new TaskCompletionSource<Task<TResult>>();
            WhenAnyCoordinator<TResult> coordinator = new WhenAnyCoordinator<TResult>(source);
            for (int i = 0; i < copy.Length; i++)
            {
                Task<TResult> observedTask = copy[i];
                observedTask.RegisterContinuation(() => coordinator.Complete(observedTask));
            }

            return source.Task;
        }

        public static int WaitAny(params Task[] tasks)
        {
            return WaitAny(tasks, -1);
        }

        public static int WaitAny(Task[] tasks, TimeSpan timeout)
        {
            return WaitAny(tasks, GetMillisecondsTimeout(timeout, "timeout"));
        }

        public static int WaitAny(Task[] tasks, CancellationToken cancellationToken)
        {
            return WaitAny(tasks, -1, cancellationToken);
        }

        public static int WaitAny(Task[] tasks, int millisecondsTimeout)
        {
            return WaitAny(tasks, millisecondsTimeout, CancellationToken.None);
        }

        public static int WaitAny(Task[] tasks, int millisecondsTimeout, CancellationToken cancellationToken)
        {
            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsTimeout");
            }

            Task[] copy = ValidateTaskArray(tasks, false);
            Task<Task> completion = WhenAny(copy);
            if (!completion.Wait(millisecondsTimeout, cancellationToken))
            {
                return -1;
            }

            Task completedTask = completion.Result;
            for (int i = 0; i < copy.Length; i++)
            {
                if (object.ReferenceEquals(copy[i], completedTask))
                {
                    return i;
                }
            }

            throw new InvalidOperationException("The completed task was not present in the input array.");
        }

        internal static Task Unwrap(Task<Task> task)
        {
            if (task == null)
            {
                throw new ArgumentNullException("task");
            }

            TaskCompletionSource<object> source = new TaskCompletionSource<object>();
            task.RegisterContinuation(() =>
            {
                if (!TryPropagateUnwrapFailure(task, source))
                {
                    Task inner = task.GetResultValue();
                    if (inner == null)
                    {
                        source.TrySetException(new InvalidOperationException("The asynchronous delegate returned a null task."));
                    }
                    else
                    {
                        inner.RegisterContinuation(() => PropagateCompletion(inner, source));
                    }
                }
            });
            return source.Task;
        }

        internal static Task<TResult> Unwrap<TResult>(Task<Task<TResult>> task)
        {
            if (task == null)
            {
                throw new ArgumentNullException("task");
            }

            TaskCompletionSource<TResult> source = new TaskCompletionSource<TResult>();
            task.RegisterContinuation(() =>
            {
                if (!TryPropagateUnwrapFailure(task, source))
                {
                    Task<TResult> inner = task.GetResultValue();
                    if (inner == null)
                    {
                        source.TrySetException(new InvalidOperationException("The asynchronous delegate returned a null task."));
                    }
                    else
                    {
                        inner.RegisterContinuation(() => PropagateCompletion(inner, source));
                    }
                }
            });
            return source.Task;
        }

        private static bool TryPropagateUnwrapFailure<TResult>(Task task, TaskCompletionSource<TResult> source)
        {
            if (task._exceptions != null)
            {
                source.TrySetException(task._exceptions);
                return true;
            }
            if (task.IsCanceled)
            {
                source.TrySetCanceled(task._cancellationToken);
                return true;
            }

            return false;
        }

        private static void PropagateCompletion(Task task, TaskCompletionSource<object> source)
        {
            if (!TryPropagateUnwrapFailure(task, source))
            {
                source.TrySetResult(null);
            }
        }

        private static void PropagateCompletion<TResult>(Task<TResult> task, TaskCompletionSource<TResult> source)
        {
            if (!TryPropagateUnwrapFailure(task, source))
            {
                source.TrySetResult(task.GetResultValue());
            }
        }

        public static YieldAwaitable Yield()
        {
            return new YieldAwaitable();
        }

        public void Dispose()
        {
            if (!IsCompleted)
            {
                throw new InvalidOperationException("A task may only be disposed after it has completed.");
            }

            _disposed = true;
        }

        internal void GetResultForAwaiter()
        {
            WaitWithoutThrow(-1);
            ThrowIfExceptional(false);
        }

        internal bool TrySetResult()
        {
            return Finish(TaskStatus.RanToCompletion, null, CancellationToken.None, null);
        }

        internal bool TrySetException(Exception exception)
        {
            if (exception == null)
            {
                throw new ArgumentNullException("exception");
            }

            return TrySetExceptions(new Exception[] { exception });
        }

        internal bool TrySetExceptions(Exception[] exceptions)
        {
            ValidateExceptions(exceptions);
            return Finish(TaskStatus.Faulted, exceptions, CancellationToken.None, null);
        }

        internal bool TrySetCanceled(CancellationToken cancellationToken)
        {
            return Finish(TaskStatus.Canceled, null, cancellationToken, null);
        }

        protected virtual void Execute()
        {
            _action();
        }

        private void ExecuteEntry()
        {
            if (_cancellationToken.IsCancellationRequested)
            {
                Finish(TaskStatus.Canceled, null, _cancellationToken, null);
                return;
            }

            _status = TaskStatus.Running;
            try
            {
                Execute();
                Finish(TaskStatus.RanToCompletion, null, CancellationToken.None, null);
            }
            catch (OperationCanceledException exception)
            {
                CancellationToken token = exception.CancellationToken.CanBeCanceled ? exception.CancellationToken : _cancellationToken;
                Finish(TaskStatus.Canceled, null, token, null);
            }
            catch (Exception exception)
            {
                Finish(TaskStatus.Faulted, new Exception[] { exception }, CancellationToken.None, null);
            }
        }

        protected bool Finish(TaskStatus finalStatus, Exception[] exceptions, CancellationToken cancellationToken, Action beforeCompletion)
        {
            Action[] continuations;

            Monitor.Enter(_gate);
            try
            {
                if (IsFinalStatus(_status))
                {
                    return false;
                }

                if (beforeCompletion != null)
                {
                    beforeCompletion();
                }

                _exceptions = exceptions;
                if (finalStatus == TaskStatus.Canceled)
                {
                    _cancellationToken = cancellationToken;
                }
                _status = finalStatus;
                continuations = _continuations == null ? new Action[0] : _continuations.ToArray();
                _continuations = null;
                Monitor.PulseAll(_gate);
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            for (int i = 0; i < continuations.Length; i++)
            {
                continuations[i]();
            }

            return true;
        }

        private void RegisterContinuation(Action continuation)
        {
            bool runNow;

            Monitor.Enter(_gate);
            try
            {
                runNow = IsFinalStatus(_status);
                if (!runNow)
                {
                    if (_continuations == null)
                    {
                        _continuations = new List<Action>();
                    }

                    _continuations.Add(continuation);
                }
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            if (runNow)
            {
                continuation();
            }
        }

        private bool WaitWithoutThrow(int millisecondsTimeout)
        {
            return WaitWithoutThrow(millisecondsTimeout, CancellationToken.None);
        }

        private bool WaitWithoutThrow(int millisecondsTimeout, CancellationToken cancellationToken)
        {
            int start = Environment.TickCount;
            CancellationTokenRegistration registration = cancellationToken.CanBeCanceled
                ? cancellationToken.Register(PulseTaskWaiters, this)
                : default(CancellationTokenRegistration);

            try
            {
                Monitor.Enter(_gate);
                try
                {
                    while (!IsFinalStatus(_status))
                    {
                        cancellationToken.ThrowIfCancellationRequested();

                        int remaining = RemainingTimeout(start, millisecondsTimeout);
                        if (remaining == 0 || !Monitor.Wait(_gate, remaining))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                finally
                {
                    Monitor.Exit(_gate);
                }
            }
            finally
            {
                registration.Dispose();
            }
        }

        private static void PulseTaskWaiters(object state)
        {
            Task task = (Task)state;
            Monitor.Enter(task._gate);
            try
            {
                Monitor.PulseAll(task._gate);
            }
            finally
            {
                Monitor.Exit(task._gate);
            }
        }

        private void ThrowIfExceptional(bool aggregate)
        {
            if (_status == TaskStatus.Faulted)
            {
                if (aggregate)
                {
                    throw new AggregateException(_exceptions);
                }

                throw _exceptions[0];
            }
            if (_status == TaskStatus.Canceled)
            {
                TaskCanceledException exception = new TaskCanceledException(this);
                if (aggregate)
                {
                    throw new AggregateException(exception);
                }

                throw exception;
            }
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
            {
                throw new ObjectDisposedException("Task");
            }
        }

        private static bool IsFinalStatus(TaskStatus status)
        {
            return status == TaskStatus.RanToCompletion || status == TaskStatus.Canceled || status == TaskStatus.Faulted;
        }

        private static int RemainingTimeout(int start, int millisecondsTimeout)
        {
            if (millisecondsTimeout < 0)
            {
                return -1;
            }

            uint elapsed = (uint)(Environment.TickCount - start);
            if (elapsed >= (uint)millisecondsTimeout)
            {
                return 0;
            }

            return millisecondsTimeout - (int)elapsed;
        }

        private static int GetMillisecondsTimeout(TimeSpan timeout, string parameterName)
        {
            double milliseconds = timeout.TotalMilliseconds;
            if (milliseconds < -1 || milliseconds > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException(parameterName);
            }

            return (int)milliseconds;
        }

        private static Action CreateAction(Action<object> action, object state)
        {
            if (action == null)
            {
                throw new ArgumentNullException("action");
            }

            return () => action(state);
        }

        private static void ValidateExceptions(Exception[] exceptions)
        {
            if (exceptions == null)
            {
                throw new ArgumentNullException("exceptions");
            }
            if (exceptions.Length == 0)
            {
                throw new ArgumentException("The exceptions array must contain at least one exception.");
            }

            for (int i = 0; i < exceptions.Length; i++)
            {
                if (exceptions[i] == null)
                {
                    throw new ArgumentException("The exceptions array contains a null exception.");
                }
            }
        }

        private static Task[] ValidateTaskArray(Task[] tasks, bool allowEmpty)
        {
            if (tasks == null)
            {
                throw new ArgumentNullException("tasks");
            }
            if (!allowEmpty && tasks.Length == 0)
            {
                throw new ArgumentException("The tasks array must contain at least one task.");
            }

            Task[] copy = new Task[tasks.Length];
            for (int i = 0; i < tasks.Length; i++)
            {
                if (tasks[i] == null)
                {
                    throw new ArgumentException("The tasks array contains a null task.");
                }

                copy[i] = tasks[i];
            }

            return copy;
        }

        private static Task<TResult>[] ValidateTaskArray<TResult>(Task<TResult>[] tasks, bool allowEmpty)
        {
            if (tasks == null)
            {
                throw new ArgumentNullException("tasks");
            }
            if (!allowEmpty && tasks.Length == 0)
            {
                throw new ArgumentException("The tasks array must contain at least one task.");
            }

            Task<TResult>[] copy = new Task<TResult>[tasks.Length];
            for (int i = 0; i < tasks.Length; i++)
            {
                if (tasks[i] == null)
                {
                    throw new ArgumentException("The tasks array contains a null task.");
                }

                copy[i] = tasks[i];
            }

            return copy;
        }

        private static Exception[] FlattenExceptions(Exception[][] taskExceptions)
        {
            int exceptionCount = 0;
            for (int taskIndex = 0; taskIndex < taskExceptions.Length; taskIndex++)
            {
                if (taskExceptions[taskIndex] != null)
                {
                    exceptionCount += taskExceptions[taskIndex].Length;
                }
            }

            if (exceptionCount == 0)
            {
                return null;
            }

            Exception[] exceptions = new Exception[exceptionCount];
            int destinationIndex = 0;
            for (int taskIndex = 0; taskIndex < taskExceptions.Length; taskIndex++)
            {
                Exception[] current = taskExceptions[taskIndex];
                if (current == null)
                {
                    continue;
                }

                for (int exceptionIndex = 0; exceptionIndex < current.Length; exceptionIndex++)
                {
                    exceptions[destinationIndex++] = current[exceptionIndex];
                }
            }

            return exceptions;
        }

        private sealed class WhenAllCoordinator
        {
            private readonly object _gate = new object();
            private readonly TaskCompletionSource<object> _source;
            private readonly Exception[][] _taskExceptions;
            private int _remaining;
            private bool _canceled;
            private CancellationToken _cancellationToken;

            internal WhenAllCoordinator(int taskCount, TaskCompletionSource<object> source)
            {
                _remaining = taskCount;
                _source = source;
                _taskExceptions = new Exception[taskCount][];
            }

            internal void Complete(Task task, int taskIndex)
            {
                Exception[] exceptions = null;
                bool canceled = false;
                bool completed = false;
                CancellationToken cancellationToken = CancellationToken.None;

                Monitor.Enter(_gate);
                try
                {
                    if (task._exceptions != null)
                    {
                        _taskExceptions[taskIndex] = task._exceptions;
                    }
                    else if (task.IsCanceled)
                    {
                        _canceled = true;
                        if (!_cancellationToken.CanBeCanceled)
                        {
                            _cancellationToken = task._cancellationToken;
                        }
                    }

                    _remaining--;
                    if (_remaining == 0)
                    {
                        exceptions = FlattenExceptions(_taskExceptions);
                        canceled = _canceled;
                        cancellationToken = _cancellationToken;
                        completed = true;
                    }
                }
                finally
                {
                    Monitor.Exit(_gate);
                }

                if (completed)
                {
                    if (exceptions != null)
                    {
                        _source.TrySetException(exceptions);
                    }
                    else if (canceled)
                    {
                        _source.TrySetCanceled(cancellationToken);
                    }
                    else
                    {
                        _source.TrySetResult(null);
                    }
                }
            }
        }

        private sealed class WhenAllCoordinator<TResult>
        {
            private readonly object _gate = new object();
            private readonly TaskCompletionSource<TResult[]> _source;
            private readonly TResult[] _results;
            private readonly Exception[][] _taskExceptions;
            private int _remaining;
            private bool _canceled;
            private CancellationToken _cancellationToken;

            internal WhenAllCoordinator(Task<TResult>[] tasks, TaskCompletionSource<TResult[]> source)
            {
                _remaining = tasks.Length;
                _results = new TResult[tasks.Length];
                _taskExceptions = new Exception[tasks.Length][];
                _source = source;
            }

            internal void Complete(Task<TResult> task, int resultIndex)
            {
                Exception[] exceptions = null;
                bool canceled = false;
                bool completed = false;
                CancellationToken cancellationToken = CancellationToken.None;

                Monitor.Enter(_gate);
                try
                {
                    if (task._exceptions != null)
                    {
                        _taskExceptions[resultIndex] = task._exceptions;
                    }
                    else if (task.IsCanceled)
                    {
                        _canceled = true;
                        if (!_cancellationToken.CanBeCanceled)
                        {
                            _cancellationToken = task._cancellationToken;
                        }
                    }
                    else
                    {
                        _results[resultIndex] = task.GetResultValue();
                    }

                    _remaining--;
                    if (_remaining == 0)
                    {
                        exceptions = FlattenExceptions(_taskExceptions);
                        canceled = _canceled;
                        cancellationToken = _cancellationToken;
                        completed = true;
                    }
                }
                finally
                {
                    Monitor.Exit(_gate);
                }

                if (completed)
                {
                    if (exceptions != null)
                    {
                        _source.TrySetException(exceptions);
                    }
                    else if (canceled)
                    {
                        _source.TrySetCanceled(cancellationToken);
                    }
                    else
                    {
                        _source.TrySetResult(_results);
                    }
                }
            }
        }

        private sealed class WhenAnyCoordinator
        {
            private readonly object _gate = new object();
            private readonly TaskCompletionSource<Task> _source;
            private bool _completed;

            internal WhenAnyCoordinator(TaskCompletionSource<Task> source)
            {
                _source = source;
            }

            internal void Complete(Task task)
            {
                bool publish;

                Monitor.Enter(_gate);
                try
                {
                    publish = !_completed;
                    _completed = true;
                }
                finally
                {
                    Monitor.Exit(_gate);
                }

                if (publish)
                {
                    _source.TrySetResult(task);
                }
            }
        }

        private sealed class WhenAnyCoordinator<TResult>
        {
            private readonly object _gate = new object();
            private readonly TaskCompletionSource<Task<TResult>> _source;
            private bool _completed;

            internal WhenAnyCoordinator(TaskCompletionSource<Task<TResult>> source)
            {
                _source = source;
            }

            internal void Complete(Task<TResult> task)
            {
                bool publish;

                Monitor.Enter(_gate);
                try
                {
                    publish = !_completed;
                    _completed = true;
                }
                finally
                {
                    Monitor.Exit(_gate);
                }

                if (publish)
                {
                    _source.TrySetResult(task);
                }
            }
        }

        private static Task CreateCompletedTask()
        {
            Task task = new Task(true);
            task.TrySetResult();
            return task;
        }
    }

    public class Task<TResult> : Task
    {
        private readonly Func<TResult> _function;
        private TResult _result;

        public Task(Func<TResult> function) : this(function, CancellationToken.None)
        {
        }

        public Task(Func<TResult> function, CancellationToken cancellationToken) : base(() => { }, cancellationToken)
        {
            if (function == null)
            {
                throw new ArgumentNullException("function");
            }

            _function = function;
        }

        internal Task(bool promiseStyle) : this(promiseStyle, null)
        {
        }

        internal Task(bool promiseStyle, object asyncState) : base(promiseStyle, asyncState)
        {
        }

        public TResult Result
        {
            get
            {
                GetResultForAwaiter();
                return _result;
            }
        }

        public new TaskAwaiter<TResult> GetAwaiter()
        {
            return new TaskAwaiter<TResult>(this);
        }

        public new ConfiguredTaskAwaitable<TResult> ConfigureAwait(bool continueOnCapturedContext)
        {
            return new ConfiguredTaskAwaitable<TResult>(this, continueOnCapturedContext);
        }

        public Task ContinueWith(Action<Task<TResult>> continuationAction)
        {
            if (continuationAction == null)
            {
                throw new ArgumentNullException("continuationAction");
            }

            return base.ContinueWith(ignored => continuationAction(this));
        }

        public Task<TNewResult> ContinueWith<TNewResult>(Func<Task<TResult>, TNewResult> continuationFunction)
        {
            if (continuationFunction == null)
            {
                throw new ArgumentNullException("continuationFunction");
            }

            return base.ContinueWith(ignored => continuationFunction(this));
        }

        internal bool TrySetResult(TResult result)
        {
            return Finish(TaskStatus.RanToCompletion, null, CancellationToken.None, () => _result = result);
        }

        internal TResult GetResultValue()
        {
            GetResultForAwaiter();
            return _result;
        }

        protected override void Execute()
        {
            _result = _function();
        }
    }

    public class TaskCompletionSource<TResult>
    {
        private readonly Task<TResult> _task;

        public TaskCompletionSource()
        {
            _task = new Task<TResult>(true);
        }

        public TaskCompletionSource(object state)
        {
            _task = new Task<TResult>(true, state);
        }

        public TaskCompletionSource(TaskCreationOptions creationOptions)
        {
            ValidateCreationOptions(creationOptions);
            _task = new Task<TResult>(true);
        }

        public TaskCompletionSource(object state, TaskCreationOptions creationOptions)
        {
            ValidateCreationOptions(creationOptions);
            _task = new Task<TResult>(true, state);
        }

        public Task<TResult> Task => _task;

        public void SetResult(TResult result)
        {
            if (!TrySetResult(result))
            {
                throw new InvalidOperationException("The task is already in a final state.");
            }
        }

        public bool TrySetResult(TResult result)
        {
            return _task.TrySetResult(result);
        }

        public void SetException(Exception exception)
        {
            if (!TrySetException(exception))
            {
                throw new InvalidOperationException("The task is already in a final state.");
            }
        }

        public bool TrySetException(Exception exception)
        {
            return _task.TrySetException(exception);
        }

        public void SetException(IEnumerable<Exception> exceptions)
        {
            if (!TrySetException(exceptions))
            {
                throw new InvalidOperationException("The task is already in a final state.");
            }
        }

        public bool TrySetException(IEnumerable<Exception> exceptions)
        {
            return _task.TrySetExceptions(CopyExceptions(exceptions));
        }

        public void SetCanceled()
        {
            SetCanceled(CancellationToken.None);
        }

        public void SetCanceled(CancellationToken cancellationToken)
        {
            if (!TrySetCanceled(cancellationToken))
            {
                throw new InvalidOperationException("The task is already in a final state.");
            }
        }

        public bool TrySetCanceled()
        {
            return TrySetCanceled(CancellationToken.None);
        }

        public bool TrySetCanceled(CancellationToken cancellationToken)
        {
            return _task.TrySetCanceled(cancellationToken);
        }

        private static void ValidateCreationOptions(TaskCreationOptions creationOptions)
        {
            if (creationOptions != TaskCreationOptions.None && creationOptions != TaskCreationOptions.RunContinuationsAsynchronously)
            {
                throw new ArgumentOutOfRangeException("creationOptions");
            }
        }

        private static Exception[] CopyExceptions(IEnumerable<Exception> exceptions)
        {
            if (exceptions == null)
            {
                throw new ArgumentNullException("exceptions");
            }

            List<Exception> copy = new List<Exception>();
            foreach (Exception exception in exceptions)
            {
                if (exception == null)
                {
                    throw new ArgumentException("The exceptions collection contains a null exception.");
                }

                copy.Add(exception);
            }

            if (copy.Count == 0)
            {
                throw new ArgumentException("The exceptions collection must contain at least one exception.");
            }

            return copy.ToArray();
        }
    }

    public static class TaskExtensions
    {
        public static Task Unwrap(this Task<Task> task)
        {
            return Task.Unwrap(task);
        }

        public static Task<TResult> Unwrap<TResult>(this Task<Task<TResult>> task)
        {
            return Task.Unwrap(task);
        }
    }
}
