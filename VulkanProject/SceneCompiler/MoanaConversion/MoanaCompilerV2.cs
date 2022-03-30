using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using Scene;
using SceneCompiler.Scene;

namespace SceneCompiler.MoanaConversion
{
    public class MoanaCompilerV2 : ASceneCompiler
    {
        public static readonly MoanaConfiguration
            Configuration = CompilerConfiguration.Configuration.MoanaConfiguration;
        public string BasePath;

        private Dictionary<string, MoanaObject> IncludedObjects { get; set; } = new();
        private Dictionary<string, List<string>> Objects { get; set; } = new();
        private Dictionary<string, Material> Materials { get; set; } = new();

        public MoanaCompilerV2(SceneBuffers buffers) : base(buffers)
        {

        }

        public override void CompileScene(string path)
        {
            BasePath = Path.GetFullPath(path);

            BuildObjectAndMaterialDictionary();
            var world = new World
            {
                Name = "World",
                FilePath = Path.Combine(BasePath, "islandX.pbrt")
            };
            Buffers.Root = world.GetSceneNode(this);


            foreach (var section in Configuration.Includes)
            {
                SceneName += section.Replace("is", "") + ",";
            }
            Console.WriteLine("Setting Parents and indices");
            Buffers.RewriteAllParents();
            
            /*if (Configuration.UseFirstSectionObject)
            {
                var section = root.Children.First();
                var list = section.Children.First();
                var listElem = list.Children.First();
                var geometry = listElem.Children.First(x => x.NumTriangles > 0);
                if (Configuration.UseObjectIncludes)
                    Buffers.Root = listElem;
                else
                    Buffers.Root = geometry;
            }*/
            IncludedObjects.Clear();
            Objects.Clear();
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
        }

        private void BuildObjectAndMaterialDictionary()
        {
            foreach (var section in Directory.GetDirectories(BasePath))
            {
                var dirName = Path.GetFileName(section);
                // Objects
                {
                    var objectsFile = Path.Combine(section, "objects.pbrt");
                    if (File.Exists(objectsFile))
                    {
                        using var stream = new StreamReader(objectsFile);

                        var prefix = "";
                        while (!stream.EndOfStream)
                        {
                            var line = stream.ReadLine()!;

                            if (line.Contains("# archives used by"))
                            {
                                prefix = line.Substring(line.LastIndexOf(" ") + 1);
                            }

                            if (line.Contains("# main object for the element:"))
                            {
                                prefix = "";
                            }

                            if (!line.Contains("AttributeBegin")) continue;
                            line = stream.ReadLine();
                            if (!line.Contains(" ObjectBegin")) throw new Exception();
                            line = line[line.IndexOf("\"")..].Replace("\"", "");
                            var name = dirName + prefix + line;
                            var includes = new List<string>();
                            line = stream.ReadLine();
                            while (line.Contains(" Include"))
                            {
                                includes.Add(line[line.IndexOf("\"")..].Replace("\"", ""));
                                line = stream.ReadLine();
                            }

                            if (!line.Contains(" ObjectEnd")) throw new Exception();
                            line = stream.ReadLine();
                            if (!line.Contains("AttributeEnd")) throw new Exception();
                            Objects.Add(name, includes);
                        }
                        Console.WriteLine("Parsed " + objectsFile);
                    }
                }
                // Materials
                {
                    var materialFile = Path.Combine(section, "materials.pbrt");
                    if (File.Exists(materialFile))
                    {
                        using var stream = new StreamReader(materialFile);
                        Material material = null;
                        while (!stream.EndOfStream)
                        {
                            var line = stream.ReadLine()!;

                            if (line.StartsWith("MakeNamedMaterial"))
                            {
                                material = new Material();
                                var start = line.IndexOf("\"", StringComparison.Ordinal);
                                material.Name = line.Substring(start + 1, line.Length - start - 2);
                                if (!material.Name.Contains("_") || material.Name.Contains("archive_"))
                                    material.Name = dirName + "_" + material.Name;
                            }

                            if (line.StartsWith("#End MakeNamedMaterial"))
                            {
                                Materials.Add(material.Name, material);
                                material = null;
                            }

                            if (material != null)
                            {
                                if (!line.Contains("["))
                                    continue;
                                line = line.Replace("#", "");
                                var varDeclaration = line[..line.IndexOf("[", StringComparison.Ordinal)]
                                    .Replace("\"", "")
                                    .Trim();

                                var varType = varDeclaration.Split(" ")[0];
                                var varName = varDeclaration.Split(" ")[1];

                                var varValue = line[line.IndexOf("[", StringComparison.Ordinal)..]
                                    .Replace("[", "")
                                    .Replace("]", "")
                                    .Replace("\"", "");

                                object value = default;
                                if (varType.Contains("float"))
                                    value = float.Parse(varValue, CultureInfo.InvariantCulture);
                                if (varType.Contains("rgb"))
                                    value = varValue.Split(" ").Select(x => float.Parse(x, CultureInfo.InvariantCulture)).ToArray();
                                if (varType.Contains("bool"))
                                    value = bool.Parse(varValue);
                                if (varType.Contains("string"))
                                    value = varValue;

                                varName = varName[0].ToString().ToUpper() + varName[1..];

                                typeof(Material).GetProperty(varName).SetValue(material, value);
                            }
                        }
                        Console.WriteLine("Parsed " + materialFile);
                    }
                }
            }
        }

