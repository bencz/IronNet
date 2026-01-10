using System;

class Point
{
    public int x;
    public int y;
    
    public Point(int x, int y)
    {
        this.x = x;
        this.y = y;
    }
    
    public void Print()
    {
        Console.Write("(");
        Console.Write(x);
        Console.Write(", ");
        Console.Write(y);
        Console.WriteLine(")");
    }
}

class Line
{
    public Point start;
    public Point end;
    
    public Line(Point s, Point e)
    {
        start = s;
        end = e;
    }
    
    public void Print()
    {
        Console.Write("Line from ");
        start.Print();
        Console.Write(" to ");
        end.Print();
    }
}

class ClassFieldTest
{
    static void Main()
    {
        Console.WriteLine("=== Class Field Test ===");
        
        // Test basic object creation and fields
        Console.WriteLine("Creating Point:");
        Point p1 = new Point(10, 20);
        p1.Print();
        
        // Test field modification
        Console.WriteLine("Modifying fields:");
        p1.x = 30;
        p1.y = 40;
        p1.Print();
        
        // Test object references in fields
        Console.WriteLine("Creating Line with Points:");
        Point start = new Point(0, 0);
        Point end = new Point(100, 100);
        Line line = new Line(start, end);
        line.Print();
        
        // Test modifying referenced objects
        Console.WriteLine("Modifying referenced Point:");
        start.x = 5;
        start.y = 5;
        line.Print();
        
        // Test multiple objects
        Console.WriteLine("Multiple objects:");
        Point[] points = new Point[3];
        points[0] = new Point(1, 1);
        points[1] = new Point(2, 2);
        points[2] = new Point(3, 3);
        
        int i = 0;
        while (i < 3)
        {
            Console.Write("Point ");
            Console.Write(i);
            Console.Write(": ");
            points[i].Print();
            i = i + 1;
        }
        
        Console.WriteLine("=== Class Field Test Complete ===");
    }
}
