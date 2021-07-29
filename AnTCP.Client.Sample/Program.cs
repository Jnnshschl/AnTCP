using AnTCP.Client.Objects;
using System;
using System.Text.Json;

namespace AnTCP.Client.Sample
{
    public enum MessageType
    {
        Add,
        Subtract,
        Multiply,
        MinAvgMax,
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
                    AnTcpResponse response = client.Send((byte)new Random().Next(0, 4), data);

                    Console.WriteLine($">> Response: {response.Length} bytes | Type: {(MessageType)response.Type}");

                    switch ((MessageType)response.Type)
                    {
                        case MessageType.Add:
                        case MessageType.Subtract:
                        case MessageType.Multiply:
                            Console.WriteLine($">> Data: {response.As<int>()}");
                            break;


                        case MessageType.MinAvgMax:
                            int[] array = response.AsArray<int>();
                            Console.WriteLine($">> Data Array[{array.Length}]: {JsonSerializer.Serialize(array)}");
                            break;

                        default:
                            break;
                }

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