namespace System
{
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
            // TODO: Implement multicast combination
            return this;
        }

        protected override Delegate RemoveImpl(Delegate value)
        {
            // TODO: Implement multicast removal
            return base.RemoveImpl(value);
        }

        public Delegate[] GetInvocationList()
        {
            if (_invocationList == null)
                return new Delegate[] { this };
            return _invocationList;
        }
    }
}
