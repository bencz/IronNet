using System;

// Generic class
class Box<T>
{
    private T value;
    
    public Box(T val)
    {
        value = val;
    }
    
    public T GetValue()
    {
        return value;
    }
    
    public void SetValue(T val)
    {
        value = val;
    }
}

// Generic class with multiple type parameters
class Pair<T1, T2>
{
    private T1 first;
    private T2 second;
    
    public Pair(T1 f, T2 s)
    {
        first = f;
        second = s;
    }
    
    public T1 GetFirst()
    {
        return first;
    }
    
    public T2 GetSecond()
    {
        return second;
    }
}

class GenericTest
{
    // Generic method
    static void PrintValue<T>(T value)
    {
        Console.Write("Value: ");
        Console.WriteLine(value.ToString());
    }
    
    static void Main()
    {
        Console.WriteLine("=== Generic Test ===");
        
        // Test generic class with int
        Console.WriteLine("Testing Box<int>:");
        Box<int> intBox = new Box<int>(42);
        Console.Write("Box contains: ");
        Console.WriteLine(intBox.GetValue());
        
        intBox.SetValue(100);
        Console.Write("After SetValue: ");
        Console.WriteLine(intBox.GetValue());
        
        // Test generic class with string
        Console.WriteLine("Testing Box<string>:");
        Box<string> strBox = new Box<string>("Hello");
        Console.Write("Box contains: ");
        Console.WriteLine(strBox.GetValue());
        
        // Test Pair
        Console.WriteLine("Testing Pair<int, string>:");
        Pair<int, string> pair = new Pair<int, string>(1, "One");
        Console.Write("First: ");
        Console.WriteLine(pair.GetFirst());
        Console.Write("Second: ");
        Console.WriteLine(pair.GetSecond());
        
        // Test generic method
        Console.WriteLine("Testing generic method:");
        PrintValue<int>(123);
        PrintValue<string>("Test");
        
        Console.WriteLine("=== Generic Test Complete ===");
    }
}
