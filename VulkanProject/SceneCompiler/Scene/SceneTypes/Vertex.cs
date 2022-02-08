using System;

namespace SceneCompiler.Scene.SceneTypes
{
    public class Vertex
    {
        public static readonly int Size = 48;
        public float[] Position; // 3 12
        public float[] Normal; // 3 12
        public float[] TexCoords; // 2 8 = 32 bytes
        public int MaterialIndex { get; set; } = -1;

        public int WriteToByteArray(byte[] vertices, int pos)
        {
            BitConverter.GetBytes(Position[0]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Position[1]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Position[2]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad1

            BitConverter.GetBytes(Normal[0]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Normal[1]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Normal[2]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad2


            BitConverter.GetBytes(TexCoords[0]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(TexCoords[1]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(MaterialIndex).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad3
            return pos;
        }
    }
}