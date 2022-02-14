using System;
using System.Collections.Generic;
using System.Drawing.Drawing2D;
using System.Globalization;
using SceneCompiler.Scene;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading.Tasks;
using SceneCompiler.Scene.SceneTypes;

namespace SceneCompiler.MoanaConversion
{
    public class MoanaCompiler : ASceneCompiler
    {
        public List<Texture> Textures = new();
        public override void CompileScene(string path)
        {
            var folders = Directory.GetDirectories(path);

            var moana = new Moana();
            foreach (var folder in folders)
            {
                //if (Path.GetFileName(folder).Contains("isKava"))
                //if (Path.GetFileName(folder).Contains("isIronwoodA1")) moana.Sections.Add(ReadFolder(folder));
                //if (Path.GetFileName(folder).Contains("isIronwoodB")) moana.Sections.Add(ReadFolder(folder));
                //if (Path.GetFileName(folder).Contains("isBayCedar")) moana.Sections.Add(ReadFolder(folder));
                //if (Path.GetFileName(folder).Contains("isCoral")) moana.Sections.Add(ReadFolder(folder));
                //if (Path.GetFileName(folder).Contains("isBeach")) moana.Sections.Add(ReadFolder(folder));
                if (Path.GetFileName(folder).Contains("isCoastline")) moana.Sections.Add(ReadFolder(folder));
                //if (Path.GetFileName(folder).Contains("isMountainB")) moana.Sections.Add(ReadFolder(folder));
                //if (Path.GetFileName(folder).Contains("isCoastline")) moana.Sections.Add(ReadFolder(folder));

                //if(Environment.WorkingSet > 1L * 512 * 1024 * 1024)
                //    break;  
            }
            ValidateMoana(moana);
            moana.SetReference();
            Console.WriteLine("Creating Scenegraph");
            var root = moana.GetSceneNode(Buffers);

            Console.WriteLine("Setting Parents and indices");
            for (var i = 0; i < Buffers.Nodes.Count; i++)
            {
                Buffers.Nodes[i].Index = i;
                Buffers.Nodes[i].NumChildren = Buffers.Nodes[i].Children.Count;
                foreach (var child in Buffers.Nodes[i].Children)
                {
                    child.Parents.Add(Buffers.Nodes[i]);
                }
            }

            Textures = moana.Textures;
            Buffers.RootNode = root.Index;

            /*var add = new List<SceneNode>();
            var lodCreator = new LodCreator(Buffers);


            foreach (var node in Buffers.Nodes.Where(x=>x.IsInstanceList))
            {
                // add.AddRange(lodCreator.CreateLevelsOfDetail(node, 1));
            }
            Buffers.Nodes.AddRange(add);
            */
            ValidateSceneGraph(moana);
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
        }

        private void ValidateSceneGraph(Moana moana)
        {
            foreach (var vertex in Buffers.VertexBuffer)
            {
                if(vertex.MaterialIndex == -1)
                    continue;
                var material = Buffers.MaterialBuffer[vertex.MaterialIndex];
                var texture = Textures[material.PbrMetallicRoughness.BaseColorTexture.Index];
                if (texture == null)
                    throw new Exception("Texture does not exist");
            }
        }

        private void ValidateMoana(Moana moana)
        {
            Console.WriteLine("Validating...");
            Parallel.ForEach(moana.Sections, section =>
            {
                Console.WriteLine(section.Name);
                Parallel.ForEach(section.InstancedGeometry.InstanceLists.SelectMany(x => x.Instances), instance =>
                {
                    Geometry geometry = null;
                    if (instance.ObjectName.Contains(section.Name))
                        geometry = section.InstancedGeometry.Geometries.Single(x => x.Name == instance.ObjectName);

                    if (geometry == null)
                        throw new Exception("Unresolved mapping for:" + instance.ObjectName);

                    Parallel.ForEach(instance.Children, child =>
                    {
                        var map = section.InstancedGeometries
                            .SelectMany(x => x.InstanceLists).SingleOrDefault(x => x.Name == child);
                        if (map == null)
                            throw new Exception("Unresolved mapping for:" + child);
                    });
                });

                Parallel.ForEach(section.InstancedGeometry.Geometries.SelectMany(x => x.Meshes), mesh =>
                {
                    if (mesh.MaterialName == null)
                    {
                        Console.WriteLine("Missing material for mesh:" + mesh.Name);
                    } else
                    {
                        var map = section.Materials.SingleOrDefault(x => x.Name == mesh.MaterialName);
                        if (map == null)
                            throw new Exception("Unresolved mapping for material:" + mesh.MaterialName);
                    }
                 });

                Parallel.ForEach(section.InstancedGeometries, instancedGeometry =>
                {
                    Parallel.ForEach(instancedGeometry.InstanceLists.SelectMany(x => x.Instances), instance =>
                    {
                        var map = instancedGeometry.Geometries.SingleOrDefault(x => x.Name == instance.ObjectName);
                        if (map == null)
                            throw new Exception("Unresolved mapping for:" + instance.ObjectName);
                    });

                    Parallel.ForEach(instancedGeometry.Geometries.SelectMany(x => x.Meshes), mesh =>
                    {
                        if (mesh.MaterialName == null)
                        {
                            Console.Write("Missing material for mesh:" + mesh.Name);
                        } else
                        {
                            var map = section.Materials.SingleOrDefault(x => x.Name == mesh.MaterialName);
                            if (map == null)
                            {
                                map = section.Materials.FirstOrDefault(x => x.Name.Contains(mesh.MaterialName));
                                if (map == null) throw new Exception("Unresolved mapping for material:" + mesh.MaterialName);
                            }
                        }
                    });
               });
            });
            Console.WriteLine("Validation successful");
        }

