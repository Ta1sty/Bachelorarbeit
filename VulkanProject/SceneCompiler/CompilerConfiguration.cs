using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using Accessibility;

namespace SceneCompiler
{
    public class CompilerConfiguration
    {
        public static readonly CompilerConfiguration Configuration = LoadCompilerConfiguration();

        public GltfConfiguration GltfConfiguration { get; set; } = new();
        public MoanaConfiguration MoanaConfiguration { get; set; } = new();
        public InstanceConfiguration InstanceConfiguration { get; set; } = new();
        public LodConfiguration LodConfiguration { get; set; } = new();
        public OptimizationConfiguration OptimizationConfiguration { get; set; } = new();
        public DebugConfiguration DebugConfiguration { get; set; } = new();
        /// <summary>if this is not null, the .vksc file will be saved in that folder</summary>
        public string StorePath { get; set; }

        private static CompilerConfiguration LoadCompilerConfiguration()
        {
            var opt = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true,
                WriteIndented = true,
                IgnoreNullValues = false,
            };
            CompilerConfiguration config;
            var path = "appsettings.json";

            var overridden = OverrideAppsettings(out var overridePath);
            if (overridden)
                path = overridePath;
            using (var str = new StreamReader(path))
            {
                var res = str.ReadToEnd();
                config = JsonSerializer.Deserialize<CompilerConfiguration>(res, opt);
            }

