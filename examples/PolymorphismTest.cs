using System;

// Abstract base class
abstract class Vehicle
{
    protected string brand;
    
    public Vehicle(string brand)
    {
        this.brand = brand;
    }
    
    public abstract void Start();
    public abstract void Stop();
    
    public void ShowBrand()
    {
        Console.Write("Brand: ");
        Console.WriteLine(brand);
    }
}

class Car : Vehicle
{
    private int doors;
    
    public Car(string brand, int doors) : base(brand)
    {
        this.doors = doors;
    }
    
    public override void Start()
    {
        Console.Write(brand);
        Console.WriteLine(" car engine starting...");
    }
    
    public override void Stop()
    {
        Console.Write(brand);
        Console.WriteLine(" car engine stopped");
    }
    
    public void OpenDoors()
    {
        Console.Write("Opening ");
        Console.Write(doors);
        Console.WriteLine(" doors");
    }
}

class Motorcycle : Vehicle
{
    public Motorcycle(string brand) : base(brand)
    {
    }
    
    public override void Start()
    {
        Console.Write(brand);
        Console.WriteLine(" motorcycle revving...");
    }
    
    public override void Stop()
    {
        Console.Write(brand);
        Console.WriteLine(" motorcycle stopped");
    }
}

class PolymorphismTest
{
    static void TestVehicle(Vehicle v)
    {
        v.ShowBrand();
        v.Start();
        v.Stop();
    }
    
    static void Main()
    {
        Console.WriteLine("=== Polymorphism Test ===");
        
        // Create instances
        Car car = new Car("Toyota", 4);
        Motorcycle bike = new Motorcycle("Honda");
        
        // Test direct calls
        Console.WriteLine("Direct calls:");
        car.Start();
        car.OpenDoors();
        car.Stop();
        
        bike.Start();
        bike.Stop();
        
        // Test polymorphic calls
        Console.WriteLine("Polymorphic calls:");
        TestVehicle(car);
        TestVehicle(bike);
        
        // Test array of base type
        Console.WriteLine("Array of vehicles:");
        Vehicle[] vehicles = new Vehicle[2];
        vehicles[0] = car;
        vehicles[1] = bike;
        
        int i = 0;
        while (i < 2)
        {
            Console.Write("Vehicle ");
            Console.Write(i);
            Console.WriteLine(":");
            vehicles[i].Start();
            i = i + 1;
        }
        
        Console.WriteLine("=== Polymorphism Test Complete ===");
    }
}
