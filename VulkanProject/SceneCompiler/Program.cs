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

            var location = Assembly.GetExecutingAssembly().Location;
            var buffers = new SceneBuffers();

            var config = CompilerConfiguration.Configuration;
            if (config.MoanaConfiguration.ConvertMoana && config.GltfConfiguration.ConvertGltf)
                throw new Exception("can not convert gltf and moana, only one can be true");
            if (config.MoanaConfiguration.ConvertMoana)
            {
                if(config.MoanaConfiguration.MoanaRootPath == null)
                    path = regex.Replace(location,
                    @"island\pbrt");
                else
                    path = config.MoanaConfiguration.MoanaRootPath;
                compiler = ConvertMoana(path, buffers);
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

            var dst = "";
            if (config.CustomStorePath == null)
            {
                dst = regex.Replace(location,
                    @"VulkanProject\Scenes\"
                    + compiler.SceneName + ".vksc");
            }
            else
            {
                dst = Path.Combine(config.CustomStorePath, compiler.SceneName + ".vksc");
            }


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
                        creator.CreateTriangleLod(node, config.LodConfiguration.NumMaxLod, config.LodConfiguration.ReductionFactor);
                    }
                }

                if (config.LodConfiguration.UseInstanceLod)
                {
                    var lodNodes = buffers.Nodes
                        .Where(x => x.IsInstanceList && x.NumChildren > config.LodConfiguration.LodInstanceThreshold)
                        .ToList();
                    foreach (var node in lodNodes)
                    {
                        creator.CreateInstanceLod(node, config.LodConfiguration.NumMaxLod, config.LodConfiguration.ReductionFactor);
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
            rayTraceOptimization.PostCompile();

            if(config.OptimizationConfiguration.RootBufferRebuild)
                buffers.RebuildNodeBufferFromRoot();
            if(config.DebugConfiguration.ValidateSceneGraph)
                rayTraceOptimization.Validate();
            if(config.DebugConfiguration.PrintScene)
                rayTraceOptimization.PrintScene();

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

        private static ASceneCompiler ConvertMoana(string path, SceneBuffers buffers)
        {
            ASceneCompiler compiler = new MoanaCompiler(buffers);
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
