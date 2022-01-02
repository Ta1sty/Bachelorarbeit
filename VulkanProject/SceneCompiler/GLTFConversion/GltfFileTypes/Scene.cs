using System.Collections.Generic;

namespace SceneCompiler.GLTFConversion.GltfFileTypes
{
    public class Scene
    {
        public string Name { get; set; }
        public List<int> Nodes { get; set; }
    }
}