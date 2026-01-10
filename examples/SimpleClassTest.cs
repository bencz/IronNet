using System;

class SimpleClass
{
    public int value;
    
    public SimpleClass()
    {
        value = 42;
    }
    
    public void Print()
    {
        Console.Write("Value: ");
        Console.WriteLine(value);
    }
}

class SimpleClassTest
{
    static void Main()
    {
        Console.WriteLine("=== Simple Class Test ===");
        
        SimpleClass obj = new SimpleClass();
        obj.Print();
        
        Console.WriteLine("=== Test Complete ===");
    }
}
