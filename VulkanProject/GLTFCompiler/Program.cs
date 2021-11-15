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
            using var src = File.Open(@"C:\Users\marku\Desktop\BA\Models\SimpleHouse.gltf", FileMode.Open);
            var path = @"C:\Users\marku\Desktop\BA\VulkanProject\VulkanProject\VulkanProject\dump.bin";
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
            s.WriteBytes(path);
            int a = 1;
            a = a + 1;
        }
    }
}
