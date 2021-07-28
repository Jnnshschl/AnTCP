#include "AnTcpServer.hpp"

int AnTcpServer::Run()
{
    WSADATA wsaData{};
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0)
    {
        std::cout << ">> WSAStartup() failed: " << result << std::endl;
        return 1;
    }

    addrinfo hints{ 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* addrResult{ nullptr };
    result = getaddrinfo(Ip.c_str(), Port.c_str(), &hints, &addrResult);

    if (result != 0)
    {
        std::cout << ">> getaddrinfo() failed: " << result << std::endl;
        WSACleanup();
        return 1;
    }

    ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);

    if (ListenSocket == INVALID_SOCKET)
    {
        std::cout << ">> socket() failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrResult);
        WSACleanup();
        return 1;
    }

    result = bind(ListenSocket, addrResult->ai_addr, static_cast<int>(addrResult->ai_addrlen));
    freeaddrinfo(addrResult);

    if (result == SOCKET_ERROR)
    {
        std::cout << ">> bind() failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << ">> listen() failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << ">> AnTCP.Server listening on: " << Ip << ":" << Port << std::endl;

    while (true)
    {
        // accept client and get socket info from it, the socket info contains the ip address
        // and port used to connect o the server
        SOCKADDR_IN clientInfo{ 0 };
        int size = sizeof(SOCKADDR_IN);
        SOCKET clientSocket = accept(ListenSocket, reinterpret_cast<sockaddr*>(&clientInfo), &size);

        if (clientSocket == INVALID_SOCKET)
        {
            std::cout << ">> accept() failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        // cleanup old disconnected clients
        ClientCleanup();

        Clients.push_back(new ClientHandler(Clients.size(), clientSocket, clientInfo, &Callbacks));
    }

    for (ClientHandler* handler : Clients)
    {
        if (handler)
        {
            delete handler;
        }
    }

    WSACleanup();
    return 0;
}

bool AnTcpServer::AddCallback(char type, std::function<void(ClientHandler*, const void*, int)> callback)
{
    if (!Callbacks.contains(type))
    {
        Callbacks[type] = callback;
        return true;
    }

    return false;
}

bool AnTcpServer::RemoveCallback(char type)
{
    if (Callbacks.contains(type))
    {
        Callbacks.erase(type);
        return true;
    }

    return false;
}

void AnTcpServer::ClientCleanup()
{
    for (size_t i = 0; i < Clients.size(); ++i)
    {
        if (Clients[i] && !Clients[i]->IsRunning())
        {
            delete Clients[i];
            Clients.erase(Clients.begin() + i);
        }
    }
}

bool ClientHandler::ProcessPacket(const char* data, int size)
{
    if ((*Callbacks).contains(data[0]))
    {
        (*Callbacks)[data[0]](this, data + 1, size - 1);
        return true;
    }

    return false;
}

bool ClientHandler::SendData(char type, const void* data, int size)
{
    const int headerSize = 5;

    char* packet = new char[headerSize + size]{ 0 };
    *(int*)packet = size + 1;
    packet[4] = type;

    if (size > 0)
    {
        __movsb((unsigned char*)(packet + headerSize), (unsigned char*)data, size);
    }

    int result = send(Socket, packet, headerSize + size, 0);

    delete[] packet;
    return result != SOCKET_ERROR;
}

void ClientHandler::HandleClient(int id, SOCKET socket, const SOCKADDR_IN& socketInfo)
{
    // convert the ip to a human readable format
    char ipBuff[16]{ 0 };
    inet_ntop(AF_INET, &socketInfo.sin_addr, ipBuff, 16);

    // this string will be used in logging to identify clients
    std::string threadTag("[" + std::to_string(id) + "|" + ipBuff + ":" + std::to_string(socketInfo.sin_port) + "] >> ");
    std::cout << threadTag << "Connected" << std::endl;

    int incomingBytes = 0;
    int bytesProcessed = 0;
    char recvbuf[ANTCP_BUFFER_LENGTH];

    int headerPosition = 0;
    char header[4]{ 0 };

    int packetSize = 0;
    int packetBytesMissing = 0;
    char packet[ANTCP_MAX_PACKET_SIZE]{ 0 };

    while (true)
    {
        incomingBytes = recv(socket, recvbuf, ANTCP_BUFFER_LENGTH, 0);

        if (incomingBytes <= 0) { break; }

#ifdef _DEBUG
        std::cout << threadTag << "Received " << std::to_string(incomingBytes) + " bytes" << std::endl;
#endif
        do
        {
            if (packetBytesMissing > 0)
            {
                int bytesLeft = incomingBytes - bytesProcessed;
                int bytesToCopy = std::min(bytesLeft, packetBytesMissing);
#ifdef _DEBUG
                std::cout << threadTag
                    << "Packet Chunk: " << std::to_string(bytesToCopy) << " bytes ("
                    << std::to_string(packetSize - (packetBytesMissing - bytesToCopy)) << "/" << std::to_string(packetSize) << ")" << std::endl;
#endif
                __movsb((unsigned char*)&packet[packetSize - packetBytesMissing], (unsigned char*)(recvbuf + bytesProcessed), bytesToCopy);
                incomingBytes -= bytesToCopy;
                packetBytesMissing -= bytesToCopy;

                if (packetBytesMissing == 0)
                {
                    if (!ProcessPacket(packet, packetSize))
                    {
                        std::cout << threadTag << "\"" << (int)packet[0] << "\" is an unknown message type, disconnecting client..." << std::endl;
                        closesocket(Socket);
                        IsActive = false;
                        return;
                    }

                    packetSize = 0;
                }
            }
            else
            {
                if (headerPosition < 4)
                {
                    int bytesToCopy = std::min(incomingBytes - bytesProcessed, 4 - headerPosition);
                    __movsb((unsigned char*)&header[headerPosition], (unsigned char*)(recvbuf + bytesProcessed), bytesToCopy);
                    bytesProcessed += bytesToCopy;
                    headerPosition += bytesToCopy;
                }
                else
                {
                    packetSize = *(int*)header;

                    if (packetSize > ANTCP_MAX_PACKET_SIZE)
                    {
                        // packet is too big, this may be a wrong/malicious payload
                        std::cout << threadTag << "Packet too big, disconnecting client..." << std::endl;
                        closesocket(Socket);
                        IsActive = false;
                        return;
                    }

                    packetBytesMissing = packetSize;
                    headerPosition = 0;
#ifdef _DEBUG
                    std::cout << threadTag << "New Packet: " << std::to_string(packetSize) + " bytes" << std::endl;
#endif
                }
            }
        } while (bytesProcessed < incomingBytes);

        bytesProcessed = 0;
    }

    closesocket(Socket);
    std::cout << threadTag << "Disconnected" << std::endl;
    IsActive = false;
}