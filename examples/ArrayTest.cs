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

        if (numbers.Rank != 1 || numbers.GetLength(0) != 5 || numbers.GetLongLength(0) != 5 || numbers.GetLowerBound(0) != 0 || numbers.GetUpperBound(0) != 4)
        {
            throw new Exception("Array bounds metadata failed");
        }

        Array.Clear(numbers, 1, 3);
        if (numbers[0] != 10 || numbers[1] != 0 || numbers[2] != 0 || numbers[3] != 0 || numbers[4] != 50)
        {
            throw new Exception("Value-type Array.Clear failed");
        }

        string[] words = new string[] { "alpha", "beta" };
        object[] objects = new object[3];
        Array.Copy(words, 0, objects, 1, words.Length);
        if ((string)objects[1] != "alpha" || (string)objects[2] != "beta")
        {
            throw new Exception("Covariant Array.Copy failed");
        }

        object[] compatibleObjects = new object[] { "left", null, "right" };
        string[] strings = new string[3];
        Array.Copy(compatibleObjects, strings, compatibleObjects.Length);
        if (strings[0] != "left" || strings[1] != null || strings[2] != "right")
        {
            throw new Exception("Checked reference Array.Copy failed");
        }

        Array.Clear(strings, 0, 2);
        if (strings[0] != null || strings[1] != null || strings[2] != "right")
        {
            throw new Exception("Reference Array.Clear failed");
        }

        int[] overlap = new int[] { 1, 2, 3, 4, 5 };
        Array.Copy(overlap, 0, overlap, 1, 4);
        if (overlap[0] != 1 || overlap[1] != 1 || overlap[2] != 2 || overlap[3] != 3 || overlap[4] != 4)
        {
            throw new Exception("Overlapping Array.Copy failed");
        }
        
        Console.WriteLine("=== Array Test Complete ===");
    }
}
