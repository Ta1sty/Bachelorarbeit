using System.IO;
using Scene;

namespace SceneCompiler.Scene
{
    public abstract class ASceneCompiler
    {
        public SceneBuffers Buffers;
        public string SceneName { get; set; } = "";
        public abstract void CompileScene(string path);
        public abstract void WriteTextures(FileStream stream);

        protected ASceneCompiler(SceneBuffers buffers)
        {
            Buffers = buffers;
        }
    }
}