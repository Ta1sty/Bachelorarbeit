using System.Collections.Generic;

namespace SceneCompiler.Scene.SceneTypes
{
    public class SceneBuffers
    {
        public List<Vertex> VertexBuffer { get; set; } = new();
        public List<uint> IndexBuffer { get; set; } = new();
        public List<Material> MaterialBuffer { get; set; } = new();
        public List<SceneNode> Nodes { get; set; } = new();
        public int RootNode { get; set; } = -1;
    }
}