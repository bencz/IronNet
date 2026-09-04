using System.Collections;

namespace System.Collections.Generic
{
    /// <summary>
    /// Provides a base class for ordering comparisons.
    /// </summary>
    public abstract class Comparer<T> : IComparer<T>, IComparer
    {
        private static Comparer<T> _default;

        public static Comparer<T> Default
        {
            get
            {
                if (_default == null)
                {
                    _default = new DefaultComparer<T>();
                }

                return _default;
            }
        }

        public static Comparer<T> Create(Comparison<T> comparison)
        {
            if (comparison == null)
            {
                throw new ArgumentNullException("comparison");
            }

            return new ComparisonComparer<T>(comparison);
        }

        public abstract int Compare(T x, T y);

        int IComparer.Compare(object x, object y)
        {
            if (x == y)
            {
                return 0;
            }
            if (x == null)
            {
                return -1;
            }
            if (y == null)
            {
                return 1;
            }
            if (!(x is T) || !(y is T))
            {
                throw new ArgumentException("Objects being compared must be compatible with the comparer type.");
            }

            return Compare((T)x, (T)y);
        }
    }

    internal sealed class DefaultComparer<T> : Comparer<T>
    {
        public override int Compare(T x, T y)
        {
            object left = x;
            object right = y;
            if (left == right)
            {
                return 0;
            }
            if (left == null)
            {
                return -1;
            }
            if (right == null)
            {
                return 1;
            }

            IComparable<T> genericComparable = left as IComparable<T>;
            if (genericComparable != null)
            {
                return genericComparable.CompareTo(y);
            }

            IComparable comparable = left as IComparable;
            if (comparable != null)
            {
                return comparable.CompareTo(right);
            }

            throw new ArgumentException("At least one object must implement IComparable.");
        }
    }

    internal sealed class ComparisonComparer<T> : Comparer<T>
    {
        private readonly Comparison<T> _comparison;

        internal ComparisonComparer(Comparison<T> comparison)
        {
            _comparison = comparison;
        }

        public override int Compare(T x, T y)
        {
            return _comparison(x, y);
        }
    }
}