        private abstract class MoanaObject
        {
            protected static readonly MoanaConfiguration Configuration =
                CompilerConfiguration.Configuration.MoanaConfiguration;

            protected SceneNode Node { get; set; }
            public string Name { get; set; }
            public string FilePath { get; set; }

            public SceneNode GetSceneNode(MoanaCompilerV2 compiler)
            {
                if (Node == null) CreateSceneNode(compiler);
                return Node;
            }

            public abstract void CreateSceneNode(MoanaCompilerV2 compiler);
        }

        private class World : MoanaObject
        {
            public override void CreateSceneNode(MoanaCompilerV2 compiler)
            {
                using var str = new StreamReader(FilePath);
                var children = new List<string>();
                while (!str.EndOfStream)
                {
                    var line = str.ReadLine()!;
                    if (line.StartsWith("Include"))
                    {
                        line = line.Replace("Include ", "").Replace("\"", "");
                        if (Configuration.Includes.Any(x => line.Contains(x)))
                            children.Add(line);
                    }
                }

                Node = new SceneNode
                {
                    ForceEven = true,
                    Name = Name
                };
                compiler.Buffers.Add(Node);
                var childNodes = new List<SceneNode>();
                foreach (var child in children)
                {
                    var obj = new Object
                    {
                        FilePath = Path.Combine(compiler.BasePath, child),
                        Name = Path.GetFileNameWithoutExtension(child)
                    };
                    compiler.IncludedObjects.TryAdd(obj.FilePath, obj);
                    compiler.IncludedObjects.TryGetValue(obj.FilePath, out var add);
                    childNodes.Add(add!.GetSceneNode(compiler));
                }

                Node.Children = childNodes;
            }
        }

