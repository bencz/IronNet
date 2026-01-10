using System;

// ============================================================================
// INTERFACES
// ============================================================================

public interface IIdentifiable
{
    int Id { get; }
    string GetIdentifier();
}

public interface INameable
{
    string Name { get; }
}

public interface IDescribable
{
    string GetDescription();
}

public interface IComparable<T>
{
    int CompareTo(T other);
}

public interface ICloneable<T>
{
    T Clone();
}

public interface ISerializable
{
    string Serialize();
    void Deserialize(string data);
}

// Combined interface inheriting multiple interfaces
public interface IEntity : IIdentifiable, INameable, IDescribable
{
    void Update();
}

// ============================================================================
// ABSTRACT BASE CLASSES
// ============================================================================

public abstract class BaseEntity : IEntity
{
    protected int id;
    protected string name;
    
    public int Id { get { return id; } }
    public string Name { get { return name; } }
    
    public BaseEntity(int id, string name)
    {
        this.id = id;
        this.name = name;
    }
    
    public virtual string GetIdentifier()
    {
        return "Entity";
    }
    
    public abstract string GetDescription();
    public abstract void Update();
}

// ============================================================================
// GENERIC BASE CLASS
// ============================================================================

public class Repository<T> where T : IIdentifiable
{
    private T[] items;
    private int count;
    private int capacity;
    
    public Repository()
    {
        capacity = 4;
        items = new T[capacity];
        count = 0;
    }
    