            if (overridden)
                return config;
                // write back so that newly added default values are added to the json file
            using (var str = new StreamWriter("appsettings.json"))
            {
                var text = JsonSerializer.Serialize(config, opt);
                str.Write(text);
            }
#if DEBUG
            using (var str = new StreamWriter("../../../appsettings.json"))
            {
                var text = JsonSerializer.Serialize(config, opt);
                str.Write(text);
            }
#endif
            return config;
        }
        private static bool OverrideAppsettings(out string retPath)
        {
            var path = "";
            var result = false;
            var t = new Thread(() =>
            {
                var confResult = MessageBox.Show("Override appsettings with file?", "Appsettings", MessageBoxButtons.YesNo);
                if (confResult == DialogResult.No)
                    return;
                using var fileDialog = new OpenFileDialog();
                fileDialog.Filter = "JSON-File (.json) | *.json";
                fileDialog.Title = "Select the overriding appsettings file";
                Regex reg = new Regex(@"\\SceneCompiler.*");
                var bPath = reg.Replace(Assembly.GetExecutingAssembly().Location, "");
                fileDialog.InitialDirectory = Directory.GetParent(bPath).FullName;

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
                result = true;
            });
            t.SetApartmentState(ApartmentState.STA);
            t.Start();
            t.Join();
            retPath = path;
            return result;
        }
    }

    public class GltfConfiguration
    {
        /// <summary>Converts a GLTF file to a scenegraph - exclusive with ConvertMoana</summary>
        public bool ConvertGltf { get; set; } = false;
        /// <summary>removes duplicates from the GLTF file</summary>
        public bool RemoveDuplicates { get; set; } = true;
        /// <summary>Removes instances when from the root node if it thinks there are any</summary>
        public bool RemovePresumedInstances { get; set; } = true;
    }

    public class MoanaConfiguration
    {
        /// <summary>Converts the .pbrt-Moana Scene to a scenegraph - exclusive with ConvertGltf</summary>
        public bool ConvertMoana { get; set; } = true;
        /// <summary>Uses Compiler V2</summary>
        public bool UseCompilerVersion2 { get; set; } = true;
        /// <summary>if this is not null, the compiler loads the moana scene from this directory. Should end with .../island/pbrt</summary>
        public string MoanaRootPath { get; set; } = null;
        /// <summary>cleans up the parsed moana file once the scenenodes are made. Preserves memory during compilation</summary>
        public bool Cleanup { get; set; } = true;
        /// <summary>Merges the meshes of a geometry file into one large mesh</summary>
        public bool MergeMeshes { get; set; } = true;
        /// <summary>takes the first object of the first included section as the root of the scenegraph</summary>
        public bool UseFirstSectionObject { get; set; } = false;
        /// <summary>Includes the instanced geometry when using only the first geometry</summary>
        public bool UseObjectIncludes { get; set; } = true;
        /// <summary>the sections to include</summary>
        public List<string> Includes { get; set; } = new(){"isBayCedarA1"};
        /// <summary>Validates the materials for the mona scene to check which are present or missing</summary>
        public bool ValidateMaterials { get; set; } = true;
        /// <summary>Validates moana island that every referenced object can be resolved</summary>
        public bool ValidateMoana { get; set; } = true;
    }

    public class InstanceConfiguration
    {
        /// <summary>instances the loaded scene multiple times</summary>
        public bool UseInstancing { get; set; } = false;
        /// <summary>instances the scene n times along the x-Axis</summary>
        public int InstanceX { get; set; } = 16;
        /// <summary>instances the scene n times along the z-Axis</summary>
        public int InstanceZ { get; set; } = 16;
        /// <summary>instances the scene multiple times - for example: root x (16x16) x (16x16) => InstanceCount is 2</summary>
        public int InstanceCount { get; set; } = 1;
        /// <summary>inserts space inbetween the nodes based on the size</summary>
        public float SpacingFactor { get; set; } = 1;
        /// <summary>instances the scene randomly, with a small offset and rotation</summary>
        public bool Randomize { get; set; } = false;
    }

    public class OptimizationConfiguration
    {
        /// <summary>
        /// Uses multi-level instancing. If this is set to true, all nodes that that do not contain geometry are
        /// collapsed into a large array and an instance list is put over them
        /// </summary>
        public bool UseSingleLevelInstancing { get; set; } = true;
        /// <summary>Reduces instancing in the same way as single level but stops at nodes that reference a node with geometry</summary>
        public int ReduceInstanceListCount { get; set; } = 2;
        /// <summary>Forces all instance lists to even levels to prevent TraversalStack overload</summary>
        public bool ForceInstanceListEven { get; set; } = true;
        /// <summary>merges instance lists if nodes reference many, reduces AABB overlap</summary>
        public bool MergeInstanceLists { get; set; } = true;
        /// <summary>caps instance lists if they exceed MaxInstances and splits them spatially</summary>
        public bool CapInstanceLists { get; set; } = true;
        /// <summary>the maximum amount of instances that can be contained in an InstanceList</summary>
        public int MaxInstances { get; set; } = 1 << 23;
        /// <summary>rebuilds the node buffer from the root once compilation has finished</summary>
        public bool RootBufferRebuild { get; set; } = true;
    }

    public class LodConfiguration
    {
        /// <summary>Uses lod for large triangle meshes</summary>
        public bool UseLod { get; set; } = false;
        /// <summary>Creates levels of detail based on a number of triangles</summary>
        public bool UseTriangleLod { get; set; } = true;
        /// <summary>The amount of triangles needed for lod to be used</summary>
        public int LodTriangleThreshold { get; set; } = 100000;
        /// <summary>stops creating LOD once the triangle count is below this value</summary>
        public int LodTriangleStopCount { get; set; } = 100000;
        /// <summary>Creates Levels of detail for large Instance lists</summary>
        public bool UseInstanceLod { get; set; } = false;
        /// <summary>Removes the excess instead of splitting it</summary>
        public bool CutExcess { get; set; } = false;
        /// <summary>The amount of instances need for a the creation of an instance list LOD</summary>
        public int LodInstanceThreshold { get; set; } = 500;
        /// <summary>stops creating LOD once the instance count is below this value</summary>
        public int LodInstanceStopCount { get; set; } = 500;
        /// <summary>The reduction factor per level of lod. Means factor 4 is ~25% of triangles</summary>
        public int ReductionFactor { get; set; } = 4;
        /// <summary>The number of Lods to build</summary>
        public int NumLod { get; set; } = 6;
        /// <summary>The additional Arguments passed to the TriDecimator</summary>
        public List<string> TriDecimatorArguments { get; set; } = new List<string>() { "-Ty", "-C" };
    }

    public class DebugConfiguration
    {
        /// <summary>prints the scene in the console</summary>
        public bool PrintScene { get; set; } = true;
        /// <summary>prints the full scene always</summary>
        public bool PrintFullScene { get; set; } = false;
        /// <summary>prints the full scene if the number of nodes is less than the threshold</summary>
        public int PrintFullThreshold { get; set; } = 1000;
        /// <summary>prints the material names and the number of vertices that use them</summary>
        public bool PrintMaterials { get; set; } = true;
        /// <summary>Validates the scenegraph at the end of the compilation</summary>
        public bool ValidateSceneGraph { get; set; } = true;
        /// <summary>validate parents and children, checks that the bidirectional relation is correct, can impact performance heavily</summary>
        public bool ValidateParentsAndChildren { get; set; } = false;
        /// <summary></summary>
        public bool InsertGround { get; set; } = false;
    }
}