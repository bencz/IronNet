using System;

struct MultidimensionalCell
{
    public object Reference;
    public int Value;
}

class MultidimensionalPayload
{
    public int Value;

    public MultidimensionalPayload(int value)
    {
        Value = value;
    }
}

class MultidimensionalArrayTest
{
    static void Main()
    {
        int[,] matrix = new int[2, 3];
        matrix[0, 0] = 11;
        matrix[1, 2] = 37;
        matrix[1, 2]++;

        if (matrix.Length != 6 || matrix.Rank != 2 || matrix.GetLength(0) != 2 || matrix.GetLength(1) != 3 || matrix[0, 0] != 11 || matrix[1, 2] != 38)
        {
            throw new Exception("Compiler multidimensional array access failed");
        }

        int sum = 0;
        foreach (int value in matrix)
        {
            sum += value;
        }
        if (sum != 49)
        {
            throw new Exception("Multidimensional array enumeration failed");
        }

        if (!typeof(object[,]).IsAssignableFrom(typeof(string[,])) || typeof(object[]).IsAssignableFrom(typeof(string[,])))
        {
            throw new Exception("Multidimensional array assignability failed");
        }

        MultidimensionalCell[,] cells = new MultidimensionalCell[2, 2];
        cells[1, 0].Value = 91;
        cells[1, 0].Reference = new MultidimensionalPayload(1234);
        GC.Collect();

        MultidimensionalPayload retained = (MultidimensionalPayload)cells[1, 0].Reference;
        if (cells[1, 0].Value != 91 || retained.Value != 1234)
        {
            throw new Exception("Multidimensional value-type storage or GC scanning failed");
        }

        object[] references = new object[1];
        ref object referenceSlot = ref references[0];
        referenceSlot = new MultidimensionalPayload(2468);
        references = null;
        GC.Collect();

        MultidimensionalPayload interiorRetained = (MultidimensionalPayload)referenceSlot;
        if (interiorRetained.Value != 2468)
        {
            throw new Exception("Managed interior pointer did not retain its owning array");
        }

        Array reflected = Array.CreateInstance(typeof(string), new int[] { 2, 3 }, new int[] { -1, 4 });
        reflected.SetValue("edge", new int[] { 0, 6 });
        if (reflected.Rank != 2 || reflected.Length != 6 || reflected.GetLowerBound(0) != -1 || reflected.GetUpperBound(0) != 0 ||
            reflected.GetLowerBound(1) != 4 || reflected.GetUpperBound(1) != 6 || (string)reflected.GetValue(0, 6) != "edge")
        {
            throw new Exception("Array.CreateInstance bounds or indexed access failed");
        }

        int populated = 0;
        foreach (object value in reflected)
        {
            if (value != null)
            {
                if ((string)value != "edge")
                {
                    throw new Exception("Array enumerator returned an unexpected value");
                }
                populated++;
            }
        }
        if (populated != 1)
        {
            throw new Exception("Array enumerator did not traverse every multidimensional element");
        }

        Array vector = Array.CreateInstance(typeof(int), 4);
        vector.SetValue(77, 3);
        if (vector.Rank != 1 || vector.GetType() != typeof(int[]) || (int)vector.GetValue(3) != 77)
        {
            throw new Exception("Array.CreateInstance vector compatibility failed");
        }

        bool caughtIndex = false;
        try
        {
            reflected.GetValue(20, 20);
        }
        catch (IndexOutOfRangeException)
        {
            caughtIndex = true;
        }
        if (!caughtIndex)
        {
            throw new Exception("InternalCall index exception was not propagated");
        }

        bool caughtCast = false;
        try
        {
            reflected.SetValue(new object(), 0, 4);
        }
        catch (InvalidCastException)
        {
            caughtCast = true;
        }
        if (!caughtCast)
        {
            throw new Exception("InternalCall cast exception was not propagated");
        }

        bool caughtOverflow = false;
        int negativeLength = -1;
        try
        {
            int[,] invalid = new int[negativeLength, 2];
            Console.WriteLine(invalid.Length);
        }
        catch (OverflowException)
        {
            caughtOverflow = true;
        }
        if (!caughtOverflow)
        {
            throw new Exception("Multidimensional constructor overflow was not propagated");
        }

        bool caughtArgument = false;
        try
        {
            Array.CreateInstance(typeof(int), negativeLength);
        }
        catch (ArgumentOutOfRangeException)
        {
            caughtArgument = true;
        }
        if (!caughtArgument)
        {
            throw new Exception("Static InternalCall argument exception was not propagated");
        }

        bool caughtNull = false;
        try
        {
            Array.Clear(null, 0, 0);
        }
        catch (ArgumentNullException)
        {
            caughtNull = true;
        }
        if (!caughtNull)
        {
            throw new Exception("InternalCall null-argument exception was not propagated");
        }

        Console.WriteLine("Multidimensional array tests passed");
    }
}
