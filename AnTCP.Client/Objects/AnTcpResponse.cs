using System;
using System.Buffers;

namespace AnTCP.Client.Objects
{
    public unsafe struct AnTcpResponse
    {
        public AnTcpResponse(byte type, byte[] data)
        {
            Type = type;
            Data = data;
        }

        /// <summary>
        /// Raw data received by the TCP client.
        /// </summary>
        public byte[] Data { get; }

        /// <summary>
        /// Lenght of the data array.
        /// </summary>
        public int Length => Data.Length;

        /// <summary>
        /// Type of the response.
        /// </summary>
        public byte Type { get; }

        /// <summary>
        /// Retrieve the data as any unmanaged type or struct.
        /// </summary>
        /// <typeparam name="T">Unmanaged type</typeparam>
        /// <returns>Data</returns>
        public T As<T>() where T : unmanaged => *Pointer<T>();

        /// <summary>
        /// Retrieve the data as any unamanaged pointer.
        /// </summary>
        /// <typeparam name="T">Unmanaged type</typeparam>
        /// <returns>Pointer to the data</returns>
        public T* Pointer<T>() where T : unmanaged
        {
            using MemoryHandle h = Data.AsMemory().Pin();
            return (T*)h.Pointer;
        }
    }
}