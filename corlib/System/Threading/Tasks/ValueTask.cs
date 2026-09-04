using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;

namespace System.Threading.Tasks
{
    [AsyncMethodBuilder(typeof(AsyncValueTaskMethodBuilder))]
    public struct ValueTask : IEquatable<ValueTask>
    {
        private readonly Task _task;

        public ValueTask(Task task)
        {
            _task = task ?? throw new ArgumentNullException("task");
        }

        public static ValueTask CompletedTask => default(ValueTask);
        public bool IsCompleted => _task == null || _task.IsCompleted;
        public bool IsCompletedSuccessfully => _task == null || _task.IsCompletedSuccessfully;
        public bool IsCanceled => _task != null && _task.IsCanceled;
        public bool IsFaulted => _task != null && _task.IsFaulted;

        public Task AsTask()
        {
            return _task ?? Task.CompletedTask;
        }

        public ValueTask Preserve()
        {
            return this;
        }

        public ValueTaskAwaiter GetAwaiter()
        {
            return new ValueTaskAwaiter(this);
        }

        public ConfiguredValueTaskAwaitable ConfigureAwait(bool continueOnCapturedContext)
        {
            return new ConfiguredValueTaskAwaitable(this, continueOnCapturedContext);
        }

        public bool Equals(ValueTask other)
        {
            return object.ReferenceEquals(_task, other._task);
        }

        public override bool Equals(object obj)
        {
            return obj is ValueTask && Equals((ValueTask)obj);
        }

        public override int GetHashCode()
        {
            return _task == null ? 0 : _task.GetHashCode();
        }

        public static bool operator ==(ValueTask left, ValueTask right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(ValueTask left, ValueTask right)
        {
            return !left.Equals(right);
        }

        internal void GetResultForAwaiter()
        {
            if (_task != null)
            {
                _task.GetResultForAwaiter();
            }
        }

        internal void OnCompleted(Action continuation, bool continueOnCapturedContext)
        {
            if (continuation == null)
            {
                throw new ArgumentNullException("continuation");
            }

            AsTask().RegisterAwaitContinuation(continuation, continueOnCapturedContext);
        }
    }

    [AsyncMethodBuilder(typeof(AsyncValueTaskMethodBuilder<>))]
    public struct ValueTask<TResult> : IEquatable<ValueTask<TResult>>
    {
        private readonly Task<TResult> _task;
        private readonly TResult _result;

        public ValueTask(TResult result)
        {
            _task = null;
            _result = result;
        }

        public ValueTask(Task<TResult> task)
        {
            _task = task ?? throw new ArgumentNullException("task");
            _result = default(TResult);
        }

        public bool IsCompleted => _task == null || _task.IsCompleted;
        public bool IsCompletedSuccessfully => _task == null || _task.IsCompletedSuccessfully;
        public bool IsCanceled => _task != null && _task.IsCanceled;
        public bool IsFaulted => _task != null && _task.IsFaulted;
        public TResult Result => GetResultForAwaiter();

        public Task<TResult> AsTask()
        {
            return _task ?? Task.FromResult(_result);
        }

        public ValueTask<TResult> Preserve()
        {
            return this;
        }

        public ValueTaskAwaiter<TResult> GetAwaiter()
        {
            return new ValueTaskAwaiter<TResult>(this);
        }

        public ConfiguredValueTaskAwaitable<TResult> ConfigureAwait(bool continueOnCapturedContext)
        {
            return new ConfiguredValueTaskAwaitable<TResult>(this, continueOnCapturedContext);
        }

        public bool Equals(ValueTask<TResult> other)
        {
            if (_task != null || other._task != null)
            {
                return object.ReferenceEquals(_task, other._task);
            }

            return EqualityComparer<TResult>.Default.Equals(_result, other._result);
        }

        public override bool Equals(object obj)
        {
            return obj is ValueTask<TResult> && Equals((ValueTask<TResult>)obj);
        }

        public override int GetHashCode()
        {
            if (_task != null)
            {
                return _task.GetHashCode();
            }

            return _result == null ? 0 : EqualityComparer<TResult>.Default.GetHashCode(_result);
        }

        public static bool operator ==(ValueTask<TResult> left, ValueTask<TResult> right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(ValueTask<TResult> left, ValueTask<TResult> right)
        {
            return !left.Equals(right);
        }

        internal TResult GetResultForAwaiter()
        {
            return _task == null ? _result : _task.GetResultValue();
        }

        internal void OnCompleted(Action continuation, bool continueOnCapturedContext)
        {
            if (continuation == null)
            {
                throw new ArgumentNullException("continuation");
            }

            AsTask().RegisterAwaitContinuation(continuation, continueOnCapturedContext);
        }
    }

}

namespace System.Runtime.CompilerServices
{
    public struct ConfiguredValueTaskAwaitable
    {
        private readonly ValueTask _value;
        private readonly bool _continueOnCapturedContext;

        internal ConfiguredValueTaskAwaitable(ValueTask value, bool continueOnCapturedContext)
        {
            _value = value;
            _continueOnCapturedContext = continueOnCapturedContext;
        }

        public ConfiguredValueTaskAwaiter GetAwaiter()
        {
            return new ConfiguredValueTaskAwaiter(_value, _continueOnCapturedContext);
        }

        public struct ConfiguredValueTaskAwaiter : ICriticalNotifyCompletion
        {
            private readonly ValueTask _value;
            private readonly bool _continueOnCapturedContext;

            internal ConfiguredValueTaskAwaiter(ValueTask value, bool continueOnCapturedContext)
            {
                _value = value;
                _continueOnCapturedContext = continueOnCapturedContext;
            }

            public bool IsCompleted => _value.IsCompleted;

            public void GetResult()
            {
                _value.GetResultForAwaiter();
            }

            public void OnCompleted(Action continuation)
            {
                _value.OnCompleted(continuation, _continueOnCapturedContext);
            }

            public void UnsafeOnCompleted(Action continuation)
            {
                _value.OnCompleted(continuation, _continueOnCapturedContext);
            }
        }
    }

    public struct ConfiguredValueTaskAwaitable<TResult>
    {
        private readonly ValueTask<TResult> _value;
        private readonly bool _continueOnCapturedContext;

        internal ConfiguredValueTaskAwaitable(ValueTask<TResult> value, bool continueOnCapturedContext)
        {
            _value = value;
            _continueOnCapturedContext = continueOnCapturedContext;
        }

        public ConfiguredValueTaskAwaiter GetAwaiter()
        {
            return new ConfiguredValueTaskAwaiter(_value, _continueOnCapturedContext);
        }

        public struct ConfiguredValueTaskAwaiter : ICriticalNotifyCompletion
        {
            private readonly ValueTask<TResult> _value;
            private readonly bool _continueOnCapturedContext;

            internal ConfiguredValueTaskAwaiter(ValueTask<TResult> value, bool continueOnCapturedContext)
            {
                _value = value;
                _continueOnCapturedContext = continueOnCapturedContext;
            }

            public bool IsCompleted => _value.IsCompleted;

            public TResult GetResult()
            {
                return _value.GetResultForAwaiter();
            }

            public void OnCompleted(Action continuation)
            {
                _value.OnCompleted(continuation, _continueOnCapturedContext);
            }

            public void UnsafeOnCompleted(Action continuation)
            {
                _value.OnCompleted(continuation, _continueOnCapturedContext);
            }
        }
    }
}
