using System;

class MethodTest
{
    static int Add(int a, int b)
    {
        return a + b;
    }
    
    static int Factorial(int n)
    {
        if (n <= 1)
            return 1;
        return n * Factorial(n - 1);
    }
    
    static int Fibonacci(int n)
    {
        if (n <= 1)
            return n;
        return Fibonacci(n - 1) + Fibonacci(n - 2);
    }
    
    static void PrintMessage(string msg)
    {
        Console.WriteLine("Message: " + msg);
    }
    
    static void Main()
    {
        Console.WriteLine("=== Method Test ===");
        
        // Test simple method
        int sum = Add(10, 20);
        Console.WriteLine("Add(10, 20) = " + sum);
        
        // Test recursive method
        Console.WriteLine("Factorial(5) = " + Factorial(5));
        Console.WriteLine("Factorial(10) = " + Factorial(10));
        
        // Test Fibonacci
        Console.WriteLine("Fibonacci sequence:");
        for (int i = 0; i < 10; i++)
        {
            Console.WriteLine("  Fib(" + i + ") = " + Fibonacci(i));
        }
        
        // Test void method
        PrintMessage("Hello from method!");
        
        Console.WriteLine("=== Method Test Complete ===");
    }
}
