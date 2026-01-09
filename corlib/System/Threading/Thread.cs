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
        private static int _nextId = 1;

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

        public bool IsAlive => false; // TODO: Implement

        public bool IsBackground { get; set; }

        public ThreadPriority Priority { get; set; }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void Start();

        public void Start(object parameter)
        {
            // TODO: Implement parameterized start
            Start();
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void Join();

        public bool Join(int millisecondsTimeout)
        {
            Join();
            return true;
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
                // TODO: Return actual current thread
                return null;
            }
        }

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

    /// <summary>
    /// Represents a time interval
    /// </summary>
    public struct TimeSpan
    {
        public const long TicksPerMillisecond = 10000;
        public const long TicksPerSecond = TicksPerMillisecond * 1000;
        public const long TicksPerMinute = TicksPerSecond * 60;
        public const long TicksPerHour = TicksPerMinute * 60;
        public const long TicksPerDay = TicksPerHour * 24;

        public static readonly TimeSpan Zero = new TimeSpan(0);
        public static readonly TimeSpan MaxValue = new TimeSpan(long.MaxValue);
        public static readonly TimeSpan MinValue = new TimeSpan(long.MinValue);

        private long _ticks;

        public TimeSpan(long ticks)
        {
            _ticks = ticks;
        }

        public TimeSpan(int hours, int minutes, int seconds)
        {
            _ticks = (hours * TicksPerHour) + (minutes * TicksPerMinute) + (seconds * TicksPerSecond);
        }

        public TimeSpan(int days, int hours, int minutes, int seconds)
        {
            _ticks = (days * TicksPerDay) + (hours * TicksPerHour) + 
                     (minutes * TicksPerMinute) + (seconds * TicksPerSecond);
        }

        public TimeSpan(int days, int hours, int minutes, int seconds, int milliseconds)
        {
            _ticks = (days * TicksPerDay) + (hours * TicksPerHour) + 
                     (minutes * TicksPerMinute) + (seconds * TicksPerSecond) +
                     (milliseconds * TicksPerMillisecond);
        }

        public long Ticks => _ticks;
        public int Days => (int)(_ticks / TicksPerDay);
        public int Hours => (int)((_ticks / TicksPerHour) % 24);
        public int Minutes => (int)((_ticks / TicksPerMinute) % 60);
        public int Seconds => (int)((_ticks / TicksPerSecond) % 60);
        public int Milliseconds => (int)((_ticks / TicksPerMillisecond) % 1000);

        public double TotalDays => (double)_ticks / TicksPerDay;
        public double TotalHours => (double)_ticks / TicksPerHour;
        public double TotalMinutes => (double)_ticks / TicksPerMinute;
        public double TotalSeconds => (double)_ticks / TicksPerSecond;
        public double TotalMilliseconds => (double)_ticks / TicksPerMillisecond;

        public static TimeSpan FromDays(double value) => new TimeSpan((long)(value * TicksPerDay));
        public static TimeSpan FromHours(double value) => new TimeSpan((long)(value * TicksPerHour));
        public static TimeSpan FromMinutes(double value) => new TimeSpan((long)(value * TicksPerMinute));
        public static TimeSpan FromSeconds(double value) => new TimeSpan((long)(value * TicksPerSecond));
        public static TimeSpan FromMilliseconds(double value) => new TimeSpan((long)(value * TicksPerMillisecond));
        public static TimeSpan FromTicks(long value) => new TimeSpan(value);

        public TimeSpan Add(TimeSpan ts) => new TimeSpan(_ticks + ts._ticks);
        public TimeSpan Subtract(TimeSpan ts) => new TimeSpan(_ticks - ts._ticks);
        public TimeSpan Negate() => new TimeSpan(-_ticks);
        public TimeSpan Duration() => new TimeSpan(_ticks < 0 ? -_ticks : _ticks);

        public static TimeSpan operator +(TimeSpan t1, TimeSpan t2) => t1.Add(t2);
        public static TimeSpan operator -(TimeSpan t1, TimeSpan t2) => t1.Subtract(t2);
        public static TimeSpan operator -(TimeSpan t) => t.Negate();
        public static bool operator ==(TimeSpan t1, TimeSpan t2) => t1._ticks == t2._ticks;
        public static bool operator !=(TimeSpan t1, TimeSpan t2) => t1._ticks != t2._ticks;
        public static bool operator <(TimeSpan t1, TimeSpan t2) => t1._ticks < t2._ticks;
        public static bool operator >(TimeSpan t1, TimeSpan t2) => t1._ticks > t2._ticks;
        public static bool operator <=(TimeSpan t1, TimeSpan t2) => t1._ticks <= t2._ticks;
        public static bool operator >=(TimeSpan t1, TimeSpan t2) => t1._ticks >= t2._ticks;

        public override bool Equals(object obj)
        {
            if (obj is TimeSpan ts)
                return _ticks == ts._ticks;
            return false;
        }

        public override int GetHashCode() => (int)_ticks ^ (int)(_ticks >> 32);

        public override string ToString()
        {
            return Days + "." + Hours.ToString().PadLeft(2, '0') + ":" +
                   Minutes.ToString().PadLeft(2, '0') + ":" +
                   Seconds.ToString().PadLeft(2, '0');
        }
    }
}
