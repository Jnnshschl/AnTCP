#pragma once

#include "../../AnTCP.Server/src/AnTcpServer.hpp"

enum class MessageType
{
    ADD,
    SUBTRACT,
    MULTIPLY
};

void AddCallback(ClientHandler* handler, const void* data, int size);
void SubtractCallback(ClientHandler* handler, const void* data, int size);
void MultiplyCallback(ClientHandler* handler, const void* data, int size);