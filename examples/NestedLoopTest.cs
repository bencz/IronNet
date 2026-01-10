using System;

class NestedLoopTest
{
    static void Main()
    {
        Console.WriteLine("Nested loop test:");
        for (int x = 1; x <= 2; x++)
        {
            for (int y = 1; y <= 2; y++)
            {
                int result = x * y;
                Console.WriteLine(result);
            }
        }
        Console.WriteLine("Done");
    }
}
