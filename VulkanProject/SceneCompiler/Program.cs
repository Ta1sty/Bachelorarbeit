using System;
using System.IO;
using System.Threading;
using System.Windows.Forms;
using System.Reflection;
using System.Text.RegularExpressions;
using SceneCompiler.GLTFConversion.Compilation;
using SceneCompiler.MoanaConversion;
using SceneCompiler.Scene;
using System.Linq;
using System.Numerics;
using Scene;

namespace SceneCompiler
{
    public class Program
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

            var buffers = new SceneBuffers();

            var config = CompilerConfiguration.Configuration;

            if (config.MoanaConfiguration.ConvertMoana && config.GltfConfiguration.ConvertGltf)
                throw new Exception("can not convert gltf and moana, only one can be true");

            config.StorePath ??= ChooseDirectory("Choose Store directory for the scenes.");

            if (config.MoanaConfiguration.ConvertMoana)
            {
                config.MoanaConfiguration.MoanaRootPath ??= 
                    ChooseDirectory("Choose moana root path. Needs to end with island/pbrt");
                compiler = ConvertMoana(config.MoanaConfiguration.MoanaRootPath, buffers);
            }
            else if (config.GltfConfiguration.ConvertGltf)
            {
                path = ChooseFile();
                compiler = ConvertGltf(path, buffers);
            }
            else
            {
                throw new Exception("at least one convert type must be true");
            }

            var dst = Path.Combine(config.StorePath, compiler.SceneName + ".vksc");

            if (config.LodConfiguration.UseLod)
            {
                var creator = new LodCreator(buffers);
                if (config.LodConfiguration.UseTriangleLod)
                {
                    var lodNodes = buffers.Nodes.Where(x => x.NumTriangles > config.LodConfiguration.LodTriangleThreshold)
                        .ToList();
                    foreach (var node in lodNodes)
                    {
                        node.ForceOdd = false;
                        node.ForceEven = true;
                        creator.CreateTriangleLod(node, config.LodConfiguration.NumLod, config.LodConfiguration.ReductionFactor);
                    }
                }

                if (config.LodConfiguration.UseInstanceLod)
                {
                    var lodNodes = buffers.Nodes
                        .Where(x => x.IsInstanceList && x.NumChildren > config.LodConfiguration.LodInstanceThreshold)
                        .ToList();
                    foreach (var node in lodNodes)
                    {
                        creator.CreateInstanceLod(node, config.LodConfiguration.NumLod, config.LodConfiguration.ReductionFactor);
                    }
                }
            }

            if (config.InstanceConfiguration.UseInstancing)
            {
                var instancePostCompile = new InstancePostCompile(buffers);
                for (var i = 0; i < config.InstanceConfiguration.InstanceCount; i++)
                {
                    instancePostCompile.InstanceMultiple(config.InstanceConfiguration.InstanceX, config.InstanceConfiguration.InstanceZ);
                }
            }


            if(config.DebugConfiguration.PrintMaterials)
            {
                foreach (var mat in buffers.MaterialBuffer)
                {
                    var index = buffers.MaterialBuffer.IndexOf(mat);
                    var num = buffers.VertexBuffer.Count(x => x.MaterialIndex == index);
                    var first = buffers.VertexBuffer.FindIndex(x => x.MaterialIndex == index);
                    Console.WriteLine(mat.Name + " used for " + num + " vertices");
                }
            }

            var rayTraceOptimization = new RayTracePostCompile(buffers);
            if(config.OptimizationConfiguration.UseSingleLevelInstancing)
                rayTraceOptimization.ToSingleLevel();

            if (config.OptimizationConfiguration.ReduceInstanceListCount > 0)
                rayTraceOptimization.ReduceHeight();

            rayTraceOptimization.PostCompile();

            if(config.OptimizationConfiguration.RootBufferRebuild)
                buffers.RebuildNodeBufferFromRoot();
            if (config.DebugConfiguration.PrintScene)
                rayTraceOptimization.PrintScene();
            if (config.DebugConfiguration.ValidateSceneGraph)
                rayTraceOptimization.Validate();


            var writer = new SceneWriter();

            var idCount = buffers.Nodes.Count(x => 
                (x.ObjectToWorld == Matrix4x4.Identity || SceneNode.MatrixAlmostZero(x.ObjectToWorld - Matrix4x4.Identity)));
            Console.WriteLine("Scene Contained " + idCount + " identity transforms");
            Console.WriteLine("Writing Scene");
            writer.WriteBuffers(dst, compiler);
            Console.WriteLine("Total Number of Triangles: " + buffers.Root.TotalPrimitiveCount);
            var numTlas = buffers.Nodes.Count(x =>x.NeedsTlas);
            var numBlas = buffers.Nodes.Count(x => x.NeedsBlas);
            Console.WriteLine("Num TLAS: " + numTlas + " Num BLAS: " + numBlas);
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
                fileDialog.InitialDirectory = Directory.GetParent(Environment.CurrentDirectory).FullName;

                var res = fileDialog.ShowDialog();
                if (res != DialogResult.OK || string.IsNullOrWhiteSpace(fileDialog.FileName))
                {
                    Console.WriteLine("File choose was aborted or is invalid");
                    Console.WriteLine("Press ENTER to exit");
                    Console.ReadLine();
                    Environment.Exit(-1);
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

        private static string ChooseDirectory(string text)
        {
            var path = "";
            var t = new Thread(() =>
            {
                using var dirDialog = new FolderBrowserDialog();
                dirDialog.Description = text;
                dirDialog.UseDescriptionForTitle = true;
                dirDialog.SelectedPath = Directory.GetParent(Environment.CurrentDirectory).FullName;

                var res = dirDialog.ShowDialog();
                if (res != DialogResult.OK || string.IsNullOrWhiteSpace(dirDialog.SelectedPath))
                {
                    Console.WriteLine("Directory choose was aborted or is invalid");
                    Console.WriteLine("Press ENTER to exit");
                    Console.ReadLine();
                    Environment.Exit(-1);
                }
                else
                {
                    path = dirDialog.SelectedPath;
                }
            });
            t.SetApartmentState(ApartmentState.STA);
            t.Start();
            t.Join();
            return path;
        }

        private static ASceneCompiler ConvertMoana(string path, SceneBuffers buffers)
        {
            ASceneCompiler compiler;
            if (CompilerConfiguration.Configuration.MoanaConfiguration.UseCompilerVersion2)
                compiler = new MoanaCompilerV2(buffers);
            else
                compiler = new MoanaCompiler(buffers);
            compiler.CompileScene(path);
            return compiler;
        }

        private static ASceneCompiler ConvertGltf(string path, SceneBuffers buffers)
        {
            ASceneCompiler compiler = new GltfCompiler(buffers);
            compiler.CompileScene(path);
            return compiler;
        }
    }
}
