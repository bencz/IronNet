using System;

class LoopTest
{
    static void Main()
    {
        Console.WriteLine("=== Loop Test ===");
        
        // For loop
        Console.WriteLine("For loop (0-4):");
        for (int i = 0; i < 5; i++)
        {
            Console.Write("  i = ");
            Console.WriteLine(i);
        }
        
        // While loop
        Console.WriteLine("While loop (countdown):");
        int count = 5;
        while (count > 0)
        {
            Console.Write("  count = ");
            Console.WriteLine(count);
            count = count - 1;
        }
        
        // Nested loops
        Console.WriteLine("Nested loops (multiplication table 3x3):");
        for (int x = 1; x <= 3; x++)
        {
            for (int y = 1; y <= 3; y++)
            {
                int result = x * y;
                Console.Write("  ");
                Console.Write(x);
                Console.Write(" * ");
                Console.Write(y);
                Console.Write(" = ");
                Console.WriteLine(result);
            }
        }
        
        Console.WriteLine("=== Loop Test Complete ===");
    }
}
