using System.Collections;
using System.Collections.Generic;

namespace System.Linq
{
    /// <summary>
    /// Represents a sequence on which a secondary ordering can be applied.
    /// </summary>
    public interface IOrderedEnumerable<out TElement> : IEnumerable<TElement>
    {
        IOrderedEnumerable<TElement> CreateOrderedEnumerable<TKey>(Func<TElement, TKey> keySelector, IComparer<TKey> comparer, bool descending);
    }

    public static partial class Enumerable
    {
        public static IOrderedEnumerable<TSource> OrderBy<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return OrderBy(source, keySelector, null);
        }

        public static IOrderedEnumerable<TSource> OrderBy<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector, IComparer<TKey> comparer)
        {
            ValidateSource(source);
            ValidateFunction(keySelector, "keySelector");
            return new OrderedEnumerable<TSource, TKey>(source, null, keySelector, comparer, false);
        }

        public static IOrderedEnumerable<TSource> OrderByDescending<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return OrderByDescending(source, keySelector, null);
        }

        public static IOrderedEnumerable<TSource> OrderByDescending<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector, IComparer<TKey> comparer)
        {
            ValidateSource(source);
            ValidateFunction(keySelector, "keySelector");
            return new OrderedEnumerable<TSource, TKey>(source, null, keySelector, comparer, true);
        }

        public static IOrderedEnumerable<TSource> ThenBy<TSource, TKey>(this IOrderedEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return ThenBy(source, keySelector, null);
        }

        public static IOrderedEnumerable<TSource> ThenBy<TSource, TKey>(this IOrderedEnumerable<TSource> source, Func<TSource, TKey> keySelector, IComparer<TKey> comparer)
        {
            if (source == null)
            {
                throw new ArgumentNullException("source");
            }
            ValidateFunction(keySelector, "keySelector");
            return source.CreateOrderedEnumerable(keySelector, comparer, false);
        }

        public static IOrderedEnumerable<TSource> ThenByDescending<TSource, TKey>(this IOrderedEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return ThenByDescending(source, keySelector, null);
        }

        public static IOrderedEnumerable<TSource> ThenByDescending<TSource, TKey>(this IOrderedEnumerable<TSource> source, Func<TSource, TKey> keySelector, IComparer<TKey> comparer)
        {
            if (source == null)
            {
                throw new ArgumentNullException("source");
            }
            ValidateFunction(keySelector, "keySelector");
            return source.CreateOrderedEnumerable(keySelector, comparer, true);
        }

        public static IEnumerable<TResult> SelectMany<TSource, TResult>(this IEnumerable<TSource> source, Func<TSource, IEnumerable<TResult>> selector)
        {
            ValidateSource(source);
            ValidateFunction(selector, "selector");
            return SelectManyIterator(source, selector);
        }

        public static IEnumerable<TResult> SelectMany<TSource, TCollection, TResult>(this IEnumerable<TSource> source,
                                                                                     Func<TSource, IEnumerable<TCollection>> collectionSelector,
                                                                                     Func<TSource, TCollection, TResult> resultSelector)
        {
            ValidateSource(source);
            ValidateFunction(collectionSelector, "collectionSelector");
            ValidateFunction(resultSelector, "resultSelector");
            return SelectManyIterator(source, collectionSelector, resultSelector);
        }

        public static IEnumerable<TSource> SkipWhile<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");
            return SkipWhileIterator(source, predicate);
        }

        public static IEnumerable<TSource> TakeWhile<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            ValidateSource(source);
            ValidateFunction(predicate, "predicate");
            return TakeWhileIterator(source, predicate);
        }

        public static IEnumerable<TResult> Zip<TFirst, TSecond, TResult>(this IEnumerable<TFirst> first,
                                                                         IEnumerable<TSecond> second,
                                                                         Func<TFirst, TSecond, TResult> resultSelector)
        {
            ValidateSource(first, "first");
            ValidateSource(second, "second");
            ValidateFunction(resultSelector, "resultSelector");
            return ZipIterator(first, second, resultSelector);
        }

        private static IEnumerable<TResult> SelectManyIterator<TSource, TResult>(IEnumerable<TSource> source, Func<TSource, IEnumerable<TResult>> selector)
        {
            foreach (TSource item in source)
            {
                IEnumerable<TResult> collection = selector(item);
                foreach (TResult result in collection)
                {
                    yield return result;
                }
            }
        }

        private static IEnumerable<TResult> SelectManyIterator<TSource, TCollection, TResult>(IEnumerable<TSource> source,
                                                                                               Func<TSource, IEnumerable<TCollection>> collectionSelector,
                                                                                               Func<TSource, TCollection, TResult> resultSelector)
        {
            foreach (TSource item in source)
            {
                IEnumerable<TCollection> collection = collectionSelector(item);
                foreach (TCollection collectionItem in collection)
                {
                    yield return resultSelector(item, collectionItem);
                }
            }
        }

        private static IEnumerable<TSource> SkipWhileIterator<TSource>(IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            bool yielding = false;
            foreach (TSource item in source)
            {
                if (!yielding && !predicate(item))
                {
                    yielding = true;
                }

                if (yielding)
                {
                    yield return item;
                }
            }
        }

        private static IEnumerable<TSource> TakeWhileIterator<TSource>(IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            foreach (TSource item in source)
            {
                if (!predicate(item))
                {
                    yield break;
                }

                yield return item;
            }
        }

        private static IEnumerable<TResult> ZipIterator<TFirst, TSecond, TResult>(IEnumerable<TFirst> first,
                                                                                  IEnumerable<TSecond> second,
                                                                                  Func<TFirst, TSecond, TResult> resultSelector)
        {
            IEnumerator<TFirst> firstEnumerator = first.GetEnumerator();
            IEnumerator<TSecond> secondEnumerator = second.GetEnumerator();
            try
            {
                while (firstEnumerator.MoveNext() && secondEnumerator.MoveNext())
                {
                    yield return resultSelector(firstEnumerator.Current, secondEnumerator.Current);
                }
            }
            finally
            {
                firstEnumerator.Dispose();
                secondEnumerator.Dispose();
            }
        }
    }

    internal interface IOrdering<TElement>
    {
        int Compare(TElement left, TElement right);
    }

    internal sealed class CompoundOrdering<TElement> : IOrdering<TElement>
    {
        private readonly IOrdering<TElement> _primary;
        private readonly IOrdering<TElement> _secondary;

        internal CompoundOrdering(IOrdering<TElement> primary, IOrdering<TElement> secondary)
        {
            _primary = primary;
            _secondary = secondary;
        }

        public int Compare(TElement left, TElement right)
        {
            int result = _primary.Compare(left, right);
            return result != 0 ? result : _secondary.Compare(left, right);
        }
    }

    internal sealed class KeyOrdering<TElement, TKey> : IOrdering<TElement>
    {
        private readonly Func<TElement, TKey> _keySelector;
        private readonly IComparer<TKey> _comparer;
        private readonly bool _descending;

        internal KeyOrdering(Func<TElement, TKey> keySelector, IComparer<TKey> comparer, bool descending)
        {
            _keySelector = keySelector;
            _comparer = comparer ?? Comparer<TKey>.Default;
            _descending = descending;
        }

        public int Compare(TElement left, TElement right)
        {
            TKey leftKey = _keySelector(left);
            TKey rightKey = _keySelector(right);
            return _descending ? _comparer.Compare(rightKey, leftKey) : _comparer.Compare(leftKey, rightKey);
        }
    }

    internal sealed class OrderedEnumerable<TElement, TKey> : IOrderedEnumerable<TElement>
    {
        private readonly IEnumerable<TElement> _source;
        private readonly IOrdering<TElement> _ordering;

        internal OrderedEnumerable(IEnumerable<TElement> source,
                                   IOrdering<TElement> parentOrdering,
                                   Func<TElement, TKey> keySelector,
                                   IComparer<TKey> comparer,
                                   bool descending)
        {
            _source = source;
            IOrdering<TElement> current = new KeyOrdering<TElement, TKey>(keySelector, comparer, descending);
            _ordering = parentOrdering == null ? current : new CompoundOrdering<TElement>(parentOrdering, current);
        }

        public IOrderedEnumerable<TElement> CreateOrderedEnumerable<TNextKey>(Func<TElement, TNextKey> keySelector, IComparer<TNextKey> comparer, bool descending)
        {
            if (keySelector == null)
            {
                throw new ArgumentNullException("keySelector");
            }

            return new OrderedEnumerable<TElement, TNextKey>(_source, _ordering, keySelector, comparer, descending);
        }

        public IEnumerator<TElement> GetEnumerator()
        {
            TElement[] items = new List<TElement>(_source).ToArray();
            StableSort(items, _ordering);
            return ((IEnumerable<TElement>)items).GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        private static void StableSort(TElement[] items, IOrdering<TElement> ordering)
        {
            if (items.Length < 2)
            {
                return;
            }

            TElement[] buffer = new TElement[items.Length];
            TElement[] source = items;
            TElement[] destination = buffer;

            for (int width = 1; width < items.Length; width = width > items.Length / 2 ? items.Length : width * 2)
            {
                for (int start = 0; start < items.Length; start += width * 2)
                {
                    int middle = start + width;
                    int end = start + width * 2;
                    if (middle > items.Length)
                    {
                        middle = items.Length;
                    }
                    if (end > items.Length)
                    {
                        end = items.Length;
                    }

                    Merge(source, destination, start, middle, end, ordering);
                }

                TElement[] swap = source;
                source = destination;
                destination = swap;
            }

            if (!object.ReferenceEquals(source, items))
            {
                Array.Copy(source, 0, items, 0, items.Length);
            }
        }

        private static void Merge(TElement[] source, TElement[] destination, int start, int middle, int end, IOrdering<TElement> ordering)
        {
            int left = start;
            int right = middle;
            int destinationIndex = start;

            while (left < middle && right < end)
            {
                if (ordering.Compare(source[left], source[right]) <= 0)
                {
                    destination[destinationIndex++] = source[left++];
                }
                else
                {
                    destination[destinationIndex++] = source[right++];
                }
            }

            while (left < middle)
            {
                destination[destinationIndex++] = source[left++];
            }
            while (right < end)
            {
                destination[destinationIndex++] = source[right++];
            }
        }
    }
}
