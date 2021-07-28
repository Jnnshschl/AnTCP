#include "Main.hpp"

int main()
{
    SetConsoleTitle(L"AnTCP.Server Sample");

    AnTcpServer server("127.0.0.1", "47110");
    server.AddCallback((char)MessageType::ADD, AddCallback);
    server.AddCallback((char)MessageType::SUBTRACT, SubtractCallback);
    server.AddCallback((char)MessageType::MULTIPLY, MultiplyCallback);
    server.Run();
}

void AddCallback(ClientHandler* handler, const void* data, int size)
{
    int c = ((int*)data)[0] + ((int*)data)[1];
    handler->SendData((char)MessageType::ADD, &c, 4);
}

void SubtractCallback(ClientHandler* handler, const void* data, int size)
{
    int c = ((int*)data)[0] - ((int*)data)[1];
    handler->SendData((char)MessageType::SUBTRACT, &c, 4);
}

void MultiplyCallback(ClientHandler* handler, const void* data, int size)
{
    int c = ((int*)data)[0] * ((int*)data)[1];
    handler->SendData((char)MessageType::MULTIPLY, &c, 4);
}