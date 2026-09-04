namespace System
{
    public delegate bool Predicate<in T>(T obj);
    public delegate int Comparison<in T>(T x, T y);

    public delegate TResult Func<out TResult>();
    public delegate TResult Func<in T, out TResult>(T arg);
    public delegate TResult Func<in T1, in T2, out TResult>(T1 arg1, T2 arg2);
    public delegate TResult Func<in T1, in T2, in T3, out TResult>(T1 arg1, T2 arg2, T3 arg3);
    public delegate TResult Func<in T1, in T2, in T3, in T4, out TResult>(T1 arg1, T2 arg2, T3 arg3, T4 arg4);

    public delegate void Action();
    public delegate void Action<in T>(T obj);
    public delegate void Action<in T1, in T2>(T1 arg1, T2 arg2);
    public delegate void Action<in T1, in T2, in T3>(T1 arg1, T2 arg2, T3 arg3);
    public delegate void Action<in T1, in T2, in T3, in T4>(T1 arg1, T2 arg2, T3 arg3, T4 arg4);

    /// <summary>
    /// Represents a delegate, which is a data structure that refers to a static method or to a class instance and an instance method
    /// </summary>
    public abstract class Delegate
    {
        internal object _target;
        internal IntPtr _methodPtr;

        protected Delegate(object target, string method)
        {
            _target = target;
        }

        protected Delegate(Type target, string method)
        {
        }

        public object Target => _target;

        public static Delegate Combine(Delegate a, Delegate b)
        {
            if (a == null) return b;
            if (b == null) return a;
            return a.CombineImpl(b);
        }

        public static Delegate Remove(Delegate source, Delegate value)
        {
            if (source == null) return null;
            if (value == null) return source;
            return source.RemoveImpl(value);
        }

        protected virtual Delegate CombineImpl(Delegate d)
        {
            throw new NotSupportedException();
        }

        protected virtual Delegate RemoveImpl(Delegate d)
        {
            if (this.Equals(d))
                return null;
            return this;
        }

        public override bool Equals(object obj)
        {
            if (obj is Delegate d)
            {
                return _target == d._target && _methodPtr == d._methodPtr;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return _methodPtr.GetHashCode();
        }

        public static bool operator ==(Delegate d1, Delegate d2)
        {
            if ((object)d1 == null)
                return (object)d2 == null;
            return d1.Equals(d2);
        }

        public static bool operator !=(Delegate d1, Delegate d2)
        {
            return !(d1 == d2);
        }
    }

    /// <summary>
    /// Represents a multicast delegate
    /// </summary>
    public abstract class MulticastDelegate : Delegate
    {
        private Delegate[] _invocationList;

        protected MulticastDelegate(object target, string method) : base(target, method)
        {
        }

        protected MulticastDelegate(Type target, string method) : base(target, method)
        {
        }

        protected override Delegate CombineImpl(Delegate follow)
        {
            MulticastDelegate mc = (MulticastDelegate)follow;
            Delegate[] thisList = GetInvocationList();
            Delegate[] followList = mc.GetInvocationList();
            Delegate[] newList = new Delegate[thisList.Length + followList.Length];
            int i;
            for (i = 0; i < thisList.Length; i++)
                newList[i] = thisList[i];
            for (int j = 0; j < followList.Length; j++)
                newList[i + j] = followList[j];

            /* Create result that looks like the last delegate but carries the full list.
             * Since we can't construct a new MulticastDelegate directly (it's abstract),
             * we copy this delegate and attach the combined invocation list. */
            MulticastDelegate result = (MulticastDelegate)this.MemberwiseClone();
            result._invocationList = newList;
            return result;
        }

        protected override Delegate RemoveImpl(Delegate value)
        {
            Delegate[] list = GetInvocationList();
            if (list.Length == 1)
                return base.RemoveImpl(value);

            /* Find the last occurrence of value in the invocation list */
            int removeIdx = -1;
            for (int i = list.Length - 1; i >= 0; i--)
            {
                if (list[i].Equals(value))
                {
                    removeIdx = i;
                    break;
                }
            }

            if (removeIdx < 0)
                return this;

            if (list.Length == 2)
                return list[1 - removeIdx];

            Delegate[] newList = new Delegate[list.Length - 1];
            int j = 0;
            for (int i = 0; i < list.Length; i++)
            {
                if (i != removeIdx)
                    newList[j++] = list[i];
            }

            MulticastDelegate result = (MulticastDelegate)this.MemberwiseClone();
            result._invocationList = newList;
            return result;
        }

        public Delegate[] GetInvocationList()
        {
            if (_invocationList == null)
                return new Delegate[] { this };
            return _invocationList;
        }
    }
}
