using System;

namespace GLTFCompiler.GltfFileTypes
{
    public class Node
    {
        public int Mesh { get; set; } = -1;
        public string Name { get; set; }
        public float[] Rotation { get; set; }
        public float[] Scale { get; set; }
        public float[] Translation { get; set; }
        public int[] Children { get; set; } = Array.Empty<int>();
    }
}