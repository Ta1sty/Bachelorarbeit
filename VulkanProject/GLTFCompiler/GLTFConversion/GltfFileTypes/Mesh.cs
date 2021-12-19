using System.Collections.Generic;

namespace GLTFCompiler.GltfFileTypes
{
    public class Mesh
    {
        public string Name { get; set; }
        public List<Primitive> Primitives { get; set; }
    }
}