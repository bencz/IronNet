using System.Collections;
using System.Collections.Generic;

namespace System.Linq
{
    /// <summary>
    /// Provides a minimal set of LINQ extension methods for IEnumerable<T>
    /// </summary>
    public static class Enumerable
    {
        /// <summary>
        /// Projects each element of a sequence into a new form.
        /// </summary>
        public static IEnumerable<TResult> Select<TSource, TResult>(
            this IEnumerable<TSource> source,
            Func<TSource, TResult> selector)
        {
            if (source == null) throw new ArgumentNullException("source");
            if (selector == null) throw new ArgumentNullException("selector");
            
            return new SelectIterator<TSource, TResult>(source, selector);
        }
        
        /// <summary>
        /// Filters a sequence of values based on a predicate.
        /// </summary>
        public static IEnumerable<TResult> Where<TResult>(
            this IEnumerable<TResult> source,
            Func<TResult, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException("source");
            if (predicate == null) throw new ArgumentNullException("predicate");
            
            return new WhereIterator<TResult>(source, predicate);
        }
        
        /// <summary>
        /// Computes the sum of a sequence of Int32 values.
        /// </summary>
        public static int Sum(this IEnumerable<int> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            
            int sum = 0;
            foreach (int value in source)
            {
                sum = sum + value;
            }
            return sum;
        }
        
        /// <summary>
        /// Computes the sum of the sequence of Int32 values that are obtained 
        /// by invoking a transform function on each element of the input sequence.
        /// </summary>
        public static int Sum<TSource>(
            this IEnumerable<TSource> source,
            Func<TSource, int> selector)
        {
            if (source == null) throw new ArgumentNullException("source");
            if (selector == null) throw new ArgumentNullException("selector");
            
            int sum = 0;
            foreach (TSource item in source)
            {
                sum = sum + selector(item);
            }
            return sum;
        }
        
        /// <summary>
        /// Returns the number of elements in a sequence.
        /// </summary>
        public static int Count<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            
            // Check if it's a collection with Count property
            ICollection<TSource> collection = source as ICollection<TSource>;
            if (collection != null)
            {
                return collection.Count;
            }
            
            int count = 0;
            foreach (TSource item in source)
            {
                count++;
            }
            return count;
        }
        
        /// <summary>
        /// Returns the first element of a sequence.
        /// </summary>
        public static TSource First<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            
            foreach (TSource item in source)
            {
                return item;
            }
            throw new InvalidOperationException("Sequence contains no elements");
        }
        
        /// <summary>
        /// Returns the first element of a sequence, or a default value if no element is found.
        /// </summary>
        public static TSource FirstOrDefault<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            
            foreach (TSource item in source)
            {
                return item;
            }
            return default(TSource);
        }
        
        /// <summary>
        /// Creates a List<T> from an IEnumerable<T>.
        /// </summary>
        public static List<TSource> ToList<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            
            return new List<TSource>(source);
        }
        
        /// <summary>
        /// Creates an array from an IEnumerable<T>.
        /// </summary>
        public static TSource[] ToArray<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            
            List<TSource> list = new List<TSource>(source);
            return list.ToArray();
        }
        
        /// <summary>
        /// Determines whether any element of a sequence satisfies a condition.
        /// </summary>
        public static bool Any<TSource>(
            this IEnumerable<TSource> source,
            Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException("source");
            if (predicate == null) throw new ArgumentNullException("predicate");
            
            foreach (TSource item in source)
            {
                if (predicate(item))
                {
                    return true;
                }
            }
            return false;
        }
        
        /// <summary>
        /// Determines whether all elements of a sequence satisfy a condition.
        /// </summary>
        public static bool All<TSource>(
            this IEnumerable<TSource> source,
            Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException("source");
            if (predicate == null) throw new ArgumentNullException("predicate");
            
            foreach (TSource item in source)
            {
                if (!predicate(item))
                {
                    return false;
                }
            }
            return true;
        }
    }
    
    /// <summary>
    /// Iterator for Select operation
    /// </summary>
    internal class SelectIterator<TSource, TResult> : IEnumerable<TResult>, IEnumerator<TResult>
    {
        private IEnumerable<TSource> source;
        private Func<TSource, TResult> selector;
        private IEnumerator<TSource> enumerator;
        private TResult current;
        
        public SelectIterator(IEnumerable<TSource> source, Func<TSource, TResult> selector)
        {
            this.source = source;
            this.selector = selector;
        }
        
        public TResult Current { get { return current; } }
        object IEnumerator.Current { get { return current; } }
        
        public bool MoveNext()
        {
            if (enumerator == null)
            {
                enumerator = source.GetEnumerator();
            }
            
            if (enumerator.MoveNext())
            {
                current = selector(enumerator.Current);
                return true;
            }
            return false;
        }
        
        public void Reset()
        {
            enumerator = null;
            current = default(TResult);
        }
        
        public void Dispose()
        {
            if (enumerator != null)
            {
                enumerator.Dispose();
            }
        }
        
        public IEnumerator<TResult> GetEnumerator()
        {
            return new SelectIterator<TSource, TResult>(source, selector);
        }
        
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
    
    /// <summary>
    /// Iterator for Where operation
    /// </summary>
    internal class WhereIterator<T> : IEnumerable<T>, IEnumerator<T>
    {
        private IEnumerable<T> source;
        private Func<T, bool> predicate;
        private IEnumerator<T> enumerator;
        private T current;
        
        public WhereIterator(IEnumerable<T> source, Func<T, bool> predicate)
        {
            this.source = source;
            this.predicate = predicate;
        }
        
        public T Current { get { return current; } }
        object IEnumerator.Current { get { return current; } }
        
        public bool MoveNext()
        {
            if (enumerator == null)
            {
                enumerator = source.GetEnumerator();
            }
            
            while (enumerator.MoveNext())
            {
                if (predicate(enumerator.Current))
                {
                    current = enumerator.Current;
                    return true;
                }
            }
            return false;
        }
        
        public void Reset()
        {
            enumerator = null;
            current = default(T);
        }
        
        public void Dispose()
        {
            if (enumerator != null)
            {
                enumerator.Dispose();
            }
        }
        
        public IEnumerator<T> GetEnumerator()
        {
            return new WhereIterator<T>(source, predicate);
        }
        
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}

namespace System
{
    /// <summary>
    /// Encapsulates a method that has one parameter and returns a value of the type specified by the TResult parameter.
    /// </summary>
    public delegate TResult Func<in T, out TResult>(T arg);
    
    /// <summary>
    /// Encapsulates a method that has two parameters and returns a value of the type specified by the TResult parameter.
    /// </summary>
    public delegate TResult Func<in T1, in T2, out TResult>(T1 arg1, T2 arg2);
}
