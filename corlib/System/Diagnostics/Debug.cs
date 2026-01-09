using System.Threading;

namespace System.Diagnostics
{
    /// <summary>
    /// Provides a set of methods and properties that help debug your code
    /// </summary>
    public static class Debug
    {
        public static void Assert(bool condition)
        {
            Assert(condition, null, null);
        }

        public static void Assert(bool condition, string message)
        {
            Assert(condition, message, null);
        }

        public static void Assert(bool condition, string message, string detailMessage)
        {
            if (!condition)
            {
                string msg = "Assertion failed";
                if (message != null)
                    msg += ": " + message;
                if (detailMessage != null)
                    msg += "\n" + detailMessage;
                Console.WriteLine(msg);
            }
        }

        public static void WriteLine(string message)
        {
            Console.WriteLine(message);
        }

        public static void WriteLine(object value)
        {
            Console.WriteLine(value);
        }

        public static void Write(string message)
        {
            Console.Write(message);
        }

        public static void WriteIf(bool condition, string message)
        {
            if (condition)
                Write(message);
        }

        public static void WriteLineIf(bool condition, string message)
        {
            if (condition)
                WriteLine(message);
        }

        public static void Fail(string message)
        {
            WriteLine("Debug Fail: " + message);
        }

        public static void Fail(string message, string detailMessage)
        {
            WriteLine("Debug Fail: " + message);
            if (detailMessage != null)
                WriteLine(detailMessage);
        }
    }

    /// <summary>
    /// Provides a set of methods and properties for measuring elapsed time
    /// </summary>
    public class Stopwatch
    {
        private long _startTimestamp;
        private long _elapsed;
        private bool _isRunning;

        public static readonly long Frequency = 10000000; // 100ns ticks
        public static readonly bool IsHighResolution = false;

        public Stopwatch()
        {
        }

        public void Start()
        {
            if (!_isRunning)
            {
                _startTimestamp = GetTimestamp();
                _isRunning = true;
            }
        }

        public void Stop()
        {
            if (_isRunning)
            {
                _elapsed += GetTimestamp() - _startTimestamp;
                _isRunning = false;
            }
        }

        public void Reset()
        {
            _elapsed = 0;
            _isRunning = false;
        }

        public void Restart()
        {
            _elapsed = 0;
            _startTimestamp = GetTimestamp();
            _isRunning = true;
        }

        public bool IsRunning => _isRunning;

        public TimeSpan Elapsed => new TimeSpan(ElapsedTicks);

        public long ElapsedMilliseconds => ElapsedTicks / (Frequency / 1000);

        public long ElapsedTicks
        {
            get
            {
                long elapsed = _elapsed;
                if (_isRunning)
                    elapsed += GetTimestamp() - _startTimestamp;
                return elapsed;
            }
        }

        public static Stopwatch StartNew()
        {
            Stopwatch sw = new Stopwatch();
            sw.Start();
            return sw;
        }

        public static long GetTimestamp()
        {
            return Environment.TickCount * (Frequency / 1000);
        }
    }
}
