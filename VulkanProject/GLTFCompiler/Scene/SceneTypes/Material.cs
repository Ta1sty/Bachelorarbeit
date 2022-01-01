namespace SceneCompiler.Scene.SceneTypes
{
    public class Material
    {
        public bool DoubleSided { get; set; }
        public MaterialProperties PbrMetallicRoughness { get; set; }
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