namespace System.Threading
{
    public enum EventResetMode
    {
        AutoReset = 0,
        ManualReset = 1
    }

    /// <summary>
    /// An unnamed synchronization event with manual or automatic reset semantics.
    /// </summary>
    public class EventWaitHandle : WaitHandle
    {
        private readonly object _gate = new object();
        private readonly bool _autoReset;
        private bool _signaled;
        private bool _disposed;

        public EventWaitHandle(bool initialState, EventResetMode mode)
        {
            if (mode != EventResetMode.AutoReset && mode != EventResetMode.ManualReset)
            {
                throw new ArgumentException("The event reset mode is invalid.");
            }

            _signaled = initialState;
            _autoReset = mode == EventResetMode.AutoReset;
        }

        public bool Set()
        {
            Monitor.Enter(_gate);
            try
            {
                ThrowIfDisposed();
                _signaled = true;
                if (_autoReset)
                {
                    Monitor.Pulse(_gate);
                }
                else
                {
                    Monitor.PulseAll(_gate);
                }

                return true;
            }
            finally
            {
                Monitor.Exit(_gate);
            }
        }

        public bool Reset()
        {
            Monitor.Enter(_gate);
            try
            {
                ThrowIfDisposed();
                _signaled = false;
                return true;
            }
            finally
            {
                Monitor.Exit(_gate);
            }
        }

        public override bool WaitOne(int millisecondsTimeout)
        {
            ValidateTimeout(millisecondsTimeout);
            int start = Environment.TickCount;

            Monitor.Enter(_gate);
            try
            {
                ThrowIfDisposed();
                while (!_signaled)
                {
                    int remaining = RemainingTimeout(start, millisecondsTimeout);
                    if (remaining == 0)
                    {
                        return false;
                    }

                    Monitor.Wait(_gate, remaining);
                    ThrowIfDisposed();
                }

                if (_autoReset)
                {
                    _signaled = false;
                }

                return true;
            }
            finally
            {
                Monitor.Exit(_gate);
            }
        }

        protected override void Dispose(bool disposing)
        {
            Monitor.Enter(_gate);
            try
            {
                _disposed = true;
                Monitor.PulseAll(_gate);
            }
            finally
            {
                Monitor.Exit(_gate);
            }

            base.Dispose(disposing);
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
            {
                throw new ObjectDisposedException("EventWaitHandle");
            }
        }
    }
}
