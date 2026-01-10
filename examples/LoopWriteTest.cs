using System;

class LoopWriteTest
{
    static void Main()
    {
        Console.WriteLine("Start");
        for (int i = 0; i < 3; i++)
        {
            Console.Write("i=");
            Console.Write(i);
            Console.WriteLine("");
        }
        Console.WriteLine("Done");
    }
}
