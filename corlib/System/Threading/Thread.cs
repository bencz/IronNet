using System.Runtime.CompilerServices;

namespace System.Threading
{
    /// <summary>
    /// Creates and controls a thread
    /// </summary>
    public sealed class Thread
    {
        private ThreadStart _start;
        private ParameterizedThreadStart _paramStart;
        private string _name;
        private int _managedThreadId;
        private bool _started;
        private static int _nextId = 2;

        public Thread(ThreadStart start)
        {
            _start = start ?? throw new ArgumentNullException("start");
            _managedThreadId = _nextId++;
        }

        public Thread(ParameterizedThreadStart start)
        {
            _paramStart = start ?? throw new ArgumentNullException("start");
            _managedThreadId = _nextId++;
        }

        public string Name
        {
            get => _name;
            set => _name = value;
        }

        public int ManagedThreadId => _managedThreadId;

        public bool IsAlive
        {
            get
            {
                return GetIsAliveInternal();
            }
        }

        public bool IsBackground { get; set; }

        public ThreadPriority Priority { get; set; }

        public void Start()
        {
            StartCore(null, false);
        }

        public void Start(object parameter)
        {
            if (_paramStart == null)
            {
                throw new InvalidOperationException("A parameter can only be supplied to a ParameterizedThreadStart thread.");
            }

            StartCore(parameter, true);
        }

        private void StartCore(object parameter, bool parameterSupplied)
        {
            if (_started)
            {
                throw new InvalidOperationException("A thread can only be started once.");
            }

            Delegate start = _start != null ? (Delegate)_start : _paramStart;
            bool hasParameter = _paramStart != null;
            _started = true;
            StartInternal(start, hasParameter && parameterSupplied ? parameter : null, hasParameter);
        }

        public void Join()
        {
            if (!_started)
            {
                throw new InvalidOperationException("The thread has not been started.");
            }

            JoinInternal(-1);
        }

        public bool Join(int millisecondsTimeout)
        {
            if (!_started)
            {
                throw new InvalidOperationException("The thread has not been started.");
            }
            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsTimeout");
            }

            return JoinInternal(millisecondsTimeout);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Sleep(int millisecondsTimeout);

        public static void Sleep(TimeSpan timeout)
        {
            Sleep((int)timeout.TotalMilliseconds);
        }

        public static Thread CurrentThread
        {
            get
            {
                return GetCurrentThreadInternal();
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void StartInternal(Delegate start, object parameter, bool hasParameter);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern bool GetIsAliveInternal();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern bool JoinInternal(int millisecondsTimeout);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern Thread GetCurrentThreadInternal();

        public void Abort()
        {
            throw new NotSupportedException("Thread.Abort is not supported");
        }
    }

    /// <summary>
    /// Represents the method that executes on a Thread
    /// </summary>
    public delegate void ThreadStart();

    /// <summary>
    /// Represents the method that executes on a Thread with a parameter
    /// </summary>
    public delegate void ParameterizedThreadStart(object obj);

    /// <summary>
    /// Specifies the scheduling priority of a Thread
    /// </summary>
    public enum ThreadPriority
    {
        Lowest = 0,
        BelowNormal = 1,
        Normal = 2,
        AboveNormal = 3,
        Highest = 4
    }

}
