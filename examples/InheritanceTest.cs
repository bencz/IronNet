using System;

// Base class
class Animal
{
    protected string name;
    
    public Animal(string name)
    {
        this.name = name;
    }
    
    public virtual void Speak()
    {
        Console.WriteLine("Animal speaks");
    }
    
    public void Eat()
    {
        Console.Write(name);
        Console.WriteLine(" is eating");
    }
}

// Derived class
class Dog : Animal
{
    public Dog(string name) : base(name)
    {
    }
    
    public override void Speak()
    {
        Console.Write(name);
        Console.WriteLine(" says: Woof!");
    }
    
    public void Fetch()
    {
        Console.Write(name);
        Console.WriteLine(" is fetching");
    }
}

// Another derived class
class Cat : Animal
{
    public Cat(string name) : base(name)
    {
    }
    
    public override void Speak()
    {
        Console.Write(name);
        Console.WriteLine(" says: Meow!");
    }
}

class InheritanceTest
{
    static void Main()
    {
        Console.WriteLine("=== Inheritance Test ===");
        
        // Test basic inheritance
        Console.WriteLine("Creating animals...");
        Dog dog = new Dog("Rex");
        Cat cat = new Cat("Whiskers");
        
        // Test virtual method override
        Console.WriteLine("Testing virtual methods:");
        dog.Speak();
        cat.Speak();
        
        // Test inherited method
        Console.WriteLine("Testing inherited methods:");
        dog.Eat();
        cat.Eat();
        
        // Test derived-only method
        Console.WriteLine("Testing derived-only method:");
        dog.Fetch();
        
        // Test polymorphism
        Console.WriteLine("Testing polymorphism:");
        Animal animal1 = dog;
        Animal animal2 = cat;
        animal1.Speak();
        animal2.Speak();
        
        Console.WriteLine("=== Inheritance Test Complete ===");
    }
}
