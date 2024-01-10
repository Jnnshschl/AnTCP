# AnTCP Client/Server

TCP Client (C#) and Server (C++) Library that I'll use in my projects, mainly the AmeisenBotX and its NavigationServer.

## Usage Client

Create a new AnTcpClient with your IP and Port. 🚀

```csharp
AnTcpClient client = new("127.0.0.1", 47110);
```

Call the `Connect` method. 🌐

```csharp
client.Connect();
```

Call the send method to send data (example sends two integers, but any unmanaged variable can be used). 0x0 is the message type that needs to have a handler on the server side, more on that later. 📤

```csharp
(int, int) data = (new Random().Next(1, 11), new Random().Next(1, 11));
                    
AnTcpResponse response = client.Send((byte)0x0, data);
```

Convert the response to any unmanaged data type. 🔄

```csharp
Console.WriteLine($">> Data: {response.As<int>()}");
```

Call the `Disconnect` method if you're done sending stuff. 🚪

```csharp
client.Disconnect();
```

## Usage Server

Create a new instance of the AnTcpServer with your IP and Port. 🛠️

```cpp
AnTcpServer server("127.0.0.1", "47110");
```

Create a callback function that will be called when the server received a message with type 0x0. 📬

```cpp
void AddCallback(ClientHandler* handler, const void* data, int size)
{
    // the function is going to add the two integers and returns its result
    int c = ((int*)data)[0] + ((int*)data)[1];
    handler->SendData((char)0x0, &c, 4);
}
```

Add the Callback to the server. ➕

```cpp
server.AddCallback((char)0x0, AddCallback);
```

Run the server. 🚀

```cpp
server.Run();
```
