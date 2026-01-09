using System.Runtime.CompilerServices;

namespace System.Threading
{
    /// <summary>
    /// Provides a mechanism that synchronizes access to objects
    /// </summary>
    public static class Monitor
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Enter(object obj);

        public static void Enter(object obj, ref bool lockTaken)
        {
            Enter(obj);
            lockTaken = true;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Exit(object obj);

        public static bool TryEnter(object obj)
        {
            return TryEnter(obj, 0);
        }

        public static void TryEnter(object obj, ref bool lockTaken)
        {
            lockTaken = TryEnter(obj, 0);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool TryEnter(object obj, int millisecondsTimeout);

        public static bool TryEnter(object obj, TimeSpan timeout)
        {
            return TryEnter(obj, (int)timeout.TotalMilliseconds);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool Wait(object obj);

        public static bool Wait(object obj, int millisecondsTimeout)
        {
            return Wait(obj);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Pulse(object obj);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void PulseAll(object obj);
    }

    /// <summary>
    /// Provides atomic operations for variables that are shared by multiple threads
    /// </summary>
    public static class Interlocked
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Increment(ref int location);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long Increment(ref long location);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Decrement(ref int location);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long Decrement(ref long location);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Exchange(ref int location1, int value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long Exchange(ref long location1, long value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern object Exchange(ref object location1, object value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int CompareExchange(ref int location1, int value, int comparand);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long CompareExchange(ref long location1, long value, long comparand);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern object CompareExchange(ref object location1, object value, object comparand);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Add(ref int location1, int value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern long Add(ref long location1, long value);

        public static long Read(ref long location)
        {
            return CompareExchange(ref location, 0, 0);
        }

        public static void MemoryBarrier()
        {
            // TODO: Implement memory barrier
        }
    }

    /// <summary>
    /// Notifies one or more waiting threads that an event has occurred
    /// </summary>
    public class ManualResetEvent : WaitHandle
    {
        private bool _signaled;

        public ManualResetEvent(bool initialState)
        {
            _signaled = initialState;
        }

        public bool Set()
        {
            _signaled = true;
            return true;
        }

        public bool Reset()
        {
            _signaled = false;
            return true;
        }

        public override bool WaitOne()
        {
            while (!_signaled)
            {
                Thread.Sleep(1);
            }
            return true;
        }
    }

    /// <summary>
    /// Represents a thread synchronization event that resets automatically
    /// </summary>
    public class AutoResetEvent : WaitHandle
    {
        private bool _signaled;

        public AutoResetEvent(bool initialState)
        {
            _signaled = initialState;
        }

        public bool Set()
        {
            _signaled = true;
            return true;
        }

        public bool Reset()
        {
            _signaled = false;
            return true;
        }

        public override bool WaitOne()
        {
            while (!_signaled)
            {
                Thread.Sleep(1);
            }
            _signaled = false;
            return true;
        }
    }

    /// <summary>
    /// Encapsulates operating system-specific objects that wait for exclusive access to shared resources
    /// </summary>
    public abstract class WaitHandle : IDisposable
    {
        public const int WaitTimeout = 258;

        public virtual bool WaitOne()
        {
            return WaitOne(-1);
        }

        public virtual bool WaitOne(int millisecondsTimeout)
        {
            return true;
        }

        public virtual bool WaitOne(TimeSpan timeout)
        {
            return WaitOne((int)timeout.TotalMilliseconds);
        }

        public virtual void Close()
        {
            Dispose();
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
        }
    }

    /// <summary>
    /// Limits the number of threads that can access a resource concurrently
    /// </summary>
    public sealed class Semaphore : WaitHandle
    {
        private int _currentCount;
        private int _maximumCount;

        public Semaphore(int initialCount, int maximumCount)
        {
            if (initialCount < 0)
                throw new ArgumentOutOfRangeException("initialCount");
            if (maximumCount < 1)
                throw new ArgumentOutOfRangeException("maximumCount");
            if (initialCount > maximumCount)
                throw new ArgumentException("initialCount must be <= maximumCount");

            _currentCount = initialCount;
            _maximumCount = maximumCount;
        }

        public int Release()
        {
            return Release(1);
        }

        public int Release(int releaseCount)
        {
            if (releaseCount < 1)
                throw new ArgumentOutOfRangeException("releaseCount");

            int previousCount = _currentCount;
            _currentCount += releaseCount;
            if (_currentCount > _maximumCount)
            {
                _currentCount = previousCount;
                throw new InvalidOperationException("Adding the specified count would cause the semaphore to exceed its maximum count.");
            }
            return previousCount;
        }

        public override bool WaitOne()
        {
            while (_currentCount <= 0)
            {
                Thread.Sleep(1);
            }
            _currentCount--;
            return true;
        }
    }

    /// <summary>
    /// A synchronization primitive that can also be used for interprocess synchronization
    /// </summary>
    public sealed class Mutex : WaitHandle
    {
        private bool _owned;

        public Mutex() : this(false)
        {
        }

        public Mutex(bool initiallyOwned)
        {
            _owned = initiallyOwned;
        }

        public void ReleaseMutex()
        {
            _owned = false;
        }

        public override bool WaitOne()
        {
            while (_owned)
            {
                Thread.Sleep(1);
            }
            _owned = true;
            return true;
        }
    }
}
