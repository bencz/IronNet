using System.Threading;
using System.Threading.Tasks;

namespace System.Runtime.CompilerServices
{
    public interface INotifyCompletion
    {
        void OnCompleted(Action continuation);
    }

    public interface ICriticalNotifyCompletion : INotifyCompletion
    {
        void UnsafeOnCompleted(Action continuation);
    }

    public interface IAsyncStateMachine
    {
        void MoveNext();
        void SetStateMachine(IAsyncStateMachine stateMachine);
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public sealed class AsyncStateMachineAttribute : Attribute
    {
        public AsyncStateMachineAttribute(Type stateMachineType)
        {
            StateMachineType = stateMachineType;
        }

        public Type StateMachineType { get; }
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public sealed class IteratorStateMachineAttribute : Attribute
    {
        public IteratorStateMachineAttribute(Type stateMachineType)
        {
            StateMachineType = stateMachineType;
        }

        public Type StateMachineType { get; }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate, Inherited = false)]
    public sealed class AsyncMethodBuilderAttribute : Attribute
    {
        public AsyncMethodBuilderAttribute(Type builderType)
        {
            BuilderType = builderType ?? throw new ArgumentNullException("builderType");
        }

        public Type BuilderType { get; }
    }

    public struct TaskAwaiter : ICriticalNotifyCompletion
    {
        private readonly Task _task;

        internal TaskAwaiter(Task task)
        {
            _task = task;
        }

        public bool IsCompleted => _task.IsCompleted;

        public void GetResult()
        {
            _task.GetResultForAwaiter();
        }

        public void OnCompleted(Action continuation)
        {
            _task.RegisterAwaitContinuation(continuation, true);
        }

        public void UnsafeOnCompleted(Action continuation)
        {
            OnCompleted(continuation);
        }
    }

    public struct TaskAwaiter<TResult> : ICriticalNotifyCompletion
    {
        private readonly Task<TResult> _task;

        internal TaskAwaiter(Task<TResult> task)
        {
            _task = task;
        }

        public bool IsCompleted => _task.IsCompleted;

        public TResult GetResult()
        {
            return _task.GetResultValue();
        }

        public void OnCompleted(Action continuation)
        {
            if (continuation == null)
            {
                throw new ArgumentNullException("continuation");
            }

            _task.RegisterAwaitContinuation(continuation, true);
        }

        public void UnsafeOnCompleted(Action continuation)
        {
            OnCompleted(continuation);
        }
    }

    public struct ConfiguredTaskAwaitable
    {
        private readonly Task _task;
        private readonly bool _continueOnCapturedContext;

        internal ConfiguredTaskAwaitable(Task task, bool continueOnCapturedContext)
        {
            _task = task;
            _continueOnCapturedContext = continueOnCapturedContext;
        }

        public ConfiguredTaskAwaiter GetAwaiter()
        {
            return new ConfiguredTaskAwaiter(_task, _continueOnCapturedContext);
        }

        public struct ConfiguredTaskAwaiter : ICriticalNotifyCompletion
        {
            private readonly Task _task;
            private readonly bool _continueOnCapturedContext;

            internal ConfiguredTaskAwaiter(Task task, bool continueOnCapturedContext)
            {
                _task = task;
                _continueOnCapturedContext = continueOnCapturedContext;
            }

            public bool IsCompleted => _task.IsCompleted;

            public void GetResult()
            {
                _task.GetResultForAwaiter();
            }

            public void OnCompleted(Action continuation)
            {
                _task.RegisterAwaitContinuation(continuation, _continueOnCapturedContext);
            }

            public void UnsafeOnCompleted(Action continuation)
            {
                _task.RegisterAwaitContinuation(continuation, _continueOnCapturedContext);
            }
        }
    }

    public struct ConfiguredTaskAwaitable<TResult>
    {
        private readonly Task<TResult> _task;
        private readonly bool _continueOnCapturedContext;

        internal ConfiguredTaskAwaitable(Task<TResult> task, bool continueOnCapturedContext)
        {
            _task = task;
            _continueOnCapturedContext = continueOnCapturedContext;
        }

        public ConfiguredTaskAwaiter GetAwaiter()
        {
            return new ConfiguredTaskAwaiter(_task, _continueOnCapturedContext);
        }

