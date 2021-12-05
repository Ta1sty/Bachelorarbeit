using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading;
using GLTFCompiler.GltfFileTypes;
using GLTFCompiler.Scene;
using System.Windows.Forms;
using Microsoft.VisualBasic;
using System.Reflection;
using System.Text.RegularExpressions;

namespace GLTFCompiler
{
    class Program
    {
        static void Main(string[] args)
        {
            foreach (var arg in args)
            {
                Console.WriteLine(arg);
            }
            var path = "";
            var t = new Thread(() =>
            {
                using var fileDialog = new OpenFileDialog();
                fileDialog.Filter = "GLTF (.gltf) | *.gltf";
                fileDialog.Title = "Select the file to open";
                Regex reg = new Regex(@"\\GLTFCompiler.*");
                var bPath = reg.Replace(Assembly.GetExecutingAssembly().Location, "");
                fileDialog.InitialDirectory = Directory.GetParent(bPath).FullName;

                var res = fileDialog.ShowDialog();
                if (res != DialogResult.OK || string.IsNullOrWhiteSpace(fileDialog.FileName))
                {
                    path = "";
                }
                else
                {
                    path = fileDialog.FileName;
                }
            });
            t.SetApartmentState(ApartmentState.STA);
            t.Start();
            t.Join();
            //using var src = File.Open(@"C:\Users\marku\Desktop\BA\Models\textureCube\2Cubes.gltf", FileMode.Open);
            var location = Assembly.GetExecutingAssembly().Location;
            var regex = new Regex(@"VulkanProject\\.*");
            var dst = regex.Replace(location, @"VulkanProject\VulkanProject\dump.bin");
            var p = Path.GetFullPath(dst);
            using var str = new StreamReader(path);
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
            s.WriteBytes(dst);
        }
    }
}
