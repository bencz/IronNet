using System;

class DataObject
{
    private int id;
    private int value;
    
    public DataObject(int id, int val)
    {
        this.id = id;
        this.value = val;
    }
    
    public int GetValue()
    {
        return value;
    }
    
    public int GetId()
    {
        return id;
    }
}

class GCStressTest
{
    static void AllocateObjects(int count)
    {
        Console.Write("Allocating ");
        Console.Write(count);
        Console.WriteLine(" objects...");
        
        int i = 0;
        while (i < count)
        {
            DataObject obj = new DataObject(i, i * 10);
            i = i + 1;
        }
    }
    
    static void AllocateAndKeep(int count)
    {
        Console.Write("Allocating and keeping ");
        Console.Write(count);
        Console.WriteLine(" objects...");
        
        // This tests that objects stay alive when referenced
        DataObject first = new DataObject(0, 100);
        DataObject last = null;
        
        int i = 1;
        while (i < count)
        {
            last = new DataObject(i, i * 10);
            i = i + 1;
        }
        
        Console.Write("First object value: ");
        Console.WriteLine(first.GetValue());
        
        if (last != null)
        {
            Console.Write("Last object value: ");
            Console.WriteLine(last.GetValue());
        }
    }
    
    static void Main()
    {
        Console.WriteLine("=== GC Stress Test ===");
        
        // Phase 1: Allocate many temporary objects
        Console.WriteLine("Phase 1: Temporary allocations");
        AllocateObjects(100);
        Console.WriteLine("Phase 1 complete");
        
        // Phase 2: Allocate and keep references
        Console.WriteLine("Phase 2: Retained allocations");
        AllocateAndKeep(50);
        Console.WriteLine("Phase 2 complete");
        
        // Phase 3: Nested allocations
        Console.WriteLine("Phase 3: Nested allocations");
        int outer = 0;
        while (outer < 5)
        {
            int inner = 0;
            while (inner < 10)
            {
                DataObject temp = new DataObject(outer * 10 + inner, outer + inner);
                inner = inner + 1;
            }
            outer = outer + 1;
        }
        Console.WriteLine("Phase 3 complete");
        
        // Phase 4: Large batch
        Console.WriteLine("Phase 4: Large batch");
        AllocateObjects(500);
        Console.WriteLine("Phase 4 complete");
        
        Console.WriteLine("=== GC Stress Test Complete ===");
    }
}
