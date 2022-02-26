namespace Scene
{
    public class SceneMaterial
    {
        public bool DoubleSided { get; set; }
        public MaterialProperties PbrMetallicRoughness { get; set; }
        public string Name { get; set; }
    }

    public class MaterialProperties
    {
        public ColorTexture BaseColorTexture { get; set; }
        public float MetallicFactor { get; set; }
        public float RoughnessFactor { get; set; }
    }

    public class ColorTexture
    {
        public int Index { get; set; }
    }
}