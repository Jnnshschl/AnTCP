using AnTCP.Client.Objects;
using System;

namespace AnTCP.Client.Sample
{
    public enum MessageType
    {
        Add,
        Subtract,
        Multiply
    }

    internal unsafe class Program
    {
        private static void Main(string[] args)
        {
            Console.Title = "AnTCP.Client Sample";
            AnTcpClient client = new("127.0.0.1", 47110);

            while (true)
            {
                Console.ReadKey();

                if (!client.IsConnected)
                {
                    try
                    {
                        Console.WriteLine(">> Connecting...");
                        client.Connect();
                    }
                    catch
                    {
                        Console.WriteLine(">> Failed to connect...");
                        continue;
                    }
                }

                try
                {
                    (int, int) data = (new Random().Next(1, 11), new Random().Next(1, 11));

                    Console.WriteLine($"\n>> Sending: {data.Item1}, {data.Item2}");
                    AnTcpResponse response = client.Send((byte)new Random().Next(0, 3), data);

                    Console.WriteLine($">> Response: {response.Length} bytes | Type: {(MessageType)response.Type}");
                    Console.WriteLine($">> Data: {response.As<int>()}");
                }
                catch
                {
                    Console.WriteLine(">> Failed to send data...");
                }
            }

            client.Disconnect();
        }
    }
}