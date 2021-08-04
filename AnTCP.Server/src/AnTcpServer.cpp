#include "AnTcpServer.hpp"

AnTcpError AnTcpServer::Run() noexcept
{
    WSADATA wsaData{};
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0)
    {
        DEBUG_ONLY(std::cout << ">> WSAStartup() failed: " << result << std::endl);
        return AnTcpError::Win32WsaStartupFailed;
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
        DEBUG_ONLY(std::cout << ">> getaddrinfo() failed: " << result << std::endl);
        WSACleanup();
        return AnTcpError::GetAddrInfoFailed;
    }

    ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);

    if (ListenSocket == INVALID_SOCKET)
    {
        DEBUG_ONLY(std::cout << ">> socket() failed: " << WSAGetLastError() << std::endl);
        freeaddrinfo(addrResult);
        WSACleanup();
        return AnTcpError::SocketCreationFailed;
    }

    result = bind(ListenSocket, addrResult->ai_addr, static_cast<int>(addrResult->ai_addrlen));
    freeaddrinfo(addrResult);

    if (result == SOCKET_ERROR)
    {
        DEBUG_ONLY(std::cout << ">> bind() failed: " << WSAGetLastError() << std::endl);
        closesocket(ListenSocket);
        ListenSocket = INVALID_SOCKET;
        WSACleanup();
        return AnTcpError::SocketBindingFailed;
    }

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        DEBUG_ONLY(std::cout << ">> listen() failed: " << WSAGetLastError() << std::endl);
        closesocket(ListenSocket);
        ListenSocket = INVALID_SOCKET;
        WSACleanup();
        return AnTcpError::SocketListeningFailed;
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

        Clients.push_back(new ClientHandler(clientSocket, clientInfo, ShouldExit, &Callbacks, &OnClientConnected, &OnClientDisconnected));
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
    return AnTcpError::Success;
}

void ClientHandler::Listen() noexcept
{
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

    if (OnClientConnected)
    {
        (*OnClientConnected)(this);
    }

    while (!ShouldExit)
    {
        int incomingBytes = recv(Socket, recvbuf, ANTCP_BUFFER_LENGTH, 0);
        int bytesProcessed = 0;

        // if we received 0 or -1 bytes, we're going to disconnect the client
        if (incomingBytes <= 0)
        {
            break;
        }

        DEBUG_ONLY(std::cout << "[" << Id << "] " << "Received " << std::to_string(incomingBytes) + " bytes" << std::endl);

        do
        {
            if (packetBytesMissing > 0)
            {
                // gather bytes complete the packet
                const int bytesToCopy = std::min(incomingBytes - bytesProcessed, packetBytesMissing);

                DEBUG_ONLY(std::cout << "[" << Id << "] " << "Packet Chunk: " << std::to_string(bytesToCopy) << " bytes ("
                    << std::to_string(packetSize - (packetBytesMissing - bytesToCopy))
                    << "/" << std::to_string(packetSize) << ")" << std::endl);

                // copy data from incoming buffer to packet buffer
                memcpy(packet + (packetSize - packetBytesMissing), recvbuf + bytesProcessed, bytesToCopy);
                bytesProcessed += bytesToCopy;

                // check whether we still miss some bytes
                packetBytesMissing -= bytesToCopy;

                if (packetBytesMissing == 0 && !ProcessPacket(packet, packetSize))
                {
                    Disconnect();
                    return;
                }
            }
            else
            {
                if (headerPosition < static_cast<int>(sizeof(AnTcpSizeType)))
                {
                    // we still need to finish building the header
                    const int bytesToCopy = std::min(incomingBytes - bytesProcessed, static_cast<int>(sizeof(AnTcpSizeType)) - headerPosition);

                    // copy data from buffer to header buffer
                    memcpy(header + headerPosition, recvbuf + bytesProcessed, bytesToCopy);
                    bytesProcessed += bytesToCopy;
                    headerPosition += bytesToCopy;
                }
                else
                {
                    // header is completed, begin to build packet
                    headerPosition = 0;
                    packetSize = *reinterpret_cast<AnTcpSizeType*>(header);

                    if (packetSize > ANTCP_MAX_PACKET_SIZE)
                    {
                        // packet is too big, this may be a wrong/malicious payload
                        DEBUG_ONLY(std::cout << "[" << Id << "] " << "Packet too big (" << std::to_string(packetSize)
                            << "/" << ANTCP_MAX_PACKET_SIZE << "), disconnecting client..." << std::endl);

                        Disconnect();
                        return;
                    }
                    else
                    {
                        DEBUG_ONLY(std::cout << "[" << Id << "] " << "New Packet: " << std::to_string(packetSize) + " bytes" << std::endl);

                        if (incomingBytes >= packetSize)
                        {
                            // we already received the full packet, so process it directly
                            if (!ProcessPacket(recvbuf + bytesProcessed, packetSize))
                            {
                                Disconnect();
                                return;
                            }

                            bytesProcessed += packetSize;
                        }
                        else
                        {
                            // some bytes are missing
                            packetBytesMissing = packetSize;
                        }
                    }
                }
            }
        } while (bytesProcessed < incomingBytes);
    }

    Disconnect();
}