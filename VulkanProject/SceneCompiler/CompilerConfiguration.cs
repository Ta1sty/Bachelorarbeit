using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace SceneCompiler
{
    public class CompilerConfiguration
    {
        public static readonly CompilerConfiguration Configuration = LoadCompilerConfiguration();

        private static CompilerConfiguration LoadCompilerConfiguration()
        {
            var opt = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true,
                WriteIndented = true,
                IgnoreNullValues = false,
            };
            CompilerConfiguration config;
            using(var str = new StreamReader("appsettings.json"))
            {
                var res = str.ReadToEnd();
                config = JsonSerializer.Deserialize<CompilerConfiguration>(res, opt);
            }
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

        public GltfConfiguration GltfConfiguration { get; set; } = new();
        public MoanaConfiguration MoanaConfiguration { get; set; } = new();
        public InstanceConfiguration InstanceConfiguration { get; set; } = new();
        public OptimizationConfiguration OptimizationConfiguration { get; set; } = new();
        public LodConfiguration LodConfiguration { get; set; } = new();
        public DebugConfiguration DebugConfiguration { get; set; } = new();
        /// <summary>if this is not null, the .vksc file will be saved in that folder</summary>
        public string CustomStorePath { get; set; } = null;
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
        /// <summary>if this is not null, the compiler loads the moana scene from this directory. Should end with .../island/pbrt</summary>
        public string MoanaRootPath { get; set; } = null;
        /// <summary>cleans up the parsed moana file once the scenenodes are made. Preserves memory during compilation</summary>
        public bool Cleanup { get; set; } = true;
        /// <summary>Merges the meshes of a geometry file into one large mesh</summary>
        public bool MergeMeshes { get; set; } = true;
        /// <summary>takes the first geometry of the first included section as the root of the scenegraph</summary>
        public bool UseFirstGeometry { get; set; } = false;
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
    }

    public class OptimizationConfiguration
    {
        /// <summary>
        /// Uses multi-level instancing. If this is set to true, all nodes that that do not contain geometry are
        /// collapsed into a large array and an instance list is put over them
        /// </summary>
        public bool UseSingleLevelInstancing { get; set; } = true;
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
        /// <summary>uses lod for large triangle meshes</summary>
        public bool UseLod { get; set; } = false;
        /// <summary>the amount of triangles needed for lod to be used</summary>
        public int LodTriangleThreshold { get; set; } = 1000000;
        /// <summary>The reduction factor per level of lod. Means factor 4 is ~25% of triangles</summary>
        public int ReductionFactor { get; set; } = 4;
        /// <summary>The number of Lods to build</summary>
        public int NumLod { get; set; } = 5;
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
    }
}