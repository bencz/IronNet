using System;

// Generic List implementation
public class SimpleList<T>
{
    private T[] items;
    private int count;
    private int capacity;
    
    public SimpleList()
    {
        capacity = 4;
        items = new T[capacity];
        count = 0;
    }
    
    public void Add(T item)
    {
        if (count >= capacity)
        {
            // Grow array
            capacity = capacity * 2;
            T[] newItems = new T[capacity];
            for (int i = 0; i < count; i++)
            {
                newItems[i] = items[i];
            }
            items = newItems;
        }
        items[count] = item;
        count++;
    }
    
    public T Get(int index)
    {
        return items[index];
    }
    
    public int Count
    {
        get { return count; }
    }
}

// Test class to store in the list
public class Person
{
    public string Name;
    public int Age;
    public int Id;
    
    public Person(string name, int age, int id)
    {
        Name = name;
        Age = age;
        Id = id;
    }
    
    public void Birthday()
    {
        Age = Age + 1;
    }
}

// Generic Pair for testing multiple type parameters
public class KeyValue<TKey, TValue>
{
    public TKey Key;
    public TValue Value;
    
    public KeyValue(TKey key, TValue value)
    {
        Key = key;
        Value = value;
    }
}

class AdvancedGenericTest
{
    static void Main()
    {
        Console.WriteLine("=== Advanced Generic Test ===");
        
        // Test 1: SimpleList<int> with many items
        Console.WriteLine("Test 1: SimpleList<int> with 100 items");
        SimpleList<int> intList = new SimpleList<int>();
        for (int i = 0; i < 100; i++)
        {
            intList.Add(i * 10);
        }
        Console.Write("Count: ");
        Console.WriteLine(intList.Count);
        Console.Write("First: ");
        Console.WriteLine(intList.Get(0));
        Console.Write("Last: ");
        Console.WriteLine(intList.Get(99));
        Console.Write("Middle: ");
        Console.WriteLine(intList.Get(50));
        
        // Test 2: SimpleList<Person> - stress test with objects
        Console.WriteLine("Test 2: SimpleList<Person> with 50 objects");
        SimpleList<Person> people = new SimpleList<Person>();
        for (int i = 0; i < 50; i++)
        {
            Person p = new Person("Person", 20, i);
            people.Add(p);
        }
        Console.Write("People count: ");
        Console.WriteLine(people.Count);
        
        // Modify some objects
        Person first = people.Get(0);
        first.Birthday();
        Person last = people.Get(49);
        last.Birthday();
        last.Birthday();
        
        Console.Write("First person age: ");
        Console.WriteLine(people.Get(0).Age);
        Console.Write("Last person age: ");
        Console.WriteLine(people.Get(49).Age);
        Console.Write("Last person ID: ");
        Console.WriteLine(people.Get(49).Id);
        
        // Test 3: SimpleList<string>
        Console.WriteLine("Test 3: SimpleList<string>");
        SimpleList<string> strings = new SimpleList<string>();
        strings.Add("Hello");
        strings.Add("World");
        strings.Add("Generic");
        strings.Add("Test");
        Console.Write("String count: ");
        Console.WriteLine(strings.Count);
        Console.WriteLine(strings.Get(0));
        Console.WriteLine(strings.Get(3));
        
        // Test 4: Nested generics - SimpleList<KeyValue<int, string>>
        Console.WriteLine("Test 4: Nested generics");
        SimpleList<KeyValue<int, string>> kvList = new SimpleList<KeyValue<int, string>>();
        kvList.Add(new KeyValue<int, string>(1, "One"));
        kvList.Add(new KeyValue<int, string>(2, "Two"));
        kvList.Add(new KeyValue<int, string>(3, "Three"));
        Console.Write("KV count: ");
        Console.WriteLine(kvList.Count);
        KeyValue<int, string> kv = kvList.Get(1);
        Console.Write("Key: ");
        Console.WriteLine(kv.Key);
        Console.Write("Value: ");
        Console.WriteLine(kv.Value);
        
        // Test 5: GC stress - create many temporary objects
        Console.WriteLine("Test 5: GC stress with temporary objects");
        int sum = 0;
        for (int i = 0; i < 200; i++)
        {
            SimpleList<int> temp = new SimpleList<int>();
            temp.Add(i);
            temp.Add(i * 2);
            sum = sum + temp.Get(0) + temp.Get(1);
        }
        Console.Write("Sum: ");
        Console.WriteLine(sum);
        
        Console.WriteLine("=== Advanced Generic Test Complete ===");
    }
}
