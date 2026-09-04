using System;

class IndirectObjectTest
{
    static T Read<T>(ref T value)
    {
        return value;
    }

    static void Write<T>(ref T destination, T value)
    {
        destination = value;
    }

    static void Copy<T>(ref T destination, ref T source)
    {
        destination = source;
    }

    static void Clear<T>(ref T value)
    {
        value = default(T);
    }

    static void Main()
    {
        int first = 42;
        int second = 0;
        Console.WriteLine(Read(ref first) == 42);
        Write(ref second, 17);
        Console.WriteLine(second == 17);
        Copy(ref second, ref first);
        Console.WriteLine(second == 42);
        Clear(ref second);
        Console.WriteLine(second == 0);

        long wide = 0x100000002L;
        long wideCopy = 0;
        Console.WriteLine(Read(ref wide) == 0x100000002L);
        Copy(ref wideCopy, ref wide);
        Console.WriteLine(wideCopy == 0x100000002L);
        Clear(ref wideCopy);
        Console.WriteLine(wideCopy == 0);
    }
}
