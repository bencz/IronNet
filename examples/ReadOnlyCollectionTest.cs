using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;

public static class ReadOnlyCollectionTest
{
    private sealed class ExposedCollection<T> : ReadOnlyCollection<T>
    {
        public ExposedCollection(IList<T> list) : base(list)
        {
        }

        public IList<T> Underlying => Items;
    }

    private sealed class GenericOnlyList<T> : IList<T>
    {
        private readonly List<T> _list = new List<T>();

        public int Count => _list.Count;
        public bool IsReadOnly => false;

        public T this[int index]
        {
            get => _list[index];
            set => _list[index] = value;
        }

        public void Add(T item)
        {
            _list.Add(item);
        }

        public void Clear()
        {
            _list.Clear();
        }

        public bool Contains(T item)
        {
            return _list.Contains(item);
        }

        public void CopyTo(T[] array, int index)
        {
            _list.CopyTo(array, index);
        }

        public bool Remove(T item)
        {
            return _list.Remove(item);
        }

        public int IndexOf(T item)
        {
            return _list.IndexOf(item);
        }

        public void Insert(int index, T item)
        {
            _list.Insert(index, item);
        }

        public void RemoveAt(int index)
        {
            _list.RemoveAt(index);
        }

        public IEnumerator<T> GetEnumerator()
        {
            return _list.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return ((IEnumerable)_list).GetEnumerator();
        }
    }

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    private static void Throws<TException>(Action action) where TException : Exception
    {
        Exception caught = null;
        try
        {
            action();
        }
        catch (Exception exception)
        {
            caught = exception;
        }

        Require(caught != null && caught.GetType() == typeof(TException), "Expected exception: " + typeof(TException).FullName);
    }

    private static void TestViewsAndInterfaces()
    {
        List<string> list = new List<string>();
        list.Add("first");
        ExposedCollection<string> view = new ExposedCollection<string>(list);
        IReadOnlyList<string> readOnly = view;
        Require(object.ReferenceEquals(view.Underlying, list), "Protected Items does not expose the supplied list.");
        Require(readOnly.Count == 1 && readOnly[0] == "first", "Read-only list interface failed.");
        list.Add("second");
        list[0] = "changed";
        Require(view.Count == 2 && view[0] == "changed", "Read-only collection is a snapshot instead of a view.");
        Require(view.Contains("second") && view.IndexOf("second") == 1, "Lookup did not delegate to the list.");
        Require(((IReadOnlyList<string>)list)[1] == "second", "List does not expose IReadOnlyList.");
        Require(list.AsReadOnly()[0] == "changed", "List.AsReadOnly failed.");
        Require(object.ReferenceEquals(((ICollection)view).SyncRoot, ((ICollection)list).SyncRoot), "SyncRoot was not shared with the list.");

        GenericOnlyList<int> genericOnly = new GenericOnlyList<int>();
        genericOnly.Add(7);
        ReadOnlyCollection<int> genericView = new ReadOnlyCollection<int>(genericOnly);
        Require(object.ReferenceEquals(((ICollection)genericView).SyncRoot, genericView), "Generic-only list SyncRoot is not stable.");
        Require(genericView[0] == 7 && !((ICollection)genericView).IsSynchronized, "Generic-only list wrapping failed.");

        string[] array = new string[] { "array" };
        ReadOnlyCollection<string> arrayView = Array.AsReadOnly(array);
        array[0] = "updated";
        Require(arrayView[0] == "updated" && arrayView.Count == 1, "Array.AsReadOnly lost live array contents.");
        IReadOnlyList<object> covariantArray = (IReadOnlyList<object>)(object)array;
        Require(covariantArray.Count == 1 && (string)covariantArray[0] == "updated", "Array read-only interface covariance failed.");
        IReadOnlyList<object> covariantView = view;
        Require(covariantView.Count == 2 && (string)covariantView[1] == "second", "Collection read-only interface covariance failed.");
        Require(!((object)new int[1] is IReadOnlyList<object>), "Value-type array gained reference covariance.");
        Require(!((object)new int[1, 1] is IReadOnlyList<int>), "Multidimensional array gained vector-only interfaces.");
        Throws<ArgumentNullException>(() => new ReadOnlyCollection<int>(null));
        Throws<ArgumentNullException>(() => Array.AsReadOnly<int>(null));
    }

