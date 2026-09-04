using System;
using System.Threading;

class InterlockedTest
{
    static void Main()
    {
        int intValue = 10;
        Console.WriteLine(Interlocked.Increment(ref intValue) == 11 && intValue == 11);
        Console.WriteLine(Interlocked.Decrement(ref intValue) == 10 && intValue == 10);
        Console.WriteLine(Interlocked.Add(ref intValue, 7) == 17 && intValue == 17);
        Console.WriteLine(Interlocked.Exchange(ref intValue, 3) == 17 && intValue == 3);
        Console.WriteLine(Interlocked.CompareExchange(ref intValue, 9, 3) == 3 && intValue == 9);
        Console.WriteLine(Interlocked.CompareExchange(ref intValue, 4, 3) == 9 && intValue == 9);

        long longValue = 0x100000000L;
        Console.WriteLine(Interlocked.Increment(ref longValue) == 0x100000001L && longValue == 0x100000001L);
        Console.WriteLine(Interlocked.Decrement(ref longValue) == 0x100000000L && longValue == 0x100000000L);
        Console.WriteLine(Interlocked.Add(ref longValue, 5) == 0x100000005L && longValue == 0x100000005L);
        Console.WriteLine(Interlocked.Exchange(ref longValue, 2) == 0x100000005L && longValue == 2);
        Console.WriteLine(Interlocked.CompareExchange(ref longValue, 8, 2) == 2 && longValue == 8);

        object first = new object();
        object second = new object();
        object value = first;
        Console.WriteLine(object.ReferenceEquals(Interlocked.Exchange(ref value, second), first) && object.ReferenceEquals(value, second));
        Console.WriteLine(object.ReferenceEquals(Interlocked.CompareExchange(ref value, first, second), second) && object.ReferenceEquals(value, first));

        Interlocked.MemoryBarrier();
        Thread.Sleep(0);
        Console.WriteLine(true);
    }
}
