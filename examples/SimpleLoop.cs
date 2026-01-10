using System;

class SimpleLoop
{
    static void Main()
    {
        Console.WriteLine("Start");
        int i = 0;
        while (i < 3)
        {
            Console.WriteLine("Loop");
            i = i + 1;
        }
        Console.WriteLine("End");
    }
}
