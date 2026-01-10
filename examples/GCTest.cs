using System;

class GCTest
{
    static void Main()
    {
        Console.WriteLine("=== GC Test ===");
        
        // Get initial memory
        long initialMem = GC.GetTotalMemory(false);
        Console.WriteLine("Initial memory: " + initialMem + " bytes");
        
        // Allocate some objects
        Console.WriteLine("Allocating objects...");
        for (int i = 0; i < 100; i++)
        {
            int[] arr = new int[100];
            arr[0] = i;
        }
        
        long afterAlloc = GC.GetTotalMemory(false);
        Console.WriteLine("After allocation: " + afterAlloc + " bytes");
        
        // Force GC
        Console.WriteLine("Forcing GC collection...");
        GC.Collect();
        
        long afterGC = GC.GetTotalMemory(false);
        Console.WriteLine("After GC: " + afterGC + " bytes");
        
        Console.WriteLine("=== GC Test Complete ===");
    }
}
