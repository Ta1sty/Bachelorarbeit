using System;
using System.Collections.Generic;
using System.Numerics;

namespace SceneCompiler.Scene.SceneTypes
{
    public struct Vertex
    {
        public static readonly int Size = 48;
        private float[] Array;
        public int MaterialIndex;
        public Vertex(float[] pos, float[] norm, float[] tex, int materialIndex)
        {
            Array = new float[8];
            Array[0] = pos[0];
            Array[1] = pos[1];
            Array[2] = pos[2];
            Array[3] = norm[0];
            Array[4] = norm[1];
            Array[5] = norm[2];
            Array[6] = tex[0];
            Array[7] = tex[1];
            MaterialIndex = materialIndex;
        }

        public Vertex(int triangle, List<float> pos, List<float> norm, List<float> tex, int materialIndex)
        {
            Array = new float[8];
            Array[0] = pos[triangle * 3 + 0];
            Array[1] = pos[triangle * 3 + 1];
            Array[2] = pos[triangle * 3 + 2];
            Array[3] = norm[triangle * 3 + 0];
            Array[4] = norm[triangle * 3 + 1];
            Array[5] = norm[triangle * 3 + 2];
            Array[6] = tex[triangle * 2 + 0];
            Array[7] = tex[triangle * 2 + 1];
            MaterialIndex = materialIndex;
        }

        public int WriteToByteArray(byte[] vertices, int pos)
        {
            BitConverter.GetBytes(Array[0]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Array[1]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Array[2]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad1

            BitConverter.GetBytes(Array[3]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Array[4]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Array[5]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad2


            BitConverter.GetBytes(Array[6]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(Array[7]).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(MaterialIndex).CopyTo(vertices.AsSpan(pos += 4));
            BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad3
            return pos;
        }

        public Vector3 Position()
        {
            return new Vector3(Array[0], Array[1], Array[2]);
        }
    }
}