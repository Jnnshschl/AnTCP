#include "AnTcpServer.hpp"

int AnTcpServer::Run() noexcept
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
        ListenSocket = INVALID_SOCKET;
        WSACleanup();
        return 1;
    }

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << ">> listen() failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        ListenSocket = INVALID_SOCKET;
        WSACleanup();
        return 1;
    }

    while (!ShouldExit)
    {
        // accept client and get socket info from it, the socket info contains the ip address
        // and port used to connect o the server
        SOCKADDR_IN clientInfo{ 0 };
        int size = sizeof(SOCKADDR_IN);
        SOCKET clientSocket = accept(ListenSocket, reinterpret_cast<sockaddr*>(&clientInfo), &size);

        if (clientSocket == INVALID_SOCKET)
        {
            DEBUG_ONLY(std::cout << ">> accept() failed: " << WSAGetLastError() << std::endl);
            continue;
        }

        // cleanup old disconnected clients
        ClientCleanup();

        Clients.push_back(new ClientHandler(Clients.size(), clientSocket, clientInfo, ShouldExit, &Callbacks));
    }

    for (ClientHandler* clientHandler : Clients)
    {
        if (clientHandler)
        {
            delete clientHandler;
        }
    }

    Clients.clear();

    WSACleanup();
    return 0;
}

void ClientHandler::Listen() noexcept
{
    // convert the ip to a human readable format
    char ipAddressBuffer[16]{ 0 };
    inet_ntop(AF_INET, &SocketInfo.sin_addr, ipAddressBuffer, 16);

    // this string will be used in logging to identify clients
    std::string logTag("[" + std::to_string(Id) + "|" + ipAddressBuffer + ":" + std::to_string(SocketInfo.sin_port) + "] >> ");
    std::cout << logTag << "Connected" << std::endl;

    // the amount of bytes received from the recv function
    int incomingBytes = 0;
    // the amount of bytes that we processed
    int bytesProcessed = 0;
    // buffer for the incoming bytes
    char recvbuf[ANTCP_BUFFER_LENGTH];

    // the index of our current header
    int headerPosition = 0;
    // buffer for the header
    char header[sizeof(AnTcpSizeType)]{ 0 };

    // the total packet size
    AnTcpSizeType packetSize = 0;
    // the amount of bytes we are missing to complete the packet
    int packetBytesMissing = 0;
    // buffer for the packet
    char packet[ANTCP_MAX_PACKET_SIZE]{ 0 };

    while (!ShouldExit)
    {
        incomingBytes = recv(Socket, recvbuf, ANTCP_BUFFER_LENGTH, 0);

        // if we received 0 or -1 bytes, we're going to disconnect the client
        if (incomingBytes <= 0)
        {
            break;
        }

        DEBUG_ONLY(std::cout << logTag << "Received " << std::to_string(incomingBytes) + " bytes" << std::endl);

        do
        {
            if (packetBytesMissing > 0)
            {
                const int bytesToCopy = std::min(incomingBytes - bytesProcessed, packetBytesMissing);

                DEBUG_ONLY(std::cout << logTag
                    << "Packet Chunk: " << std::to_string(bytesToCopy) << " bytes ("
                    << std::to_string(packetSize - (packetBytesMissing - bytesToCopy))
                    << "/" << std::to_string(packetSize) << ")" << std::endl);

                // copy data from incoming buffer to packet buffer
                const unsigned char* bufferData = reinterpret_cast<const unsigned char*>(recvbuf + bytesProcessed);
                unsigned char* packetData = reinterpret_cast<unsigned char*>(packet + (packetSize - packetBytesMissing));

                __movsb(packetData, bufferData, bytesToCopy);
                bytesProcessed += bytesToCopy;

                // check whether we still miss some bytes
                packetBytesMissing -= bytesToCopy;

                if (packetBytesMissing == 0)
                {
                    // measure packet processing time in debug mode
                    BENCHMARK(const auto packetStart = std::chrono::high_resolution_clock::now());

                    if (!ProcessPacket(packet, packetSize))
                    {
                        std::cout << logTag << "\"" << static_cast<int>(*reinterpret_cast<AnTcpMessageType*>(packet)) << "\" is an unknown message type, disconnecting client..." << std::endl;
                        closesocket(Socket);
                        Socket = INVALID_SOCKET;
                        IsConnected = false;
                        return;
                    }

                    BENCHMARK(std::cout << logTag << "Processing packet of type \"" << static_cast<int>(*reinterpret_cast<AnTcpMessageType*>(packet)) << "\" took: "
                        << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - packetStart) << std::endl);
                }
            }
            else
            {
                if (headerPosition < static_cast<int>(sizeof(AnTcpSizeType)))
                {
                    const int bytesToCopy = std::min(incomingBytes - bytesProcessed, static_cast<int>(sizeof(AnTcpSizeType)) - headerPosition);

                    // copy data from buffer to header buffer
                    const unsigned char* bufferData = reinterpret_cast<const unsigned char*>(recvbuf + bytesProcessed);
                    unsigned char* headerData = reinterpret_cast<unsigned char*>(header + headerPosition);

                    __movsb(headerData, bufferData, bytesToCopy);
                    bytesProcessed += bytesToCopy;

                    headerPosition += bytesToCopy;
                }
                else
                {
                    headerPosition = 0;
                    packetSize = *reinterpret_cast<AnTcpSizeType*>(header);

                    if (packetSize > ANTCP_MAX_PACKET_SIZE)
                    {
                        // packet is too big, this may be a wrong/malicious payload
                        std::cout << logTag << "Packet too big (" << packetSize << "/" << ANTCP_MAX_PACKET_SIZE << "), disconnecting client..." << std::endl;
                        closesocket(Socket);
                        Socket = INVALID_SOCKET;
                        IsConnected = false;
                        return;
                    }

                    packetBytesMissing = packetSize;
                    DEBUG_ONLY(std::cout << logTag << "New Packet: " << std::to_string(packetSize) + " bytes" << std::endl);
                }
            }
        } while (bytesProcessed < incomingBytes);

        bytesProcessed = 0;
    }

    closesocket(Socket);
    Socket = INVALID_SOCKET;
    std::cout << logTag << "Disconnected" << std::endl;
    IsConnected = false;
}