#pragma once

#include "../../AnTCP.Server/src/AnTcpServer.hpp"

enum class MessageType
{
    ADD,
    SUBTRACT,
    MULTIPLY,
    MIN_AVG_MAX
};

// global pointer used to stop server in the SigIntHandler function
inline AnTcpServer* Server = nullptr;

int __stdcall SigIntHandler(unsigned long signal);

void ConnectedCallback(ClientHandler* handler);
void DisconnectedCallback(ClientHandler* handler);

void AddCallback(ClientHandler* handler, char type, const void* data, int size);
void SubtractCallback(ClientHandler* handler, char type, const void* data, int size);
void MultiplyCallback(ClientHandler* handler, char type, const void* data, int size);
void MinAvgMaxCallback(ClientHandler* handler, char type, const void* data, int size);