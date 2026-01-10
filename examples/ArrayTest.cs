using System;

class ArrayTest
{
    static void Main()
    {
        Console.WriteLine("=== Array Test ===");
        
        // Test integer array
        int[] numbers = new int[5];
        numbers[0] = 10;
        numbers[1] = 20;
        numbers[2] = 30;
        numbers[3] = 40;
        numbers[4] = 50;
        
        Console.WriteLine("Array length: " + numbers.Length);
        
        int sum = 0;
        for (int i = 0; i < numbers.Length; i++)
        {
            sum = sum + numbers[i];
        }
        Console.WriteLine("Sum: " + sum);
        
        Console.WriteLine("=== Array Test Complete ===");
    }
}
