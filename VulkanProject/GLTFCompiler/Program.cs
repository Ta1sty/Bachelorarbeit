using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using GLTFCompiler.GltfFileTypes;
using GLTFCompiler.Scene;

namespace GLTFCompiler
{
    class Program
    {
        static void Main(string[] args)
        {
            using var src = File.Open(@"C:\Users\marku\Desktop\BA\test.gltf", FileMode.Open);
            using var str = new StreamReader(src);
            var res = str.ReadToEnd();
            var opt = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            };
            var result = JsonSerializer.Deserialize<GltfFile>(res, opt);
            SceneData s = new SceneData(result);
            s.DecodeBuffers();
            s.ParseMeshes();
            s.BuildSceneGraph();
            s.WriteBytes();
            int a = 1;
            a = a + 1;
        }
    }
}
