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
        private readonly CancellationTokenSource.CallbackRegistration _registration;

        internal CancellationTokenRegistration(CancellationTokenSource source, CancellationTokenSource.CallbackRegistration registration)
        {
            _source = source;
            _registration = registration;
        }

        public void Dispose()
        {
            if (_source != null)
            {
                _source.Unregister(_registration);
            }
        }

        public bool Equals(CancellationTokenRegistration other)
        {
            return object.ReferenceEquals(_source, other._source) && object.ReferenceEquals(_registration, other._registration);
        }

        public override bool Equals(object obj)
        {
            return obj is CancellationTokenRegistration && Equals((CancellationTokenRegistration)obj);
        }

        public override int GetHashCode()
        {
            return _registration == null ? 0 : _registration.GetHashCode();
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
        internal sealed class CallbackRegistration
        {
            public Action<object> Callback;
            public object State;
            public bool Active;
            public Thread ExecutingThread;
        }

        private readonly object _gate = new object();
        private readonly List<CallbackRegistration> _callbacks = new List<CallbackRegistration>();
        private List<CancellationTokenRegistration> _linkedRegistrations;
        private ManualResetEvent _event;
        private volatile bool _cancellationRequested;
        private bool _disposed;
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
            Cancel(throwOnFirstException, true);
        }

        private void Cancel(bool throwOnFirstException, bool throwIfDisposed)
        {
            CallbackRegistration[] callbacks;

            Monitor.Enter(_gate);
            try
            {
                if (_disposed && !throwIfDisposed)
                {
                    return;
                }

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

        private void ExecuteCallbacks(CallbackRegistration[] callbacks, bool throwOnFirstException)
        {
            List<Exception> exceptions = null;
            try
            {
                for (int i = callbacks.Length - 1; i >= 0; i--)
                {
                    CallbackRegistration registration = callbacks[i];
                    Action<object> callback;
                    object state;

                    Monitor.Enter(_gate);
                    try
                    {
                        if (!registration.Active)
                        {
                            continue;
                        }

                        // Claim the callback under the same lock used by registration disposal.
                        registration.Active = false;
                        registration.ExecutingThread = Thread.CurrentThread;
                        callback = registration.Callback;
                        state = registration.State;
                    }
                    finally
                    {
                        Monitor.Exit(_gate);
                    }

                    try
                    {
                        callback(state);
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
                    finally
                    {
                        Monitor.Enter(_gate);
                        try
                        {
                            registration.ExecutingThread = null;
                            registration.Callback = null;
                            registration.State = null;
                            Monitor.PulseAll(_gate);
                        }
                        finally
                        {
                            Monitor.Exit(_gate);
                        }
                    }
                }
            }
            finally
            {
                // Cancel(true) can abandon pending callbacks; do not retain their captured state.
                Monitor.Enter(_gate);
                try
                {
                    for (int i = 0; i < callbacks.Length; i++)
                    {
                        callbacks[i].Active = false;
                        callbacks[i].Callback = null;
                        callbacks[i].State = null;
                    }
                }
                finally
                {
                    Monitor.Exit(_gate);
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
                if (_cancellationRequested)
                {
                    return;
                }

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
                        state => ((CancellationTokenSource)state).Cancel(false, false),
                        linkedSource);
                    linkedSource._linkedRegistrations.Add(registration);
                }
            }

            return linkedSource;
        }

        internal CancellationTokenRegistration Register(Action<object> callback, object state)
        {
            bool invokeImmediately;
            CallbackRegistration registration = null;

            Monitor.Enter(_gate);
            try
            {
                ThrowIfDisposed();
                invokeImmediately = _cancellationRequested;
                if (!invokeImmediately)
                {
                    registration = new CallbackRegistration {
                        Callback = callback,
                        State = state,
                        Active = true
                    };
                    _callbacks.Add(registration);
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

            return new CancellationTokenRegistration(this, registration);
        }

        internal void Unregister(CallbackRegistration registration)
        {
            if (registration == null)
            {
                return;
            }

            Monitor.Enter(_gate);
            try
            {
                if (registration.Active)
                {
                    registration.Active = false;
                    registration.Callback = null;
                    registration.State = null;
                    _callbacks.Remove(registration);
                }

                // A callback may dispose its own registration, including through a linked source.
                Thread currentThread = Thread.CurrentThread;
                while (registration.ExecutingThread != null && !object.ReferenceEquals(registration.ExecutingThread, currentThread))
                {
                    Monitor.Wait(_gate);
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
                for (int i = 0; i < _callbacks.Count; i++)
                {
                    _callbacks[i].Active = false;
                    _callbacks[i].Callback = null;
                    _callbacks[i].State = null;
                }

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
