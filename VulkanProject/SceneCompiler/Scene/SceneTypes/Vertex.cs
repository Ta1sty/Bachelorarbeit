using System;
using System.Collections.Generic;
using System.Numerics;

namespace SceneCompiler.Scene.SceneTypes
{
    public struct Vertex
    {
        public static readonly int Size = 48;
        private float[] Array; // contains the float values
        public int MaterialIndex; // the material index in the materialbuffer

        // not passed to vksc
        public int IndexOffset; //the offset that was added to the index of this vertex
        public Vertex(IReadOnlyList<float> pos, IReadOnlyList<float> norm, IReadOnlyList<float> tex, int materialIndex, int indexOffset)
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
            IndexOffset = indexOffset;
        }

        public Vertex(int index, IReadOnlyList<float> pos, IReadOnlyList<float> norm, IReadOnlyList<float> tex, int materialIndex, int indexOffset)
        {
            Array = new float[8];
            Array[0] = pos[index * 3 + 0];
            Array[1] = pos[index * 3 + 1];
            Array[2] = pos[index * 3 + 2];
            Array[3] = norm[index * 3 + 0];
            Array[4] = norm[index * 3 + 1];
            Array[5] = norm[index * 3 + 2];
            Array[6] = tex[index * 2 + 0];
            Array[7] = tex[index * 2 + 1];
            MaterialIndex = materialIndex;
            IndexOffset = indexOffset;
        }

        public Vertex(int index, IReadOnlyList<float> floats, int materialIndex, int indexOffset)
        {
            MaterialIndex = materialIndex;
            Array = new float[8];
            Array[0] = floats[index * 8 + 0];
            Array[1] = floats[index * 8 + 1];
            Array[2] = floats[index * 8 + 2];
            Array[3] = floats[index * 8 + 3];
            Array[4] = floats[index * 8 + 4];
            Array[5] = floats[index * 8 + 5];
            Array[6] = floats[index * 8 + 6];
            Array[7] = floats[index * 8 + 7];
            IndexOffset = indexOffset;
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

        public Vector3 Normal()
        {
            return new Vector3(Array[3], Array[4], Array[5]);

        }

        public Vector2 Tex()
        {
            return new Vector2(Array[6], Array[7]);
        }
    }
}