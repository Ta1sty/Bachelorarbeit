using System.Collections.Generic;

namespace SceneCompiler.GLTFConversion.GltfFileTypes
{
    public class Mesh
    {
        public string Name { get; set; }
        public List<Primitive> Primitives { get; set; }
    }
}