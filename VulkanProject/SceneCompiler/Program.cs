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

            if (false)
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


            RayTracePostCompile rayTraceOptimization = new RayTracePostCompile(compiler.Buffers, true);
            rayTraceOptimization.PostCompile();
            rayTraceOptimization.RebuildNodeBufferFromRoot();
            rayTraceOptimization.Validate();
            rayTraceOptimization.PrintScene();
            var writer = new SceneWriter();
            writer.WriteBuffers(dst, compiler);
            Console.WriteLine("PARSE FINISHED, PRESS ENTER");
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
