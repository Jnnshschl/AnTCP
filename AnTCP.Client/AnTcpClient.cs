using AnTCP.Client.Objects;
using System;
using System.IO;
using System.Net.Sockets;
using System.Runtime.CompilerServices;

namespace AnTCP.Client
{
    public unsafe class AnTcpClient(string ip, int port)
    {
        public string Ip { get; private set; } = ip;

        public bool IsConnected => Client != null && Client.Connected;

        public int Port { get; private set; } = port;

        private TcpClient Client { get; set; }

        private BinaryReader Reader { get; set; }

        private NetworkStream Stream { get; set; }

        /// <summary>
        /// Connect to the server.
        /// </summary>
        public void Connect()
        {
            Client = new(Ip, Port);
            Stream = Client.GetStream();
            Reader = new(Stream);
        }

        /// <summary>
        /// Disconnect from the server.
        /// </summary>
        public void Disconnect()
        {
            Stream.Close();
            Client.Close();
        }

        /// <summary>
        /// Send data to the server, data can be any unmanaged type.
        /// </summary>
        /// <typeparam name="T">Unamnaged type of the</typeparam>
        /// <param name="type">Mesagte type</param>
        /// <param name="data">Data to send</param>
        /// <returns>Server response</returns>
        public AnTcpResponse Send<T>(byte type, T data) where T : unmanaged
        {
            int size = sizeof(T);
            return SendData
            (
                BitConverter.GetBytes(size + 1).AsSpan(),
                new ReadOnlySpan<byte>(&type, 1),
                new ReadOnlySpan<byte>(&data, size)
            );
        }

        /// <summary>
        /// Send a byte array to the server.
        /// </summary>
        /// <param name="type">Mesagte type</param>
        /// <param name="data">Data to send</param>
        /// <returns>Server response</returns>
        public AnTcpResponse SendBytes(byte type, ReadOnlySpan<byte> data)
        {
            return SendData
            (
                BitConverter.GetBytes(data.Length + 1).AsSpan(), 
                new Span<byte>(&type, 1), 
                data
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private AnTcpResponse SendData(ReadOnlySpan<byte> size, ReadOnlySpan<byte> type, ReadOnlySpan<byte> data)
        {
            Stream.Write(size);
            Stream.Write(type);
            Stream.Write(data);
            return new AnTcpResponse(Reader.ReadBytes(Reader.ReadInt32()));
        }
    }
}