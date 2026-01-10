using System;

class ConditionalTest
{
    static void Main()
    {
        Console.WriteLine("=== Conditional Test ===");
        
        // Test if-else
        int x = 10;
        Console.WriteLine("x = " + x);
        
        if (x > 5)
        {
            Console.WriteLine("x > 5: true");
        }
        else
        {
            Console.WriteLine("x > 5: false");
        }
        
        if (x == 10)
        {
            Console.WriteLine("x == 10: true");
        }
        
        // Test comparison operators
        int a = 5;
        int b = 10;
        Console.WriteLine("a = 5, b = 10");
        
        if (a < b) Console.WriteLine("a < b: true");
        if (a <= b) Console.WriteLine("a <= b: true");
        if (b > a) Console.WriteLine("b > a: true");
        if (b >= a) Console.WriteLine("b >= a: true");
        if (a != b) Console.WriteLine("a != b: true");
        
        // Test logical operators
        bool t = true;
        bool f = false;
        
        if (t && t) Console.WriteLine("true && true: true");
        if (t || f) Console.WriteLine("true || false: true");
        if (!f) Console.WriteLine("!false: true");
        
        Console.WriteLine("=== Conditional Test Complete ===");
    }
}