        private MoanaSection ReadFolder(string folder)
        {
            var section = new MoanaSection();
            var sectionName = Path.GetFileName(folder);
            section.Name = sectionName;
            var folders = Directory.GetDirectories(folder);

            var tasks = new List<Task>();

            tasks.Add(Task.Run(() => section.InstancedGeometry = GetInstancedGeometry(section, folder)));

            tasks.AddRange(folders.Select(x=>Task.Run(
                () => section.InstancedGeometries.Add(GetInstancedGeometry(section, x))
                )));


            tasks.Add(Task.Run(
                () => section.Materials = ParseMaterials(Directory.GetFiles(folder).Single(x => x.Contains("materials")))
                ));

            Task.WaitAll(tasks.ToArray());

            GC.Collect();
            return section;
        }

        private InstancedGeometry GetInstancedGeometry(MoanaSection section, string directory)
        {
            var tasks = new List<Task>();

            var instancedGeometry = new InstancedGeometry();

            var subDirectoryName = Path.GetFileName(directory);
            List<string> instanceFiles;

            if (subDirectoryName == section.Name)
            {
                instanceFiles = Directory.GetFiles(directory).Select(Path.GetFileName)
                    .Where(x => x.Contains(section.Name) && !x.Contains("geometry"))
                    .ToList();
            }
            else
            {
                instanceFiles = Directory.GetFiles(directory).Select(Path.GetFileName)
                    .Where(x => x.Contains(section.Name) && x.Contains(subDirectoryName) && !x.Contains("geometry"))
                    .ToList();
            }

            tasks.AddRange(instanceFiles.Select(x => Task.Run(() =>
            {
                instancedGeometry.InstanceLists.Add(ParseInstancing(Path.Combine(directory, x)));
            })));


            tasks.AddRange(Directory.GetFiles(directory)
                .Select(Path.GetFileName).Where(x => x.Contains("_geometry.pbrt")).Select(x => Task.Run(() =>
            {
                instancedGeometry.Geometries.Add(ParseGeometryFile(Path.Combine(directory, x)));
            })));

            Task.WaitAll(tasks.ToArray());

            return instancedGeometry;
        }

        private Geometry ParseGeometryFile(string path)
        {
            var info = new FileInfo(path);
            using var stream = new StreamReader(path);
            var geometry = new Geometry();
            geometry.Name = Path.GetFileName(path).Replace("_geometry.pbrt", "");

            List<float[]> stageFlt = new();
            List<uint> stageUInt = new();
            var capacity = 500;
            if (info.Length > 1024 * 1024 * 128)
                capacity = 10000;
            if (info.Length > 1024 * 1024 * 1024)
                capacity = 500000;

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
                            do
                            {
                                stageFlt.Capacity = capacity;
                                stageFlt.Add(line.Split(" ")
                                    .Select(x=> float.Parse(x, CultureInfo.InvariantCulture))
                                    .ToArray());
                                line = stream.ReadLine()!.Trim();
                                mesh.Shape.Positions.AddRange(stageFlt);
                                stageFlt.Clear();
                            } while (!line.Contains("]"));
                        }
                        if (line.Contains("\"normal N\""))
                        {
                            line = stream.ReadLine()!.Trim();
                            do
                            {
                                stageFlt.Capacity = capacity;
                                stageFlt.Add(line.Split(" ")
                                    .Select(x => float.Parse(x, CultureInfo.InvariantCulture))
                                    .ToArray());
                                line = stream.ReadLine()!.Trim();
                                mesh.Shape.Normals.AddRange(stageFlt);
                                stageFlt.Clear();
                            } while (!line.Contains("]"));
                        }
                        if (line.Contains("\"point2 st\""))
                        {
                            line = stream.ReadLine()!.Trim();
                            do
                            {
                                stageFlt.Capacity = capacity;
                                stageFlt.Add(line.Split(" ")
                                    .Select(x => float.Parse(x, CultureInfo.InvariantCulture))
                                    .ToArray());
                                line = stream.ReadLine()!.Trim();
                                mesh.Shape.Tex.AddRange(stageFlt);
                                stageFlt.Clear();
                            } while (!line.Contains("]"));
                        }

