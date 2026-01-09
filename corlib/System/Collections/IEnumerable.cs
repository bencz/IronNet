namespace System.Collections
{
    /// <summary>
    /// Exposes an enumerator, which supports a simple iteration over a non-generic collection
    /// </summary>
    public interface IEnumerable
    {
        IEnumerator GetEnumerator();
    }

    /// <summary>
    /// Supports a simple iteration over a non-generic collection
    /// </summary>
    public interface IEnumerator
    {
        object Current { get; }
        bool MoveNext();
        void Reset();
    }

    /// <summary>
    /// Defines size, enumerators, and synchronization methods for all nongeneric collections
    /// </summary>
    public interface ICollection : IEnumerable
    {
        int Count { get; }
        bool IsSynchronized { get; }
        object SyncRoot { get; }
        void CopyTo(Array array, int index);
    }

    /// <summary>
    /// Represents a non-generic collection of objects that can be individually accessed by index
    /// </summary>
    public interface IList : ICollection
    {
        object this[int index] { get; set; }
        bool IsFixedSize { get; }
        bool IsReadOnly { get; }
        int Add(object value);
        void Clear();
        bool Contains(object value);
        int IndexOf(object value);
        void Insert(int index, object value);
        void Remove(object value);
        void RemoveAt(int index);
    }

    /// <summary>
    /// Represents a nongeneric collection of key/value pairs
    /// </summary>
    public interface IDictionary : ICollection
    {
        object this[object key] { get; set; }
        ICollection Keys { get; }
        ICollection Values { get; }
        bool IsFixedSize { get; }
        bool IsReadOnly { get; }
        void Add(object key, object value);
        void Clear();
        bool Contains(object key);
        new IDictionaryEnumerator GetEnumerator();
        void Remove(object key);
    }

    /// <summary>
    /// Enumerates the elements of a nongeneric dictionary
    /// </summary>
    public interface IDictionaryEnumerator : IEnumerator
    {
        DictionaryEntry Entry { get; }
        object Key { get; }
        object Value { get; }
    }

    /// <summary>
    /// Represents a collection of key/value pairs that are sorted by key
    /// </summary>
    public struct DictionaryEntry
    {
        public object Key { get; set; }
        public object Value { get; set; }

        public DictionaryEntry(object key, object value)
        {
            Key = key;
            Value = value;
        }
    }

    /// <summary>
    /// Exposes a method that compares two objects
    /// </summary>
    public interface IComparer
    {
        int Compare(object x, object y);
    }

    /// <summary>
    /// Defines methods to support the comparison of objects for equality
    /// </summary>
    public interface IEqualityComparer
    {
        bool Equals(object x, object y);
        int GetHashCode(object obj);
    }
}

namespace System.Collections.Generic
{
    /// <summary>
    /// Exposes the enumerator, which supports a simple iteration over a collection of a specified type
    /// </summary>
    public interface IEnumerable<out T> : IEnumerable
    {
        new IEnumerator<T> GetEnumerator();
    }

    /// <summary>
    /// Supports a simple iteration over a generic collection
    /// </summary>
    public interface IEnumerator<out T> : IEnumerator, IDisposable
    {
        new T Current { get; }
    }

    /// <summary>
    /// Defines methods to manipulate generic collections
    /// </summary>
    public interface ICollection<T> : IEnumerable<T>
    {
        int Count { get; }
        bool IsReadOnly { get; }
        void Add(T item);
        void Clear();
        bool Contains(T item);
        void CopyTo(T[] array, int arrayIndex);
        bool Remove(T item);
    }

    /// <summary>
    /// Represents a collection of objects that can be individually accessed by index
    /// </summary>
    public interface IList<T> : ICollection<T>
    {
        T this[int index] { get; set; }
        int IndexOf(T item);
        void Insert(int index, T item);
        void RemoveAt(int index);
    }

    /// <summary>
    /// Represents a generic collection of key/value pairs
    /// </summary>
    public interface IDictionary<TKey, TValue> : ICollection<KeyValuePair<TKey, TValue>>
    {
        TValue this[TKey key] { get; set; }
        ICollection<TKey> Keys { get; }
        ICollection<TValue> Values { get; }
        void Add(TKey key, TValue value);
        bool ContainsKey(TKey key);
        bool Remove(TKey key);
        bool TryGetValue(TKey key, out TValue value);
    }

    /// <summary>
    /// Defines a generalized comparison method
    /// </summary>
    public interface IComparer<in T>
    {
        int Compare(T x, T y);
    }

    /// <summary>
    /// Defines methods to support the comparison of objects for equality
    /// </summary>
    public interface IEqualityComparer<in T>
    {
        bool Equals(T x, T y);
        int GetHashCode(T obj);
    }

    /// <summary>
    /// Defines a key/value pair that can be set or retrieved
    /// </summary>
    public struct KeyValuePair<TKey, TValue>
    {
        public TKey Key { get; }
        public TValue Value { get; }

        public KeyValuePair(TKey key, TValue value)
        {
            Key = key;
            Value = value;
        }

        public override string ToString()
        {
            return "[" + Key?.ToString() + ", " + Value?.ToString() + "]";
        }
    }
}
