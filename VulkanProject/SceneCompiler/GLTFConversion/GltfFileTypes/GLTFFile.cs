using System.Collections.Generic;
using Scene;

namespace SceneCompiler.GLTFConversion.GltfFileTypes
{
    public class GltfFile
    {
        public int Scene { get; set; } = -1;
        public List<Scene> Scenes {get; set; } = new();
        public List<Node> Nodes { get; set; } = new();
        public List<Mesh> Meshes { get; set; } = new();
        public List<Accessor> Accessors { get; set; } = new();
        public List<BufferView> BufferViews { get; set; } = new();
        public List<Buffer> Buffers { get; set; } = new();
        public List<GltfMaterial> Materials { get; set; } = new();
        public List<Texture> Textures { get; set; } = new();
        public List<ImagePointer> Images { get; set; } = new();
    }
}