        public struct ConfiguredTaskAwaiter : ICriticalNotifyCompletion
        {
            private readonly Task<TResult> _task;
            private readonly bool _continueOnCapturedContext;

            internal ConfiguredTaskAwaiter(Task<TResult> task, bool continueOnCapturedContext)
            {
                _task = task;
                _continueOnCapturedContext = continueOnCapturedContext;
            }

            public bool IsCompleted => _task.IsCompleted;

            public TResult GetResult()
            {
                return _task.GetResultValue();
            }

            public void OnCompleted(Action continuation)
            {
                _task.RegisterAwaitContinuation(continuation, _continueOnCapturedContext);
            }

            public void UnsafeOnCompleted(Action continuation)
            {
                _task.RegisterAwaitContinuation(continuation, _continueOnCapturedContext);
            }
        }
    }

    public struct YieldAwaitable
    {
        public YieldAwaiter GetAwaiter()
        {
            return new YieldAwaiter();
        }

        public struct YieldAwaiter : ICriticalNotifyCompletion
        {
            public bool IsCompleted => false;

            public void GetResult()
            {
            }

            public void OnCompleted(Action continuation)
            {
                if (continuation == null)
                {
                    throw new ArgumentNullException("continuation");
                }

                new AsyncContinuation(continuation, true).Schedule();
            }

            public void UnsafeOnCompleted(Action continuation)
            {
                OnCompleted(continuation);
            }
        }
    }

    public struct AsyncTaskMethodBuilder
    {
        private TaskCompletionSource<object> _source;

        public static AsyncTaskMethodBuilder Create()
        {
            AsyncTaskMethodBuilder builder = new AsyncTaskMethodBuilder();
            builder._source = new TaskCompletionSource<object>();
            return builder;
        }

        public Task Task => _source.Task;

        public void SetResult()
        {
            _source.SetResult(null);
        }

        public void SetException(Exception exception)
        {
            OperationCanceledException cancellation = exception as OperationCanceledException;
            if (cancellation != null)
            {
                _source.SetCanceled(cancellation.CancellationToken);
            }
            else
            {
                _source.SetException(exception);
            }
        }

        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            if (stateMachine == null)
            {
                throw new ArgumentNullException("stateMachine");
            }
        }

        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            AsyncContinuation.Start(ref stateMachine);
        }

