using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading;
using System.Windows.Forms;
using Microsoft.VisualBasic;
using System.Reflection;
using System.Text.RegularExpressions;
using SceneCompiler.GLTFConversion.Compilation;
using SceneCompiler.MoanaConversion;
using SceneCompiler.Scene;
using System.Linq;
using System.Numerics;
using SceneCompiler.Scene.SceneTypes;

namespace SceneCompiler
{
    class Program
    {
        static void Main(string[] args)
        {
            NativeMethods.AllocConsole();


            
            foreach (var arg in args)
            {
                Console.WriteLine(arg);
            }

            var regex = new Regex(@"VulkanProject\\.*");


            var path = "";
            ASceneCompiler compiler = null;

            var location = Assembly.GetExecutingAssembly().Location;

            if (true)
            {
                path = regex.Replace(location,
                    @"island\pbrt");
                compiler = ConvertMoana(path);
            } else
            {
                path = ChooseFile();
                compiler = ConvertGLTF(path);
            }

            var dst = regex.Replace(location,
                @"VulkanProject\Scenes\"
                + Path.GetFileNameWithoutExtension(path) + ".vksc");

            InstancePostCompile instancePostCompile = new InstancePostCompile(compiler.Buffers);
            //instancePostCompile.InstanceMultiple(16, 1);
            //instancePostCompile.InstanceMultiple(32, 32);

            foreach(var mat in compiler.Buffers.MaterialBuffer)
            {
                var index = compiler.Buffers.MaterialBuffer.IndexOf(mat);
                var num = compiler.Buffers.VertexBuffer.Count(x=>x.MaterialIndex == index);
                var first = compiler.Buffers.VertexBuffer.FindIndex(x => x.MaterialIndex == index);
                Console.WriteLine(mat.Name + " used for " + num + " vertices");
            }


            RayTracePostCompile rayTraceOptimization = new RayTracePostCompile(compiler.Buffers, true);
            rayTraceOptimization.PostCompile();
            compiler.Buffers.RebuildNodeBufferFromRoot();
            rayTraceOptimization.Validate();
            rayTraceOptimization.PrintScene();
            var writer = new SceneWriter();

            var idCount = compiler.Buffers.Nodes.Count(x => 
                (x.ObjectToWorld == Matrix4x4.Identity || SceneNode.MatrixAlmostZero(x.ObjectToWorld - Matrix4x4.Identity)));
            Console.WriteLine("Scene Contained " + idCount + " identity transforms");
            Console.WriteLine("Writing Scene");
            writer.WriteBuffers(dst, compiler);
            Console.WriteLine("Total Number of Triangles: " + compiler.Buffers.Root.TotalPrimitiveCount);
            Console.WriteLine("PARSE FINISHED, PRESS ENTER");
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
            Console.ReadLine();
        }

        private static string ChooseFile()
        {
            var path = "";
            var t = new Thread(() =>
            {
                using var fileDialog = new OpenFileDialog();
                fileDialog.Filter = "GLTF (.gltf) | *.gltf";
                fileDialog.Title = "Select the file to open";
                Regex reg = new Regex(@"\\SceneCompiler.*");
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
            return path;
        }

        private static ASceneCompiler ConvertMoana(string path)
        {
            ASceneCompiler compiler = new MoanaCompiler();
            compiler.CompileScene(path);
            return compiler;
        }

        private static ASceneCompiler ConvertGLTF(string path)
        {
            //using var src = File.Open(@"C:\Users\marku\Desktop\BA\Models\textureCube\2Cubes.gltf", FileMode.Open);
            ASceneCompiler compiler = new GLTFCompiler();
            compiler.CompileScene(path);
            return compiler;
        }
    }
}
