using System;
using System.Buffers;

namespace AnTCP.Client.Objects
{
    public readonly unsafe ref struct AnTcpResponse(Memory<byte> memory)
    {
        /// <summary>
        /// Raw data received by the TCP client.
        /// </summary>
        public Span<byte> Data { get; } = memory.Span[1..];

        /// <summary>
        /// Lenght of the data array.
        /// </summary>
        public int Length => Data.Length;

        /// <summary>
        /// Type of the response.
        /// </summary>
        public byte Type { get; } = memory.Span[0];

        /// <summary>
        /// Retrieve the data as any unmanaged type or struct.
        /// </summary>
        /// <typeparam name="T">Unmanaged type</typeparam>
        /// <returns>Data</returns>
        public T As<T>() where T : unmanaged => *Pointer<T>();

        /// <summary>
        /// Retrieve the data as any unmanaged type array.
        /// </summary>
        /// <typeparam name="T">Unmanaged type</typeparam>
        /// <returns>Data array</returns>
        public T[] AsArray<T>() where T : unmanaged
        {
            T[] array = new T[Length / sizeof(T)];
            using MemoryHandle h = array.AsMemory().Pin();
            Buffer.MemoryCopy(Pointer<T>(), h.Pointer, Length, Length);
            return array;
        }

        /// <summary>
        /// Retrieve the data as any unamanaged pointer.
        /// </summary>
        /// <typeparam name="T">Unmanaged type</typeparam>
        /// <returns>Pointer to the data</returns>
        public T* Pointer<T>() where T : unmanaged
        {
            fixed (byte* ptr = Data)
                return (T*)ptr;
        }
    }
}