                        if (line.Contains("\"integer indices\""))
                        {
                            line = stream.ReadLine()!.Trim();
                            do
                            {
                                stageUInt.Capacity = capacity;
                                stageUInt.AddRange(line.Split(" ")
                                    .Select(x => uint.Parse(x, CultureInfo.InvariantCulture))
                                    .ToArray());
                                line = stream.ReadLine()!.Trim();
                                mesh.Shape.Indices.AddRange(stageUInt);
                                stageUInt.Clear();
                            } while (!line.Contains("]"));
                        }
                    }
                }


                if (line.Contains("AttributeEnd"))
                {
                    geometry.Meshes.Add(mesh);
                    mesh = null;
                }
            }
            Console.WriteLine("Parsed " + path);

            return geometry;
        }

        private InstanceList ParseInstancing(string instanceFile)
        {
            var info = new FileInfo(instanceFile);
            using var stream = new StreamReader(instanceFile);

            var list = new InstanceList();
            list.Name = Path.GetFileName(instanceFile).Replace(".pbrt", "");
            Instance instance = null;
            var capacityEstimate = info.Length * 4 / 1024;
            list.Instances.Capacity = (int) capacityEstimate;
            while (!stream.EndOfStream)
            { 
                var line = stream.ReadLine();
                if (line.Contains("AttributeBegin"))
                {
                    instance = new Instance();
                }

                if (instance != null)
                {
                    if (line.Contains("Transform"))
                    {
                        var start = line.IndexOf("[", StringComparison.Ordinal);
                        var arrayStr = line.Substring(start).Replace(" ", ",");
                        instance.Transform = JsonSerializer.Deserialize<float[]>(arrayStr);
                    }

                    if (line.Contains(" ObjectInstance"))
                    {
                        var start = line.IndexOf("\"", StringComparison.Ordinal) + 1;
                        var name = line.Substring(start, line.Length - start - 1);
                        instance.ObjectName = name;
                    }
                    
                    if (line.Contains(" Include"))
                    {
                        var start = line.LastIndexOf("/", StringComparison.Ordinal) + 1;
                        var end = line.LastIndexOf(".", StringComparison.Ordinal);
                        instance.Children.Add(line.Substring(start, end-start));
                    }
                }

                if (line.Contains("AttributeEnd"))
                {
                    if (instance != null && instance.ObjectName == null && instance.Children.Count == 0)
                        instance = null;

                    if (instance != null)
                    {
                        list.Instances.Add(instance);
                        instance = null;
                    }
                }
            }
            Console.WriteLine("Parsed " + instanceFile);
            list.Instances.TrimExcess();
            return list;
        }

        private List<Material> ParseMaterials(string materialFile)
        {
            using var stream = new StreamReader(materialFile);
            var materials = new List<Material>();
            Material material = null;
            while (!stream.EndOfStream)
            {
                var line = stream.ReadLine()!;

                if (line.StartsWith("MakeNamedMaterial"))
                {
                    material = new Material();
                    var start = line.IndexOf("\"", StringComparison.Ordinal);
                    material.Name = line.Substring(start+1, line.Length - start - 2);
                }

                if (line.StartsWith("#End MakeNamedMaterial"))
                {
                    materials.Add(material);
                    material = null;
                }

                if (material != null)
                {
                    if(!line.Contains("["))
                        continue;
                    line = line.Replace("#", "");
                    var varDeclaration = line[..line.IndexOf("[", StringComparison.Ordinal)]
                        .Replace("\"", "")
                        .Trim();

                    var varType = varDeclaration.Split(" ")[0];
                    var varName = varDeclaration.Split(" ")[1];

                    var varValue = line[line.IndexOf("[", StringComparison.Ordinal)..]
                        .Replace("[","")
                        .Replace("]", "")
                        .Replace("\"", "");

                    object value = default;
                    if (varType.Contains("float"))
                        value = float.Parse(varValue);
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

            return materials;
        }

        public override void WriteTextures(FileStream str)
        {
            Console.WriteLine("Writing Textures: " + Textures.Count);
            str.Write(BitConverter.GetBytes(Textures.Count));
            foreach (var texture in Textures)
            {
                var width = 2;
                var height = 2;
                var size = width * height * sizeof(int);
                str.Write(BitConverter.GetBytes(width));
                str.Write(BitConverter.GetBytes(height));
                str.Write(BitConverter.GetBytes(size));

                var dst = new byte[size];
                var i = 0;
                for (var y = 0; y < height; y++)
                {
                    for (var x = 0; x < width; x++)
                    {
                        byte red = (byte)(texture.Source.Color[0] * 255);
                        byte green = (byte)(texture.Source.Color[1] * 255);
                        byte blue = (byte)(texture.Source.Color[2] * 255);
                        dst[i] = red;
                        dst[i + 1] = green;
                        dst[i + 2] = blue;
                        dst[i + 3] = 255;
                        i += 4;
                    }
                }
                str.Write(dst);
            }
        }
    }
}