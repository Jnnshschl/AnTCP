#pragma once

constexpr auto ANTCP_SERVER_VERSION = "1.0.0.0";
constexpr auto ANTCP_BUFFER_LENGTH = 512;
constexpr auto ANTCP_MAX_PACKET_SIZE = 256;

#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <intrin.h>
#pragma intrinsic(__stosb)
#pragma intrinsic(__movsb)

class ClientHandler
{
private:
    bool IsActive;
    int Id;
    SOCKET Socket;
    SOCKADDR_IN SocketInfo;
    std::unordered_map <char, std::function<void(ClientHandler*, const void*, int)>>* Callbacks;

    std::thread* Thread;

public:
    ClientHandler(int id, SOCKET socket, const SOCKADDR_IN& socketInfo, std::unordered_map <char, std::function<void(ClientHandler*, const void*, int)>>* callbacks)
        : IsActive(true),
        Id(id),
        Socket(socket),
        SocketInfo(socketInfo),
        Callbacks(callbacks),
        Thread(new std::thread(&ClientHandler::HandleClient, this, Id, Socket, SocketInfo))
    {}

    ~ClientHandler()
    {
        Thread->join();
        delete Thread;
    }

    constexpr bool IsRunning() { return IsActive; }

    /// <summary>
    /// Send data to the client.
    /// </summary>
    /// <param name="type">Message type.</param>
    /// <param name="data">Data to send.</param>
    /// <param name="size">Size of the data to send.</param>
    /// <returns>True if data was sent, false if not.</returns>
    bool SendData(char type, const void* data, int size);

private:
    void HandleClient(int id, SOCKET socket, const SOCKADDR_IN& socketInfo);
    bool ProcessPacket(const char* data, int size);
};

class AnTcpServer
{
private:
    std::string Ip;
    std::string Port;
    SOCKET ListenSocket;
    std::vector<ClientHandler*> Clients;
    std::unordered_map <char, std::function<void(ClientHandler*, const void*, int)>> Callbacks;

public:
    AnTcpServer(const std::string& ip, const std::string& port)
        : Ip(ip),
        Port(port),
        ListenSocket(INVALID_SOCKET),
        Clients(),
        Callbacks()
    {}

    ~AnTcpServer() {}

    /// <summary>
    /// Add a new callback for a message type, will be fired when the server received a message of that type.
    /// </summary>
    /// <param name="type">Message type.</param>
    /// <param name="callback">Function to handle the message.</param>
    /// <returns>True if callback was added, false if there is already a callback for this message type.</returns>
    bool AddCallback(char type, std::function<void(ClientHandler*, const void*, int)> callback);

    /// <summary>
    /// Remove a callback for the given message type.
    /// </summary>
    /// <param name="type">Message type.</param>
    /// <returns>True if callback was removed, false if there was no callback for this message type.</returns>
    bool RemoveCallback(char type);

    /// <summary>
    /// Starts the server, blocking.
    /// </summary>
    /// <returns>Status of the server, 1 if an error occured, 0 if not</returns>
    int Run();

private:
    void ClientCleanup();
};