using System;

// Interface definitions
interface IShape
{
    int GetArea();
    void Draw();
}

interface IMovable
{
    void Move(int x, int y);
}

// Class implementing single interface
class Rectangle : IShape
{
    private int width;
    private int height;
    
    public Rectangle(int w, int h)
    {
        width = w;
        height = h;
    }
    
    public int GetArea()
    {
        return width * height;
    }
    
    public void Draw()
    {
        Console.Write("Drawing rectangle ");
        Console.Write(width);
        Console.Write("x");
        Console.WriteLine(height);
    }
}

// Class implementing multiple interfaces
class Circle : IShape, IMovable
{
    private int radius;
    private int x;
    private int y;
    
    public Circle(int r)
    {
        radius = r;
        x = 0;
        y = 0;
    }
    
    public int GetArea()
    {
        return 3 * radius * radius; // Simplified PI
    }
    
    public void Draw()
    {
        Console.Write("Drawing circle with radius ");
        Console.WriteLine(radius);
    }
    
    public void Move(int newX, int newY)
    {
        x = newX;
        y = newY;
        Console.Write("Circle moved to (");
        Console.Write(x);
        Console.Write(", ");
        Console.Write(y);
        Console.WriteLine(")");
    }
}

class InterfaceTest
{
    static void PrintArea(IShape shape)
    {
        Console.Write("Area: ");
        Console.WriteLine(shape.GetArea());
    }
    
    static void Main()
    {
        Console.WriteLine("=== Interface Test ===");
        
        // Test single interface
        Console.WriteLine("Testing Rectangle:");
        Rectangle rect = new Rectangle(5, 3);
        rect.Draw();
        PrintArea(rect);
        
        // Test multiple interfaces
        Console.WriteLine("Testing Circle:");
        Circle circle = new Circle(4);
        circle.Draw();
        PrintArea(circle);
        circle.Move(10, 20);
        
        // Test interface polymorphism
        Console.WriteLine("Testing interface polymorphism:");
        IShape shape1 = rect;
        IShape shape2 = circle;
        shape1.Draw();
        shape2.Draw();
        
        // Test IMovable
        Console.WriteLine("Testing IMovable:");
        IMovable movable = circle;
        movable.Move(5, 5);
        
        Console.WriteLine("=== Interface Test Complete ===");
    }
}
