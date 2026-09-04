using System.Collections.Generic;

namespace System.Linq
{
    /// <summary>
    /// Provides sequence, aggregation, set, and materialization operators for IEnumerable&lt;T&gt;.
    /// </summary>
    public static partial class Enumerable
    {
        public static bool Any<TSource>(this IEnumerable<TSource> source)
        {
            ValidateSource(source);
            IEnumerator<TSource> enumerator = source.GetEnumerator();
            try
            {
                return enumerator.MoveNext();
            }
            finally
            {
                enumerator.Dispose();
            }
        }

        public static int Count<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");

            int count = 0;
            foreach (TSource item in source)
            {
                if (predicate(item))
                {
                    count++;
                }
            }

            return count;
        }

        public static long LongCount<TSource>(this IEnumerable<TSource> source)
        {
            ValidateSource(source);
            long count = 0;
            foreach (TSource item in source)
            {
                count++;
            }

            return count;
        }

        public static long LongCount<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");

            long count = 0;
            foreach (TSource item in source)
            {
                if (predicate(item))
                {
                    count++;
                }
            }

            return count;
        }

        public static bool Contains<TSource>(this IEnumerable<TSource> source, TSource value)
        {
            return Contains(source, value, EqualityComparer<TSource>.Default);
        }

        public static bool Contains<TSource>(this IEnumerable<TSource> source, TSource value, IEqualityComparer<TSource> comparer)
        {
            ValidateSource(source);
            comparer = comparer ?? EqualityComparer<TSource>.Default;

            foreach (TSource item in source)
            {
                if (comparer.Equals(item, value))
                {
                    return true;
                }
            }

            return false;
        }

        public static TSource First<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");

            foreach (TSource item in source)
            {
                if (predicate(item))
                {
                    return item;
                }
            }

            throw new InvalidOperationException("Sequence contains no matching element.");
        }

        public static TSource FirstOrDefault<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");

            foreach (TSource item in source)
            {
                if (predicate(item))
                {
                    return item;
                }
            }

            return default(TSource);
        }

        public static TSource Last<TSource>(this IEnumerable<TSource> source)
        {
            ValidateSource(source);

            bool found = false;
            TSource result = default(TSource);
            foreach (TSource item in source)
            {
                found = true;
                result = item;
            }

            if (!found)
            {
                throw new InvalidOperationException("Sequence contains no elements.");
            }

            return result;
        }

        public static TSource Last<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");

            bool found = false;
            TSource result = default(TSource);
            foreach (TSource item in source)
            {
                if (predicate(item))
                {
                    found = true;
                    result = item;
                }
            }

            if (!found)
            {
                throw new InvalidOperationException("Sequence contains no matching element.");
            }

            return result;
        }

        public static TSource LastOrDefault<TSource>(this IEnumerable<TSource> source)
        {
            ValidateSource(source);

            TSource result = default(TSource);
            foreach (TSource item in source)
            {
                result = item;
            }

            return result;
        }

        public static TSource LastOrDefault<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");

            TSource result = default(TSource);
            foreach (TSource item in source)
            {
                if (predicate(item))
                {
                    result = item;
                }
            }

            return result;
        }

        public static TSource Single<TSource>(this IEnumerable<TSource> source)
        {
            return SingleCore(source, null, false);
        }

        public static TSource Single<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateFunction(predicate, "predicate");
            return SingleCore(source, predicate, false);
        }

        public static TSource SingleOrDefault<TSource>(this IEnumerable<TSource> source)
        {
            return SingleCore(source, null, true);
        }

        public static TSource SingleOrDefault<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateFunction(predicate, "predicate");
            return SingleCore(source, predicate, true);
        }

        public static TSource ElementAt<TSource>(this IEnumerable<TSource> source, int index)
        {
            ValidateSource(source);
            if (index < 0)
            {
                throw new ArgumentOutOfRangeException("index");
            }

            IList<TSource> list = source as IList<TSource>;
            if (list != null)
            {
                return list[index];
            }

            foreach (TSource item in source)
            {
                if (index == 0)
                {
                    return item;
                }

                index--;
            }

            throw new ArgumentOutOfRangeException("index");
        }

        public static TSource ElementAtOrDefault<TSource>(this IEnumerable<TSource> source, int index)
        {
            ValidateSource(source);
            if (index < 0)
            {
                return default(TSource);
            }

            IList<TSource> list = source as IList<TSource>;
            if (list != null)
            {
                return index < list.Count ? list[index] : default(TSource);
            }

            foreach (TSource item in source)
            {
                if (index == 0)
                {
                    return item;
                }

                index--;
            }

            return default(TSource);
        }

        public static IEnumerable<TSource> Skip<TSource>(this IEnumerable<TSource> source, int count)
        {
            ValidateSource(source);
            return SkipIterator(source, count);
        }

        public static IEnumerable<TSource> Take<TSource>(this IEnumerable<TSource> source, int count)
        {
            ValidateSource(source);
            return TakeIterator(source, count);
        }

        public static IEnumerable<TSource> Concat<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second)
        {
            ValidateSource(first);
            ValidateSource(second, "second");
            return ConcatIterator(first, second);
        }

        public static IEnumerable<TSource> Append<TSource>(this IEnumerable<TSource> source, TSource element)
        {
            ValidateSource(source);
            return AppendIterator(source, element);
        }

        public static IEnumerable<TSource> Prepend<TSource>(this IEnumerable<TSource> source, TSource element)
        {
            ValidateSource(source);
            return PrependIterator(source, element);
        }

        public static IEnumerable<TSource> Reverse<TSource>(this IEnumerable<TSource> source)
        {
            ValidateSource(source);
            return ReverseIterator(source);
        }

        public static IEnumerable<TSource> Distinct<TSource>(this IEnumerable<TSource> source)
        {
            return Distinct(source, null);
        }

        public static IEnumerable<TSource> Distinct<TSource>(this IEnumerable<TSource> source, IEqualityComparer<TSource> comparer)
        {
            ValidateSource(source);
            return DistinctIterator(source, comparer);
        }

        public static IEnumerable<TSource> Union<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second)
        {
            return Union(first, second, null);
        }

        public static IEnumerable<TSource> Union<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second, IEqualityComparer<TSource> comparer)
        {
            ValidateSource(first);
            ValidateSource(second, "second");
            return UnionIterator(first, second, comparer);
        }

        public static IEnumerable<TSource> Intersect<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second)
        {
            return Intersect(first, second, null);
        }

        public static IEnumerable<TSource> Intersect<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second, IEqualityComparer<TSource> comparer)
        {
            ValidateSource(first);
            ValidateSource(second, "second");
            return IntersectIterator(first, second, comparer);
        }

        public static IEnumerable<TSource> Except<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second)
        {
            return Except(first, second, null);
        }

        public static IEnumerable<TSource> Except<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second, IEqualityComparer<TSource> comparer)
        {
            ValidateSource(first);
            ValidateSource(second, "second");
            return ExceptIterator(first, second, comparer);
        }

        public static bool SequenceEqual<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second)
        {
            return SequenceEqual(first, second, null);
        }

        public static bool SequenceEqual<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second, IEqualityComparer<TSource> comparer)
        {
            ValidateSource(first, "first");
            ValidateSource(second, "second");
            comparer = comparer ?? EqualityComparer<TSource>.Default;

            IEnumerator<TSource> firstEnumerator = first.GetEnumerator();
            IEnumerator<TSource> secondEnumerator = second.GetEnumerator();
            try
            {
                while (firstEnumerator.MoveNext())
                {
                    if (!secondEnumerator.MoveNext() || !comparer.Equals(firstEnumerator.Current, secondEnumerator.Current))
                    {
                        return false;
                    }
                }

                return !secondEnumerator.MoveNext();
            }
            finally
            {
                firstEnumerator.Dispose();
                secondEnumerator.Dispose();
            }
        }

        public static TSource Aggregate<TSource>(this IEnumerable<TSource> source, Func<TSource, TSource, TSource> function)
        {
            ValidateSource(source);
            ValidateFunction(function, "function");

            IEnumerator<TSource> enumerator = source.GetEnumerator();
            try
            {
                if (!enumerator.MoveNext())
                {
                    throw new InvalidOperationException("Sequence contains no elements.");
                }

                TSource result = enumerator.Current;
                while (enumerator.MoveNext())
                {
                    result = function(result, enumerator.Current);
                }

                return result;
            }
            finally
            {
                enumerator.Dispose();
            }
        }

        public static TAccumulate Aggregate<TSource, TAccumulate>(this IEnumerable<TSource> source, TAccumulate seed, Func<TAccumulate, TSource, TAccumulate> function)
        {
            ValidateSource(source);
            ValidateFunction(function, "function");

            TAccumulate result = seed;
            foreach (TSource item in source)
            {
                result = function(result, item);
            }

            return result;
        }

        public static Dictionary<TKey, TSource> ToDictionary<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return ToDictionary(source, keySelector, EqualityComparer<TKey>.Default);
        }

        public static Dictionary<TKey, TSource> ToDictionary<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector, IEqualityComparer<TKey> comparer)
        {
            ValidateSource(source);
            ValidateFunction(keySelector, "keySelector");

            Dictionary<TKey, TSource> result = new Dictionary<TKey, TSource>(comparer);
            foreach (TSource item in source)
            {
                result.Add(keySelector(item), item);
            }

            return result;
        }

        public static Dictionary<TKey, TElement> ToDictionary<TSource, TKey, TElement>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector, Func<TSource, TElement> elementSelector)
        {
            ValidateSource(source);
            ValidateFunction(keySelector, "keySelector");
            ValidateFunction(elementSelector, "elementSelector");

            Dictionary<TKey, TElement> result = new Dictionary<TKey, TElement>();
            foreach (TSource item in source)
            {
                result.Add(keySelector(item), elementSelector(item));
            }

            return result;
        }

        public static int Min(this IEnumerable<int> source)
        {
            return FindIntegerExtremum(source, true);
        }

        public static int Max(this IEnumerable<int> source)
        {
            return FindIntegerExtremum(source, false);
        }

        public static double Average(this IEnumerable<int> source)
        {
            ValidateSource(source);

            long sum = 0;
            long count = 0;
            foreach (int value in source)
            {
                sum += value;
                count++;
            }

            if (count == 0)
            {
                throw new InvalidOperationException("Sequence contains no elements.");
            }

            return (double)sum / count;
        }

        public static IEnumerable<int> Range(int start, int count)
        {
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException("count");
            }

            long maximum = (long)start + count - 1;
            if (count > 0 && maximum > Int32.MaxValue)
            {
                throw new ArgumentOutOfRangeException("count");
            }

            return RangeIterator(start, count);
        }

        public static IEnumerable<TResult> Repeat<TResult>(TResult element, int count)
        {
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException("count");
            }

            return RepeatIterator(element, count);
        }

        public static IEnumerable<TResult> Empty<TResult>()
        {
            return new TResult[0];
        }

        private static TSource SingleCore<TSource>(IEnumerable<TSource> source, Func<TSource, bool> predicate, bool returnDefault)
        {
            ValidateSource(source);

            bool found = false;
            TSource result = default(TSource);
            foreach (TSource item in source)
            {
                if (predicate != null && !predicate(item))
                {
                    continue;
                }

                if (found)
                {
                    throw new InvalidOperationException("Sequence contains more than one matching element.");
                }

                found = true;
                result = item;
            }

            if (!found && !returnDefault)
            {
                throw new InvalidOperationException("Sequence contains no matching element.");
            }

            return result;
        }

        private static int FindIntegerExtremum(IEnumerable<int> source, bool findMinimum)
        {
            ValidateSource(source);
            IEnumerator<int> enumerator = source.GetEnumerator();
            try
            {
                if (!enumerator.MoveNext())
                {
                    throw new InvalidOperationException("Sequence contains no elements.");
                }

                int result = enumerator.Current;
                while (enumerator.MoveNext())
                {
                    int value = enumerator.Current;
                    if ((findMinimum && value < result) || (!findMinimum && value > result))
                    {
                        result = value;
                    }
                }

                return result;
            }
            finally
            {
                enumerator.Dispose();
            }
        }

        private static IEnumerable<TSource> SkipIterator<TSource>(IEnumerable<TSource> source, int count)
        {
            foreach (TSource item in source)
            {
                if (count > 0)
                {
                    count--;
                    continue;
                }

                yield return item;
            }
        }

        private static IEnumerable<TSource> TakeIterator<TSource>(IEnumerable<TSource> source, int count)
        {
            if (count <= 0)
            {
                yield break;
            }

            foreach (TSource item in source)
            {
                yield return item;
                count--;
                if (count == 0)
                {
                    yield break;
                }
            }
        }

        private static IEnumerable<TSource> ConcatIterator<TSource>(IEnumerable<TSource> first, IEnumerable<TSource> second)
        {
            foreach (TSource item in first)
            {
                yield return item;
            }

            foreach (TSource item in second)
            {
                yield return item;
            }
        }

        private static IEnumerable<TSource> AppendIterator<TSource>(IEnumerable<TSource> source, TSource element)
        {
            foreach (TSource item in source)
            {
                yield return item;
            }

            yield return element;
        }

        private static IEnumerable<TSource> PrependIterator<TSource>(IEnumerable<TSource> source, TSource element)
        {
            yield return element;
            foreach (TSource item in source)
            {
                yield return item;
            }
        }

        private static IEnumerable<TSource> ReverseIterator<TSource>(IEnumerable<TSource> source)
        {
            List<TSource> buffer = new List<TSource>(source);
            for (int i = buffer.Count - 1; i >= 0; i--)
            {
                yield return buffer[i];
            }
        }

        private static IEnumerable<TSource> DistinctIterator<TSource>(IEnumerable<TSource> source, IEqualityComparer<TSource> comparer)
        {
            HashSet<TSource> yielded = new HashSet<TSource>(comparer);
            foreach (TSource item in source)
            {
                if (yielded.Add(item))
                {
                    yield return item;
                }
            }
        }

        private static IEnumerable<TSource> UnionIterator<TSource>(IEnumerable<TSource> first, IEnumerable<TSource> second, IEqualityComparer<TSource> comparer)
        {
            HashSet<TSource> yielded = new HashSet<TSource>(comparer);
            foreach (TSource item in first)
            {
                if (yielded.Add(item))
                {
                    yield return item;
                }
            }

            foreach (TSource item in second)
            {
                if (yielded.Add(item))
                {
                    yield return item;
                }
            }
        }

        private static IEnumerable<TSource> IntersectIterator<TSource>(IEnumerable<TSource> first, IEnumerable<TSource> second, IEqualityComparer<TSource> comparer)
        {
            HashSet<TSource> remaining = new HashSet<TSource>(second, comparer);
            foreach (TSource item in first)
            {
                if (remaining.Remove(item))
                {
                    yield return item;
                }
            }
        }

        private static IEnumerable<TSource> ExceptIterator<TSource>(IEnumerable<TSource> first, IEnumerable<TSource> second, IEqualityComparer<TSource> comparer)
        {
            HashSet<TSource> excluded = new HashSet<TSource>(second, comparer);
            foreach (TSource item in first)
            {
                if (excluded.Add(item))
                {
                    yield return item;
                }
            }
        }

        private static IEnumerable<int> RangeIterator(int start, int count)
        {
            for (int i = 0; i < count; i++)
            {
                yield return start + i;
            }
        }

        private static IEnumerable<TResult> RepeatIterator<TResult>(TResult element, int count)
        {
            for (int i = 0; i < count; i++)
            {
                yield return element;
            }
        }

        private static void ValidateSource<TSource>(IEnumerable<TSource> source)
        {
            ValidateSource(source, "source");
        }

        private static void ValidateSource<TSource>(IEnumerable<TSource> source, string parameterName)
        {
            if (source == null)
            {
                throw new ArgumentNullException(parameterName);
            }
        }

        private static void ValidateFunction<TFunction>(TFunction function, string parameterName) where TFunction : class
        {
            if (function == null)
            {
                throw new ArgumentNullException(parameterName);
            }
        }
    }
}