        private class Object : MoanaObject
        {
            public override void CreateSceneNode(MoanaCompilerV2 compiler)
            {
                var instances = new List<Instance>();

                using var stream = new StreamReader(FilePath);
                {
                    Instance instance = null;
                    while (!stream.EndOfStream)
                    {
                        var line = stream.ReadLine()!;
                        if (line.Contains("AttributeBegin"))
                        {
                            instance = new Instance();
                        }

                        if (instance != null)
                        {
                            if (line.Contains("Transform"))
                            {
                                var start = line.IndexOf("[", StringComparison.Ordinal) + 1;
                                var arrayStr = line.Substring(start, line.Length - start - 1);
                                instance.Transform = arrayStr.Split(" ")
                                    .Select(x => float.Parse(x, CultureInfo.InvariantCulture)).ToArray();
                            }

                            if (line.Contains(" ObjectInstance"))
                            {
                                var start = line.IndexOf("\"", StringComparison.Ordinal) + 1;
                                line = line[start..].Replace("\"", "");
                                instance.ObjectName = line;
                            }

                            if (line.Contains(" Include"))
                            {
                                instance.Children ??= new();
                                var start = line.IndexOf("\"", StringComparison.Ordinal) + 1;
                                line = line[start..].Replace("\"", "");
                                instance.Children.Add(line);
                            }
                        }

                        if (line.Contains("AttributeEnd"))
                        {
                            if (instance != null && instance.ObjectName == null && instance.Children == null)
                                instance = null;

                            if (instance != null)
                            {
                                instances.Add(instance);
                                instance = null;
                            }
                        }
                    }
                    Console.WriteLine("Parsed " + FilePath);
                }


                Node = new SceneNode
                {
                    Name = Name,
                    IsInstanceList = true
                };
                compiler.Buffers.Add(Node);
                var childNodes = new List<SceneNode>();
                foreach (var inst in instances)
                {
                    var instanceChildNodes = new List<SceneNode>();
                    var instanceNode = new SceneNode
                    {
                        Name = "Inst " + Name
                    };
                    compiler.Buffers.Add(instanceNode);
                    childNodes.Add(instanceNode);
                    // handle object
                    if (inst.ObjectName != null)
                    {
                        var directory = Path.GetFileName(Path.GetDirectoryName(FilePath));
                        var parentDirectory = Path.GetFileName(Path.GetDirectoryName(Path.GetDirectoryName(FilePath)));
                        var key1 = parentDirectory + directory + inst.ObjectName;
                        var key2 = Path.GetFileNameWithoutExtension(FilePath) + inst.ObjectName;
                        List<string> includes = compiler.Objects.GetValueOrDefault(key1, null) ??
                                                compiler.Objects.GetValueOrDefault(key2, null);
                        if (includes == null)
                            throw new Exception("Unresolved ObjectName");

                        foreach (var include in includes)
                        {
                            MoanaObject obj;
                            if (include.Contains("geometry.pbrt"))
                            {
                                obj = new GeometryFile
                                {
                                    FilePath = Path.Combine(compiler.BasePath, include),
                                    Name = Path.GetFileNameWithoutExtension(include)
                                };
                            }
                            else
                            {
                                obj = new Object
                                {
                                    FilePath = Path.Combine(compiler.BasePath, include),
                                    Name = Path.GetFileNameWithoutExtension(include)
                                };
                            }

                            compiler.IncludedObjects.TryAdd(obj.FilePath, obj);
                            compiler.IncludedObjects.TryGetValue(obj.FilePath, out var add);
                            instanceChildNodes.Add(add!.GetSceneNode(compiler));
                        }
                    }

                    // handle includes
                    if (inst.Children != null)
                    {
                        foreach (var child in inst.Children)
                        {
                            MoanaObject obj;
                            if (child.Contains("geometry.pbrt"))
                            {
                                obj = new GeometryFile
                                {
                                    FilePath = Path.Combine(compiler.BasePath, child),
                                    Name = Path.GetFileNameWithoutExtension(child)
                                };
                            }
                            else
                            {
                                obj = new Object
                                {
                                    FilePath = Path.Combine(compiler.BasePath, child),
                                    Name = Path.GetFileNameWithoutExtension(child)
                                };
                            }
                            compiler.IncludedObjects.TryAdd(obj.FilePath, obj);
                            compiler.IncludedObjects.TryGetValue(obj.FilePath, out var add);
                            instanceChildNodes.Add(add!.GetSceneNode(compiler));
                        }
                    }

                    instanceNode.Children = instanceChildNodes;


                    if (inst.Transform != null)
                    {
                        instanceNode.ObjectToWorld.M11 = inst.Transform[0];
                        instanceNode.ObjectToWorld.M21 = inst.Transform[4];
                        instanceNode.ObjectToWorld.M31 = inst.Transform[8];
                        instanceNode.ObjectToWorld.M41 = inst.Transform[12];

                        instanceNode.ObjectToWorld.M12 = inst.Transform[1];
                        instanceNode.ObjectToWorld.M22 = inst.Transform[5];
                        instanceNode.ObjectToWorld.M32 = inst.Transform[9];
                        instanceNode.ObjectToWorld.M42 = inst.Transform[13];

                        instanceNode.ObjectToWorld.M13 = inst.Transform[2];
                        instanceNode.ObjectToWorld.M23 = inst.Transform[6];
                        instanceNode.ObjectToWorld.M33 = inst.Transform[10];
                        instanceNode.ObjectToWorld.M43 = inst.Transform[14];

                        instanceNode.ObjectToWorld.M14 = inst.Transform[3];
                        instanceNode.ObjectToWorld.M24 = inst.Transform[7];
                        instanceNode.ObjectToWorld.M34 = inst.Transform[11];
                        instanceNode.ObjectToWorld.M44 = inst.Transform[15];
                    }
                }

                Node.Children = childNodes;
            }
        }

