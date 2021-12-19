using System.IO;

namespace GLTFCompiler.Scene
{
    public abstract class ASceneCompiler
    {
        public SceneBuffers Buffers = new();
        public abstract void CompileScene(string path);
        public abstract void WriteTextures(FileStream stream);
    }
}