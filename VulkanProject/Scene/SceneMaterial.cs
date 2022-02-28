using System.Drawing;
using System.Numerics;

namespace Scene
{
    public class SceneMaterial
    {
        public static readonly int Size = 48;
        public Vector4 DefaultColor { get; set; } = new(.2f, .4f, .8f, 1);
        public float Ambient { get; set; } = 0.2f;
        public float Diffuse { get; set; } = 0.5f;
        public float Specular { get; set; } = 0.3f;
        public float Transmission { get; set; } = 0;
        public float Reflection { get; set; } = 0;
        public float PhongExponent { get; set; } = 1;
        public string Name { get; set; }
        public int TextureIndex = -1;
    }
}