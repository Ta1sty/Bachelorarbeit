using System;
using System.Collections.Generic;

namespace GLTFCompiler.Scene
{
    public class SceneNode
    {
        public int IndexBufferIndex = -1; // this index points to the first vertex-index in the index buffer
        public int NumTriangles = -1; // amount of triangles
        public float[,] Transform;
        public int NumChildren = 0;
        public int[] Children = Array.Empty<int>(); // [4,4] is the index of the next instance(when converted to int)
    }
}