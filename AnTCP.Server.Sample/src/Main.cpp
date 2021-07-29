#include "Main.hpp"

int main()
{
    SetConsoleTitle(L"AnTCP.Server Sample");

    if (!SetConsoleCtrlHandler(SigIntHandler, 1))
    {
        std::cout << ">> SetConsoleCtrlHandler() failed: " << GetLastError() << std::endl;
        return 1;
    }

    const std::string ip = "127.0.0.1";
    const unsigned short port = 47110;

    Server = new AnTcpServer(ip, port);

    Server->AddCallback((char)MessageType::ADD, AddCallback);
    Server->AddCallback((char)MessageType::SUBTRACT, SubtractCallback);
    Server->AddCallback((char)MessageType::MULTIPLY, MultiplyCallback);
    Server->AddCallback((char)MessageType::MIN_AVG_MAX, MinAvgMaxCallback);

    std::cout << ">> Starting server on: " << ip << ":" << std::to_string(port) << std::endl;
    Server->Run();

    std::cout << ">> Stopped server..." << std::endl;
}

int __stdcall SigIntHandler(unsigned long signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
    {
        std::cout << ">> Received CTRL-C, stopping server..." << std::endl;
        Server->Stop();
    }

    return 1;
}

void AddCallback(ClientHandler* handler, char type, const void* data, int size)
{
    int c = ((int*)data)[0] + ((int*)data)[1];
    handler->SendData(type, c);
}

void SubtractCallback(ClientHandler* handler, char type, const void* data, int size)
{
    int c = ((int*)data)[0] - ((int*)data)[1];
    handler->SendData(type, c);
}

void MultiplyCallback(ClientHandler* handler, char type, const void* data, int size)
{
    int c = ((int*)data)[0] * ((int*)data)[1];
    handler->SendData(type, c);
}

void MinAvgMaxCallback(ClientHandler* handler, char type, const void* data, int size)
{
    int a = ((int*)data)[0];
    int b = ((int*)data)[1];

    int c[3];
    c[0] = std::min(a,b);
    c[2] = std::max(a,b);
    c[1] = (c[2] + c[0]) / 2;

    handler->SendData(type, c, sizeof(c));
}
