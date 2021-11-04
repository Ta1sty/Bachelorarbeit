namespace GLTFCompiler.GltfFileTypes
{
    public class Accessor
    {
        public int BufferView { get; set; } = -1;
        public int ComponentType { get; set; } = -1;
        public int Count { get; set; } = -1;
        public string Type { get; set; }
    }
}