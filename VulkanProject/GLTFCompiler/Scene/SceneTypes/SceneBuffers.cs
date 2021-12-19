using System.Collections.Generic;
using GLTFCompiler.GltfFileTypes;

namespace GLTFCompiler.Scene
{
    public class SceneBuffers
    {
        public List<Vertex> VertexBuffer { get; set; } = new();
        public List<uint> IndexBuffer { get; set; } = new();
        public List<Material> MaterialBuffer { get; set; } = new();
        public List<SceneNode> Nodes { get; set; } = new(); // the last node is the root node, this is due to the format of gltf
    }
}