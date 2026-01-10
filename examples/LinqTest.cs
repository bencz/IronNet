using System;
using System.Collections.Generic;
using System.Linq;

public class Product
{
    public int Id;
    public string Name;
    public int Price;
    public string Category;
    
    public Product(int id, string name, int price, string category)
    {
        Id = id;
        Name = name;
        Price = price;
        Category = category;
    }
}

class LinqTest
{
    static void Main()
    {
        Console.WriteLine("=== LINQ Test ===");
        
        // Create a list of products
        List<Product> products = new List<Product>();
        products.Add(new Product(1, "Laptop", 1200, "Electronics"));
        products.Add(new Product(2, "Phone", 800, "Electronics"));
        products.Add(new Product(3, "Tablet", 500, "Electronics"));
        products.Add(new Product(4, "Chair", 150, "Furniture"));
        products.Add(new Product(5, "Desk", 300, "Furniture"));
        products.Add(new Product(6, "Lamp", 50, "Furniture"));
        products.Add(new Product(7, "Book", 25, "Books"));
        products.Add(new Product(8, "Notebook", 15, "Books"));
        
        Console.Write("Total products: ");
        Console.WriteLine(products.Count);
        
        // Test 1: Sum of all prices
        Console.WriteLine("Test 1: Sum of all prices");
        int totalPrice = products.Sum(p => p.Price);
        Console.Write("Total: $");
        Console.WriteLine(totalPrice);
        
        // Test 2: Where - filter expensive products (price > 200)
        Console.WriteLine("Test 2: Where - expensive products (price > 200)");
        IEnumerable<Product> expensive = products.Where(p => p.Price > 200);
        foreach (Product p in expensive)
        {
            Console.Write("  ");
            Console.Write(p.Name);
            Console.Write(": $");
            Console.WriteLine(p.Price);
        }
        
        // Test 3: Select - project to names
        Console.WriteLine("Test 3: Select - product names");
        IEnumerable<string> names = products.Select(p => p.Name);
        foreach (string name in names)
        {
            Console.Write("  ");
            Console.WriteLine(name);
        }
        
        // Test 4: Count with predicate
        Console.WriteLine("Test 4: Count products");
        int count = products.Count();
        Console.Write("Count: ");
        Console.WriteLine(count);
        
        // Test 5: First
        Console.WriteLine("Test 5: First product");
        Product first = products.First();
        Console.Write("First: ");
        Console.WriteLine(first.Name);
        
        // Test 6: Any with predicate
        Console.WriteLine("Test 6: Any expensive product (> 1000)?");
        bool hasExpensive = products.Any(p => p.Price > 1000);
        Console.Write("Has expensive: ");
        Console.WriteLine(hasExpensive);
        
        // Test 7: All with predicate
        Console.WriteLine("Test 7: All products have price > 0?");
        bool allPositive = products.All(p => p.Price > 0);
        Console.Write("All positive: ");
        Console.WriteLine(allPositive);
        
        // Test 8: Chained operations - Where + Sum
        Console.WriteLine("Test 8: Sum of electronics prices");
        int electronicsTotal = products
            .Where(p => p.Category == "Electronics")
            .Sum(p => p.Price);
        Console.Write("Electronics total: $");
        Console.WriteLine(electronicsTotal);
        
        // Test 9: Where + Select chain
        Console.WriteLine("Test 9: Names of cheap products (< 100)");
        IEnumerable<string> cheapNames = products
            .Where(p => p.Price < 100)
            .Select(p => p.Name);
        foreach (string name in cheapNames)
        {
            Console.Write("  ");
            Console.WriteLine(name);
        }
        
        // Test 10: ToList
        Console.WriteLine("Test 10: ToList");
        List<Product> expensiveList = products.Where(p => p.Price > 500).ToList();
        Console.Write("Expensive list count: ");
        Console.WriteLine(expensiveList.Count);
        
        Console.WriteLine("=== LINQ Test Complete ===");
    }
}