        private class GeometryFile : MoanaObject
        {

            public override void CreateSceneNode(MoanaCompilerV2 compiler)
            {
                var info = new FileInfo(FilePath);
                using var stream = new StreamReader(FilePath);
                List<Mesh> meshes = new();
                var sectionName = FilePath[(FilePath.IndexOf("pbrt") + 5)..];
                sectionName = sectionName.Split("/")[0];
                // Parse the file
                {
                    List<float> stageFlt = new();
                    List<uint> stageUInt = new();
                    var capacity = 500 * 3;
                    if (info.Length > 1024 * 1024 * 128)
                        capacity = 10000 * 3;
                    if (info.Length > 1024 * 1024 * 1024)
                        capacity = 500000 * 3;
                    Mesh mesh = null;
                    while (!stream.EndOfStream)
                    {
                        var line = stream.ReadLine()!;


                        if (line.Contains("AttributeBegin"))
                        {
                            mesh = new Mesh();
                        }

                        if (line.Contains("# Name \""))
                        {
                            var start = line.IndexOf("\"", StringComparison.Ordinal) + 1;
                            var name = line.Substring(start, line.Length - start - 1);
                            mesh = new Mesh
                            {
                                Name = name
                            };
                            line = stream.ReadLine();
                            if (line == null || !line.Contains("AttributeBegin"))
                                throw new Exception("No Attribute begin after # Name");
                        }

                        if (mesh != null)
                        {
                            if (line.Contains("Texture") && line.Contains("\"color\" \"ptex\""))
                            {
                                line = stream.ReadLine();
                                line = line.Replace("\"", "");
                                line = line.Replace(" ", "");
                                line = line.Replace("[", "");
                                line = line.Replace("]", "");
                                line = line.Replace("stringfilename../textures/", "");
                                var regex = new Regex(@$"(.*\\{sectionName}).*");
                                var sectionFolder = Path.Combine(regex.Replace(FilePath, "$1"));
                                regex = new Regex(@$".*\\{sectionName}");
                                var ptxPath = regex.Replace(line, "");
                                ptxPath = ptxPath.Replace(sectionName + "/", "");
                                ptxPath = ptxPath.Replace("/", "\\");
                                ptxPath = Path.Combine(sectionFolder, ptxPath);

                                if (File.Exists(ptxPath))
                                    mesh.TexturePath = ptxPath;
                            }

                            if (line.Contains("NamedMaterial"))
                            {
                                var start = line.IndexOf("\"", StringComparison.Ordinal) + 1;
                                mesh.MaterialName = line.Substring(start, line.Length - start - 1);
                            }

                            if (line.Contains("Shape \"trianglemesh\""))
                            {
                                mesh.Shape = new Shape();
                            }

                            if (mesh.Shape != null)
                            {
                                if (line.Contains("\"point P\"") || line.Contains("\"point3 P\""))
                                {
                                    line = stream.ReadLine()!.Trim();
                                    stageFlt.Capacity = capacity;
                                    do
                                    {
                                        stageFlt.AddRange(line.Split(" ")
                                            .Select(x => float.Parse(x, CultureInfo.InvariantCulture)));
                                        line = stream.ReadLine()!.Trim();
                                    } while (!line.Contains("]"));

                                    mesh.Shape.Positions.AddRange(stageFlt);
                                    stageFlt.Clear();
                                }

                                if (line.Contains("\"normal N\""))
                                {
                                    line = stream.ReadLine()!.Trim();
                                    stageFlt.Capacity = capacity;
                                    do
                                    {
                                        stageFlt.AddRange(line.Split(" ")
                                            .Select(x => float.Parse(x, CultureInfo.InvariantCulture)));
                                        line = stream.ReadLine()!.Trim();
                                    } while (!line.Contains("]"));

                                    mesh.Shape.Normals.AddRange(stageFlt);
                                    stageFlt.Clear();
                                }

                                if (line.Contains("\"point2 st\""))
                                {
                                    line = stream.ReadLine()!.Trim();
                                    stageFlt.Capacity = capacity;
                                    do
                                    {
                                        stageFlt.AddRange(line.Split(" ")
                                            .Select(x => float.Parse(x, CultureInfo.InvariantCulture)));
                                        line = stream.ReadLine()!.Trim();

                                    } while (!line.Contains("]"));

                                    mesh.Shape.Tex.AddRange(stageFlt);
                                    stageFlt.Clear();
                                }

                                if (line.Contains("\"integer indices\""))
                                {
                                    line = stream.ReadLine()!.Trim();
                                    stageUInt.Capacity = capacity;
                                    do
                                    {
                                        stageUInt.AddRange(line.Split(" ")
                                            .Select(x => uint.Parse(x, CultureInfo.InvariantCulture))
                                            .ToArray());
                                        line = stream.ReadLine()!.Trim();
                                    } while (!line.Contains("]"));

                                    mesh.Shape.Indices.AddRange(stageUInt);
                                    stageUInt.Clear();
                                }
                            }
                        }


                        if (line.Contains("AttributeEnd"))
                        {
                            meshes.Add(mesh);
                            mesh = null;
                        }
                    }

                    Console.WriteLine("Parsed " + FilePath);
                }


                Node = new SceneNode
                {
                    Name = "Geom " + Name,
                    ForceOdd = true
                };
                compiler.Buffers.Add(Node);

                if (meshes.Any())
                {
                    var buffers = compiler.Buffers;
                    Node.IndexBufferIndex = buffers.IndexBuffer.Count;

                    var indexOffset = compiler.Buffers.VertexBuffer.Count;
                    foreach (var mesh in meshes)
                    {
                        var materialIndex = -1;

                        if (mesh.MaterialName == null)
                        {
                            // this dataset is so bad sometimes, there are mesh duplicates and for the duplicate the material is missing...
                            var match = meshes.FirstOrDefault(x => x.Name == mesh.Name && x != mesh);
                            if (match != null && Math.Abs(match.Shape.Positions[0] - mesh.Shape.Positions[0]) < 0.001f)
                            {
                                Console.WriteLine("duplicate mesh in pbrt file, skipping:" + mesh.Name);
                                continue;
                            }
                            Console.WriteLine("unspecified material");
                        }

                        if (mesh.MaterialName != null)
                        {
                            // why do a normal material dictionary when you can hava a mess like in this scene
                            var subSection1 = mesh.Name.Split("_")[0];
                            var subSection2 = Path.GetFileNameWithoutExtension(FilePath).Split("_")[0];
                            if (compiler.Materials.TryGetValue(sectionName + "_" + mesh.MaterialName, out var material)) ;
                            else if (compiler.Materials.TryGetValue(subSection1 + "_" + mesh.MaterialName, out material)) ;
                            else if (compiler.Materials.TryGetValue(subSection1 + "1_" + mesh.MaterialName, out material)) ;
                            else if (compiler.Materials.TryGetValue(subSection2 + "_" + mesh.MaterialName, out material)) ;
                            else if (compiler.Materials.TryGetValue(subSection2 + "1_" + mesh.MaterialName, out material)) ;
                            else if (compiler.Materials.TryGetValue(mesh.MaterialName, out material)) ;
                            else Console.WriteLine("unresolved material " + mesh.MaterialName);
                            materialIndex = material.GetBufferIndex(buffers);
                        }

                        var start = buffers.VertexBuffer.Count; // add this to all indices
                        Node.NumTriangles += mesh.Shape.Indices.Count / 3;
                        if (!CompilerConfiguration.Configuration.MoanaConfiguration.MergeMeshes)
                            buffers.Add(Node);

                        buffers.IndexBuffer.AddRange(mesh.Shape.Indices.Select(x => (uint)(x + start)));

                        var vertices = new List<Vertex>(mesh.Shape.Positions.Count);
                        var num = mesh.Shape.Positions.Count / 3;
                        for (var i = 0; i < num; i++)
                        {
                            var vertex = new Vertex(i, mesh.Shape.Positions, mesh.Shape.Normals, mesh.Shape.Tex,
                                materialIndex, indexOffset);
                            vertices.Add(vertex);
                        }

                        buffers.VertexBuffer.AddRange(vertices);
                    }
                }
            }
        }

        public override void WriteTextures(FileStream str)
        {
            Console.WriteLine("Writing Textures: 0");
            str.Write(BitConverter.GetBytes(0));
        }
    }
}