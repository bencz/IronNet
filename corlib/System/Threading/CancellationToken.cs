using System.Collections.Generic;

namespace System.Threading
{
    public struct CancellationToken : IEquatable<CancellationToken>
    {
        private readonly CancellationTokenSource _source;

        internal CancellationToken(CancellationTokenSource source)
        {
            _source = source;
        }

        public CancellationToken(bool canceled)
        {
            _source = canceled ? CancellationTokenSource.CanceledSource : null;
        }

        public static CancellationToken None => default(CancellationToken);
        public bool CanBeCanceled => _source != null;
        public bool IsCancellationRequested => _source != null && _source.IsCancellationRequested;
        public WaitHandle WaitHandle => _source == null ? CancellationTokenSource.NeverCanceledEvent : _source.WaitHandle;

        public CancellationTokenRegistration Register(Action callback)
        {
            if (callback == null)
            {
                throw new ArgumentNullException("callback");
            }

            return Register(
                state => ((Action)state)(),
                callback);
        }

        public CancellationTokenRegistration Register(Action<object> callback, object state)
        {
            if (callback == null)
            {
                throw new ArgumentNullException("callback");
            }
            if (_source == null)
            {
                return default(CancellationTokenRegistration);
            }

            return _source.Register(callback, state);
        }

        public void ThrowIfCancellationRequested()
        {
            if (IsCancellationRequested)
            {
                throw new OperationCanceledException(this);
            }
        }

        public bool Equals(CancellationToken other)
        {
            return object.ReferenceEquals(_source, other._source);
        }

        public override bool Equals(object obj)
        {
            return obj is CancellationToken && Equals((CancellationToken)obj);
        }

        public override int GetHashCode()
        {
            return _source == null ? 0 : _source.GetHashCode();
        }

        public static bool operator ==(CancellationToken left, CancellationToken right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(CancellationToken left, CancellationToken right)
        {
            return !left.Equals(right);
        }
    }

    public struct CancellationTokenRegistration : IDisposable, IEquatable<CancellationTokenRegistration>
    {
        private readonly CancellationTokenSource _source;
        private readonly int _id;

        internal CancellationTokenRegistration(CancellationTokenSource source, int id)
        {
            _source = source;
            _id = id;
        }

        public void Dispose()
        {
            if (_source != null)
            {
                _source.Unregister(_id);
            }
        }

        public bool Equals(CancellationTokenRegistration other)
        {
            return object.ReferenceEquals(_source, other._source) && _id == other._id;
        }

        public override bool Equals(object obj)
        {
            return obj is CancellationTokenRegistration && Equals((CancellationTokenRegistration)obj);
        }

        public override int GetHashCode()
        {
            return (_source == null ? 0 : _source.GetHashCode()) ^ _id;
        }

        public static bool operator ==(CancellationTokenRegistration left, CancellationTokenRegistration right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(CancellationTokenRegistration left, CancellationTokenRegistration right)
        {
            return !left.Equals(right);
        }
    }

    public class CancellationTokenSource : IDisposable
    {
        private sealed class CallbackRegistration
        {
            public int Id;
            public Action<object> Callback;
            public object State;
            public bool Active;
        }

        private readonly object _gate = new object();
        private readonly List<CallbackRegistration> _callbacks = new List<CallbackRegistration>();
        private List<CancellationTokenRegistration> _linkedRegistrations;
        private ManualResetEvent _event;
        private volatile bool _cancellationRequested;
        private bool _disposed;
        private int _nextRegistrationId;
        private int _cancelAfterVersion;

        internal static readonly ManualResetEvent NeverCanceledEvent = new ManualResetEvent(false);
        internal static readonly CancellationTokenSource CanceledSource = CreateCanceledSource();

        public CancellationTokenSource()
        {
        }

        public CancellationTokenSource(int millisecondsDelay)
        {
            CancelAfter(millisecondsDelay);
        }

        public CancellationTokenSource(TimeSpan delay)
        {
            CancelAfter(delay);
        }

        public bool IsCancellationRequested => _cancellationRequested;
        public CancellationToken Token
        {
            get
            {
                ThrowIfDisposed();
                return new CancellationToken(this);
            }
        }

        internal WaitHandle WaitHandle
        {
            get
            {
                ThrowIfDisposed();

                Monitor.Enter(_gate);
                try
                {
                    if (_event == null)
                    {
                        _event = new ManualResetEvent(_cancellationRequested);
                    }

                    return _event;
                }
                finally
                {
                    Monitor.Exit(_gate);
                }
            }
        }

        public void Cancel()
        {
            Cancel(false);
        }

