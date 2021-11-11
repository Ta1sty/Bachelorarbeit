namespace GLTFCompiler.GltfFileTypes
{
    public class Primitive
    {
        public Attribute Attributes { get; set; }
        public int Indices { get; set; } = -1;
        public int Material { get; set; } = -1;
    }
}