    public void Add(T item)
    {
        if (count >= capacity)
        {
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
    
    public T GetByIndex(int index)
    {
        if (index >= 0 && index < count)
        {
            return items[index];
        }
        return default(T);
    }
    
    public T Get(int index)
    {
        return items[index];
    }
    
    public int Count { get { return count; } }
    
    public void ForEach(Action<T> action)
    {
        for (int i = 0; i < count; i++)
        {
            action(items[i]);
        }
    }
}

// ============================================================================
// CONCRETE CLASSES WITH MULTIPLE INHERITANCE
// ============================================================================

public class Person : BaseEntity, IComparable<Person>, ICloneable<Person>
{
    public int Age;
    public string Email;
    
    public Person(int id, string name, int age, string email) : base(id, name)
    {
        Age = age;
        Email = email;
    }
    
    public override string GetDescription()
    {
        return name;
    }
    
    public override void Update()
    {
        Age = Age + 1;
    }
    
    public int CompareTo(Person other)
    {
        if (Age < other.Age) return -1;
        if (Age > other.Age) return 1;
        return 0;
    }
    
    public Person Clone()
    {
        return new Person(id, name, Age, Email);
    }
}

public class Employee : Person, ISerializable
{
    public string Department;
    public int Salary;
    
    public Employee(int id, string name, int age, string email, string dept, int salary) 
        : base(id, name, age, email)
    {
        Department = dept;
        Salary = salary;
    }
    
    public override string GetIdentifier()
    {
        return "EMP";
    }
    
    public override string GetDescription()
    {
        return Department;
    }
    
    public string Serialize()
    {
        return Department;
    }
    
    public void Deserialize(string data)
    {
        // Simplified - just update name
        name = data;
    }
}

public class Manager : Employee
{
    public int TeamSize;
    public int Budget;
    
    public Manager(int id, string name, int age, string email, string dept, int salary, int teamSize, int budget)
        : base(id, name, age, email, dept, salary)
    {
        TeamSize = teamSize;
        Budget = budget;
    }
    
    public override string GetIdentifier()
    {
        return "MGR";
    }
    
    public override string GetDescription()
    {
        return Department;
    }
    
    public void AddToTeam()
    {
        TeamSize = TeamSize + 1;
    }
}

// ============================================================================
// GENERIC CLASS WITH INTERFACE CONSTRAINT
// ============================================================================

public class ComparableList<T> where T : IComparable<T>
{
    private T[] items;
    private int count;
    
    public ComparableList()
    {
        items = new T[10];
        count = 0;
    }
    
    public void Add(T item)
    {
        items[count] = item;
        count++;
    }
    
    public T Get(int index)
    {
        return items[index];
    }
    
    public int Count { get { return count; } }
    
    public T GetFirst()
    {
        if (count == 0) return default(T);
        return items[0];
    }
    
    public T GetLast()
    {
        if (count == 0) return default(T);
        return items[count - 1];
    }
}

// ============================================================================
// DELEGATE FOR CALLBACKS
// ============================================================================

public delegate void Action<T>(T item);
public delegate bool Predicate<T>(T item);
public delegate TResult Func<T, TResult>(T item);

// ============================================================================
// TEST PROGRAM
// ============================================================================

class ComplexInheritanceTest
{
    static void Main()
    {
        Console.WriteLine("=== Complex Inheritance Test ===");
        
        // Test 1: Basic inheritance chain
        Console.WriteLine("Test 1: Inheritance chain (Person -> Employee -> Manager)");
        Person p1 = new Person(1, "Alice", 30, "alice@test.com");
        Employee e1 = new Employee(2, "Bob", 35, "bob@test.com", "Engineering", 75000);
        Manager m1 = new Manager(3, "Carol", 45, "carol@test.com", "Engineering", 120000, 5, 500000);
        
        Console.Write("Person: ");
        Console.WriteLine(p1.GetDescription());
        Console.Write("Employee: ");
        Console.WriteLine(e1.GetDescription());
        Console.Write("Manager: ");
        Console.WriteLine(m1.GetDescription());
        
        // Test 2: Virtual method override
        Console.WriteLine("Test 2: Virtual method override (GetIdentifier)");
        Console.Write("Person ID: ");
        Console.WriteLine(p1.GetIdentifier());
        Console.Write("Employee ID: ");
        Console.WriteLine(e1.GetIdentifier());
        Console.Write("Manager ID: ");
        Console.WriteLine(m1.GetIdentifier());
        
        // Test 3: Interface implementation
        Console.WriteLine("Test 3: ISerializable interface");
        Console.Write("Serialized Employee: ");
        Console.WriteLine(e1.Serialize());
        Console.Write("Serialized Manager: ");
        Console.WriteLine(m1.Serialize());
        
        // Test 4: IComparable<T> and ICloneable<T>
        Console.WriteLine("Test 4: IComparable and ICloneable");
        Person p2 = new Person(4, "Dave", 25, "dave@test.com");
        int cmp = p1.CompareTo(p2);
        Console.Write("Alice.CompareTo(Dave): ");
        Console.WriteLine(cmp);
        
        Person p1Clone = p1.Clone();
        Console.Write("Cloned person: ");
        Console.WriteLine(p1Clone.GetDescription());
        
        // Test 5: Generic Repository with IIdentifiable constraint
        Console.WriteLine("Test 5: Generic Repository<T> where T : IIdentifiable");
        Repository<Person> personRepo = new Repository<Person>();
        personRepo.Add(p1);
        personRepo.Add(p2);
        personRepo.Add(e1);
        personRepo.Add(m1);
        
        Console.Write("Repository count: ");
        Console.WriteLine(personRepo.Count);
        
        Person found = personRepo.GetByIndex(1);
        Console.Write("Found at index 1: ");
        Console.WriteLine(found.GetDescription());
        
        // Test 6: ComparableList with GetFirst/GetLast
        Console.WriteLine("Test 6: ComparableList<Person> with GetFirst/GetLast");
        ComparableList<Person> comparableList = new ComparableList<Person>();
        comparableList.Add(p1);  // Age 30
        comparableList.Add(p2);  // Age 25
        comparableList.Add(e1);  // Age 35
        comparableList.Add(m1);  // Age 45
        
        Person first = comparableList.GetFirst();
        Person last = comparableList.GetLast();
        Console.Write("First: ");
        Console.WriteLine(first.GetDescription());
        Console.Write("Last: ");
        Console.WriteLine(last.GetDescription());
        
        // Test 7: Polymorphism with base interface
        Console.WriteLine("Test 7: Polymorphism through IEntity interface");
        IEntity[] entities = new IEntity[3];
        entities[0] = p1;
        entities[1] = e1;
        entities[2] = m1;
        
        for (int i = 0; i < 3; i++)
        {
            Console.Write("Entity ");
            Console.Write(i);
            Console.Write(": ");
            Console.WriteLine(entities[i].GetDescription());
        }
        
        // Test 8: Update through abstract method
        Console.WriteLine("Test 8: Abstract method Update()");
        Console.Write("Before update - Alice age: ");
        Console.WriteLine(p1.Age);
        p1.Update();
        Console.Write("After update - Alice age: ");
        Console.WriteLine(p1.Age);
        
        // Test 9: Manager-specific method
        Console.WriteLine("Test 9: Derived class specific method");
        Console.Write("Before AddToTeam - Team size: ");
        Console.WriteLine(m1.TeamSize);
        m1.AddToTeam();
        m1.AddToTeam();
        Console.Write("After AddToTeam - Team size: ");
        Console.WriteLine(m1.TeamSize);
        
        // Test 10: Multiple interface casting
        Console.WriteLine("Test 10: Multiple interface casting");
        IIdentifiable identifiable = m1;
        INameable nameable = m1;
        IDescribable describable = m1;
        ISerializable serializable = m1;
        
        Console.Write("As IIdentifiable: ");
        Console.WriteLine(identifiable.GetIdentifier());
        Console.Write("As INameable: ");
        Console.WriteLine(nameable.Name);
        Console.Write("As IDescribable: ");
        Console.WriteLine(describable.GetDescription());
        Console.Write("As ISerializable: ");
        Console.WriteLine(serializable.Serialize());
        
        Console.WriteLine("=== Complex Inheritance Test Complete ===");
    }
}
