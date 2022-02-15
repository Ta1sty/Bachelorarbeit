using System.IO;
using SceneCompiler.Scene.SceneTypes;

namespace SceneCompiler.Scene
{
    public abstract class ASceneCompiler
    {
        public SceneBuffers Buffers = new();
        public abstract void CompileScene(string path);
        public abstract void WriteTextures(FileStream stream);
        public ASceneCompiler()
        {
            SceneNode.buffers = Buffers;
        }
    }
}