        public void Cancel(bool throwOnFirstException)
        {
            CallbackRegistration[] callbacks;

            Monitor.Enter(_gate);
            try
            {
                ThrowIfDisposed();
                callbacks = BeginCancellation();
                if (callbacks == null)
                {
                    return;
                }
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            ExecuteCallbacks(callbacks, throwOnFirstException);
        }

        private CallbackRegistration[] BeginCancellation()
        {
            if (_cancellationRequested)
            {
                return null;
            }

            _cancellationRequested = true;
            _cancelAfterVersion++;
            if (_event != null)
            {
                _event.Set();
            }

            CallbackRegistration[] callbacks = _callbacks.ToArray();
            _callbacks.Clear();
            return callbacks;
        }

        private static void ExecuteCallbacks(CallbackRegistration[] callbacks, bool throwOnFirstException)
        {
            List<Exception> exceptions = null;
            for (int i = callbacks.Length - 1; i >= 0; i--)
            {
                if (!callbacks[i].Active)
                {
                    continue;
                }

                callbacks[i].Active = false;
                try
                {
                    callbacks[i].Callback(callbacks[i].State);
                }
                catch (Exception exception)
                {
                    if (throwOnFirstException)
                    {
                        throw;
                    }
                    if (exceptions == null)
                    {
                        exceptions = new List<Exception>();
                    }

                    exceptions.Add(exception);
                }
            }

            if (exceptions != null)
            {
                throw new AggregateException(exceptions.ToArray());
            }
        }

        public void CancelAfter(int millisecondsDelay)
        {
            if (millisecondsDelay < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsDelay");
            }

            int version;
            Monitor.Enter(_gate);
            try
            {
                ThrowIfDisposed();
                version = ++_cancelAfterVersion;
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            if (millisecondsDelay == -1)
            {
                return;
            }

            Thread timerThread = new Thread(() =>
            {
                if (millisecondsDelay > 0)
                {
                    Thread.Sleep(millisecondsDelay);
                }

                CallbackRegistration[] callbacks = null;
                Monitor.Enter(_gate);
                try
                {
                    if (!_disposed && !_cancellationRequested && version == _cancelAfterVersion)
                    {
                        callbacks = BeginCancellation();
                    }
                }
                finally
                {
                    Monitor.Exit(_gate);
                }

                if (callbacks != null)
                {
                    ExecuteCallbacks(callbacks, false);
                }
            });
            timerThread.IsBackground = true;
            timerThread.Start();
        }

        public void CancelAfter(TimeSpan delay)
        {
            double milliseconds = delay.TotalMilliseconds;
            if (milliseconds < -1 || milliseconds > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException("delay");
            }

            CancelAfter((int)milliseconds);
        }

        public static CancellationTokenSource CreateLinkedTokenSource(CancellationToken token1, CancellationToken token2)
        {
            return CreateLinkedTokenSource(new CancellationToken[] { token1, token2 });
        }

        public static CancellationTokenSource CreateLinkedTokenSource(params CancellationToken[] tokens)
        {
            if (tokens == null)
            {
                throw new ArgumentNullException("tokens");
            }
            if (tokens.Length == 0)
            {
                throw new ArgumentException("At least one cancellation token is required.");
            }

            CancellationTokenSource linkedSource = new CancellationTokenSource();
            linkedSource._linkedRegistrations = new List<CancellationTokenRegistration>();
            for (int i = 0; i < tokens.Length; i++)
            {
                if (tokens[i].CanBeCanceled)
                {
                    CancellationTokenRegistration registration = tokens[i].Register(
                        state => ((CancellationTokenSource)state).Cancel(),
                        linkedSource);
                    linkedSource._linkedRegistrations.Add(registration);
                }
            }

            return linkedSource;
        }

        internal CancellationTokenRegistration Register(Action<object> callback, object state)
        {
            bool invokeImmediately;
            int id = 0;

            ThrowIfDisposed();
            Monitor.Enter(_gate);
            try
            {
                invokeImmediately = _cancellationRequested;
                if (!invokeImmediately)
                {
                    id = ++_nextRegistrationId;
                    _callbacks.Add(new CallbackRegistration {
                        Id = id,
                        Callback = callback,
                        State = state,
                        Active = true
                    });
                }
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            if (invokeImmediately)
            {
                callback(state);
                return default(CancellationTokenRegistration);
            }

            return new CancellationTokenRegistration(this, id);
        }

        internal void Unregister(int id)
        {
            if (id == 0)
            {
                return;
            }

            Monitor.Enter(_gate);
            try
            {
                for (int i = 0; i < _callbacks.Count; i++)
                {
                    if (_callbacks[i].Id == id)
                    {
                        _callbacks[i].Active = false;
                        _callbacks.RemoveAt(i);
                        return;
                    }
                }
            }
            finally
            {
                Monitor.Exit(_gate);
            }
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }

            Monitor.Enter(_gate);
            try
            {
                if (_disposed)
                {
                    return;
                }

                _disposed = true;
                _cancelAfterVersion++;
                _callbacks.Clear();
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            if (_linkedRegistrations != null)
            {
                for (int i = 0; i < _linkedRegistrations.Count; i++)
                {
                    _linkedRegistrations[i].Dispose();
                }

                _linkedRegistrations.Clear();
            }

            if (_event != null)
            {
                _event.Dispose();
            }
        }

        private static CancellationTokenSource CreateCanceledSource()
        {
            CancellationTokenSource source = new CancellationTokenSource();
            source._cancellationRequested = true;
            return source;
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
            {
                throw new ObjectDisposedException("CancellationTokenSource");
            }
        }
    }
}