    private static void TestMutationAndLookup()
    {
        ReadOnlyCollection<int> view = Array.AsReadOnly(new int[] { 1, 2 });
        IList<int> generic = view;
        IList nongeneric = view;
        Require(generic.IsReadOnly && nongeneric.IsReadOnly && nongeneric.IsFixedSize, "Read-only flags are incorrect.");

        Action[] mutations = new Action[] {
            () => generic.Add(3),
            () => generic.Clear(),
            () => generic.Remove(9),
            () => generic.Insert(-1, 4),
            () => generic.RemoveAt(-1),
            () => generic[-1] = 5,
            () => nongeneric.Add("wrong-type"),
            () => nongeneric.Clear(),
            () => nongeneric.Remove("wrong-type"),
            () => nongeneric.Insert(-1, null),
            () => nongeneric.RemoveAt(-1),
            () => nongeneric[-1] = null
        };
        foreach (Action mutation in mutations)
        {
            Throws<NotSupportedException>(mutation);
        }

        Require(view.Count == 2 && view[0] == 1, "Rejected mutation changed the collection.");
        Require(!nongeneric.Contains(null) && !nongeneric.Contains("wrong-type") && nongeneric.IndexOf("wrong-type") == -1, "Incompatible lookup threw or matched.");
        Require(nongeneric.Contains(2) && nongeneric.IndexOf(2) == 1, "Boxed value lookup failed.");
        IList nullable = Array.AsReadOnly(new int?[] { null, 8 });
        Require(nullable.Contains(null) && nullable.IndexOf(null) == 0 && nullable.Contains(8), "Nullable lookup compatibility failed.");
        IList references = Array.AsReadOnly(new string[] { null, "value" });
        Require(references.Contains(null) && !references.Contains(1), "Reference lookup compatibility failed.");
    }

    private static void TestCopying()
    {
        ReadOnlyCollection<int> view = Array.AsReadOnly(new int[] { 4, 5 });
        int[] numbers = new int[4];
        view.CopyTo(numbers, 1);
        Require(numbers[1] == 4 && numbers[2] == 5 && numbers[3] == 0, "Typed CopyTo failed.");
        ICollection collection = view;
        object[] objects = new object[3];
        collection.CopyTo(objects, 1);
        Require((int)objects[1] == 4 && (int)objects[2] == 5, "Boxing CopyTo failed.");
        Throws<ArgumentNullException>(() => collection.CopyTo(null, 0));
        Throws<ArgumentOutOfRangeException>(() => collection.CopyTo(numbers, -1));
        Throws<ArgumentException>(() => collection.CopyTo(numbers, int.MaxValue));
        Throws<ArgumentException>(() => collection.CopyTo(new int[1], 0));
        Throws<ArgumentException>(() => collection.CopyTo(new int[2, 2], 0));
        Throws<ArgumentException>(() => collection.CopyTo(Array.CreateInstance(typeof(int), new int[] { 2 }, new int[] { 1 }), 0));
        Throws<ArgumentException>(() => collection.CopyTo(new long[2], 0));
        Throws<ArgumentException>(() => collection.CopyTo(new string[2], 0));
        ICollection empty = Array.AsReadOnly(new int[0]);
        Throws<ArgumentException>(() => empty.CopyTo(new string[0], 0));
        empty.CopyTo(new object[0], 0);

        ReadOnlyCollection<IComparable> mixed = Array.AsReadOnly(new IComparable[] { "fits", 10 });
        string[] narrowed = new string[2];
        Throws<InvalidCastException>(() => ((ICollection)mixed).CopyTo(narrowed, 0));
        Require(narrowed[0] == "fits" && narrowed[1] == null, "Narrowing CopyTo did not preserve its successful prefix.");
    }

    private static void TestEnumeration()
    {
        List<int> list = new List<int>();
        list.Add(1);
        list.Add(2);
        ReadOnlyCollection<int> view = list.AsReadOnly();
        int sum = 0;
        foreach (int value in view)
        {
            sum += value;
        }

        Require(sum == 3, "Generic enumeration failed.");
        IEnumerator nongeneric = ((IEnumerable)view).GetEnumerator();
        Require(nongeneric.MoveNext() && (int)nongeneric.Current == 1, "Nongeneric enumeration failed.");
        IEnumerator<int> enumerator = view.GetEnumerator();
        Require(enumerator.MoveNext(), "Enumerator failed to start.");
        list.Add(3);
        Throws<InvalidOperationException>(() => enumerator.MoveNext());
        enumerator.Dispose();
    }

    public static int Main()
    {
        TestViewsAndInterfaces();
        TestMutationAndLookup();
        TestCopying();
        TestEnumeration();
        Console.WriteLine("ReadOnlyCollectionTest passed");
        return 0;
    }
}
