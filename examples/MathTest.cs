using System;

class MathTest
{
    static void Main()
    {
        Console.WriteLine("=== Math Test ===");
        
        // Test basic math operations
        double a = 16.0;
        double b = 2.0;
        
        Console.WriteLine("a = 16.0, b = 2.0");
        Console.WriteLine("Sqrt(a) = " + Math.Sqrt(a));
        Console.WriteLine("Pow(b, 10) = " + Math.Pow(b, 10));
        Console.WriteLine("Abs(-42) = " + Math.Abs(-42));
        Console.WriteLine("Max(5, 10) = " + Math.Max(5, 10));
        Console.WriteLine("Min(5, 10) = " + Math.Min(5, 10));
        
        // Test trigonometry
        double angle = Math.PI / 4; // 45 degrees
        Console.WriteLine("Sin(PI/4) = " + Math.Sin(angle));
        Console.WriteLine("Cos(PI/4) = " + Math.Cos(angle));
        
        Console.WriteLine("=== Math Test Complete ===");
    }
}