        public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            IAsyncStateMachine boxedStateMachine = stateMachine;
            awaiter.OnCompleted(boxedStateMachine.MoveNext);
        }

        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : ICriticalNotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            IAsyncStateMachine boxedStateMachine = stateMachine;
            awaiter.UnsafeOnCompleted(boxedStateMachine.MoveNext);
        }
    }

    public struct AsyncTaskMethodBuilder<TResult>
    {
        private TaskCompletionSource<TResult> _source;

        public static AsyncTaskMethodBuilder<TResult> Create()
        {
            AsyncTaskMethodBuilder<TResult> builder = new AsyncTaskMethodBuilder<TResult>();
            builder._source = new TaskCompletionSource<TResult>();
            return builder;
        }

        public Task<TResult> Task => _source.Task;

        public void SetResult(TResult result)
        {
            _source.SetResult(result);
        }

        public void SetException(Exception exception)
        {
            OperationCanceledException cancellation = exception as OperationCanceledException;
            if (cancellation != null)
            {
                _source.SetCanceled(cancellation.CancellationToken);
            }
            else
            {
                _source.SetException(exception);
            }
        }

        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            if (stateMachine == null)
            {
                throw new ArgumentNullException("stateMachine");
            }
        }

        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            AsyncContinuation.Start(ref stateMachine);
        }

        public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            IAsyncStateMachine boxedStateMachine = stateMachine;
            awaiter.OnCompleted(boxedStateMachine.MoveNext);
        }

        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : ICriticalNotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            IAsyncStateMachine boxedStateMachine = stateMachine;
            awaiter.UnsafeOnCompleted(boxedStateMachine.MoveNext);
        }
    }

    public struct AsyncValueTaskMethodBuilder
    {
        private AsyncTaskMethodBuilder _builder;

        public static AsyncValueTaskMethodBuilder Create()
        {
            AsyncValueTaskMethodBuilder builder = new AsyncValueTaskMethodBuilder();
            builder._builder = AsyncTaskMethodBuilder.Create();
            return builder;
        }

        public ValueTask Task => new ValueTask(_builder.Task);

        public void SetResult()
        {
            _builder.SetResult();
        }

        public void SetException(Exception exception)
        {
            _builder.SetException(exception);
        }

        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            _builder.SetStateMachine(stateMachine);
        }

        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            _builder.Start(ref stateMachine);
        }

        public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            _builder.AwaitOnCompleted(ref awaiter, ref stateMachine);
        }

        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : ICriticalNotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            _builder.AwaitUnsafeOnCompleted(ref awaiter, ref stateMachine);
        }
    }

    public struct AsyncValueTaskMethodBuilder<TResult>
    {
        private AsyncTaskMethodBuilder<TResult> _builder;

        public static AsyncValueTaskMethodBuilder<TResult> Create()
        {
            AsyncValueTaskMethodBuilder<TResult> builder = new AsyncValueTaskMethodBuilder<TResult>();
            builder._builder = AsyncTaskMethodBuilder<TResult>.Create();
            return builder;
        }

        public ValueTask<TResult> Task => new ValueTask<TResult>(_builder.Task);

        public void SetResult(TResult result)
        {
            _builder.SetResult(result);
        }

        public void SetException(Exception exception)
        {
            _builder.SetException(exception);
        }

        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            _builder.SetStateMachine(stateMachine);
        }

        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            _builder.Start(ref stateMachine);
        }

        public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            _builder.AwaitOnCompleted(ref awaiter, ref stateMachine);
        }

        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : ICriticalNotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            _builder.AwaitUnsafeOnCompleted(ref awaiter, ref stateMachine);
        }
    }

    public struct ValueTaskAwaiter : ICriticalNotifyCompletion
    {
        private readonly ValueTask _value;

        internal ValueTaskAwaiter(ValueTask value)
        {
            _value = value;
        }

        public bool IsCompleted => _value.IsCompleted;

        public void GetResult()
        {
            _value.GetResultForAwaiter();
        }

        public void OnCompleted(Action continuation)
        {
            _value.OnCompleted(continuation, true);
        }

        public void UnsafeOnCompleted(Action continuation)
        {
            _value.OnCompleted(continuation, true);
        }
    }

    public struct ValueTaskAwaiter<TResult> : ICriticalNotifyCompletion
    {
        private readonly ValueTask<TResult> _value;

        internal ValueTaskAwaiter(ValueTask<TResult> value)
        {
            _value = value;
        }

        public bool IsCompleted => _value.IsCompleted;

        public TResult GetResult()
        {
            return _value.GetResultForAwaiter();
        }

        public void OnCompleted(Action continuation)
        {
            _value.OnCompleted(continuation, true);
        }

        public void UnsafeOnCompleted(Action continuation)
        {
            _value.OnCompleted(continuation, true);
        }
    }

    public struct AsyncVoidMethodBuilder
    {
        private SynchronizationContext _context;

        public static AsyncVoidMethodBuilder Create()
        {
            AsyncVoidMethodBuilder builder = new AsyncVoidMethodBuilder();
            builder._context = SynchronizationContext.Current;
            if (builder._context != null)
            {
                builder._context.OperationStarted();
            }

            return builder;
        }

        public void SetResult()
        {
            NotifyCompletion();
        }

        private void NotifyCompletion()
        {
            if (_context != null)
            {
                try
                {
                    _context.OperationCompleted();
                }
                catch (Exception exception)
                {
                    AsyncContinuation.ThrowAsync(exception, null);
                }
            }
        }

        public void SetException(Exception exception)
        {
            if (exception == null)
            {
                throw new ArgumentNullException("exception");
            }

            try
            {
                AsyncContinuation.ThrowAsync(exception, _context);
            }
            finally
            {
                NotifyCompletion();
            }
        }

        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            if (stateMachine == null)
            {
                throw new ArgumentNullException("stateMachine");
            }
        }

        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            AsyncContinuation.Start(ref stateMachine);
        }

        public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            IAsyncStateMachine boxedStateMachine = stateMachine;
            awaiter.OnCompleted(boxedStateMachine.MoveNext);
        }

        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : ICriticalNotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            IAsyncStateMachine boxedStateMachine = stateMachine;
            awaiter.UnsafeOnCompleted(boxedStateMachine.MoveNext);
        }
    }
}
