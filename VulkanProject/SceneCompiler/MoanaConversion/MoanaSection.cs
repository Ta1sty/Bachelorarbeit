#define CLEANUP

using System;
using System.Buffers;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using SceneCompiler.Scene.SceneTypes;

namespace SceneCompiler.MoanaConversion
{

    public class Moana
    {
        public List<Texture> Textures { get; set; } = new();
        public List<MoanaSection> Sections = new();
        public SceneNode Node { get; set; }
        public SceneNode GetSceneNode(SceneBuffers buffers)
        {
            if (Node != null)
                return Node;

            Node = new SceneNode();
            Node.Name = "ROOT Moana Island";
            Node.Children.AddRange(Sections.Select(x=>x.GetSceneNode(buffers)));

#if CLEANUP
            Sections.Clear();
#endif
            buffers.Nodes.Add(Node);
            return Node;
        }
        public void SetReference()
        {
            foreach (var moanaSection in Sections)
            {
                moanaSection.SetReference(this);
            }
        }
    }

    public class MoanaSection
    {
        public Moana Moana { get; set; }
        public string Name { get; set; }
        public InstancedGeometry InstancedGeometry { get; set; }
        public List<Material> Materials { get; set; } = new();
        public ConcurrentBag<InstancedGeometry> InstancedGeometries { get; set; } = new();
        public SceneNode Node { get; set; }
        public SceneNode GetSceneNode(SceneBuffers buffers)
        {
            if (Node != null)
                return Node;

            Node = new SceneNode();
            Node.Name = "Sect " + Name;

            foreach (var material in Materials)
            {
                material.ToSceneMaterial(buffers, Moana.Textures);
            }

            Node.Children.AddRange(InstancedGeometry.InstanceLists
                .Select(x=>x.GetSceneNode(buffers)));

#if CLEANUP
            Materials.Clear();
            foreach (var instancedGeometry in InstancedGeometries)
            {
                instancedGeometry.Clear();
            }
            InstancedGeometries.Clear();
#endif
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
            buffers.Nodes.Add(Node);
            return Node;
        }
        public void SetReference(Moana moana)
        {
            Moana = moana;
            InstancedGeometry.SetReference(this);
            foreach (var instancedGeometry in InstancedGeometries)
            {
                instancedGeometry.SetReference(this);
            }
        }
    }

    public class InstancedGeometry
    {
        // this is just a container
        public MoanaSection Section { get; set; }
        public ConcurrentBag<InstanceList> InstanceLists { get; set; } = new();
        public ConcurrentBag<Geometry> Geometries { get; set; } = new();
        public void SetReference(MoanaSection section)
        {
            Section = section;
            foreach (var geometry in Geometries)
            {
                geometry.SetReference(this);
            }

            foreach (var instanceList in InstanceLists)
            {
                instanceList.SetReference(this);
            }
        }

        public void Clear()
        {
            InstanceLists.Clear();
            Geometries.Clear();
        }
    }

    public class InstanceList
    {
        public InstancedGeometry InstancedGeometry { get; set; }
        public string Name { get; set; }
        public List<Instance> Instances { get; set; } = new();
        public SceneNode Node { get; set; }
        public SceneNode GetSceneNode(SceneBuffers buffers)
        {
            if (Node != null)
                return Node;

            Node = new SceneNode();
            Node.Name = "List " + Name;
            Node.ForceEven = true;
            Node.IsInstanceList = true;
            Node.Children.AddRange(Instances.Select(x=>x.GetSceneNode(buffers)));
#if CLEANUP
            Instances.Clear();
#endif
            buffers.Nodes.Add(Node);
            GC.Collect();
            return Node;
        }
        public void SetReference(InstancedGeometry instancedGeometry)
        {
            InstancedGeometry = instancedGeometry;
            foreach (var instance in Instances)
            {
                instance.SetReference(this);
            }
        }
    }

    public class Instance
    {
        public InstanceList InstanceList { get; set; }
        public string ObjectName { get; set; }
        public float[] Transform { get; set; }
        public List<string> Children { get; set; } = new();
        public SceneNode Node { get; set; }
        public SceneNode GetSceneNode(SceneBuffers buffers)
        {
            if (Node != null)
                return Node;

            var objectToWorld = Matrix4x4.Identity;
            var worldToObject = Matrix4x4.Identity;
            if (Transform != null)
            {
                objectToWorld.M11 = Transform[0];
                objectToWorld.M21 = Transform[4];
                objectToWorld.M31 = Transform[8];
                objectToWorld.M41 = Transform[12];

                objectToWorld.M12 = Transform[1];
                objectToWorld.M22 = Transform[5];
                objectToWorld.M32 = Transform[9];
                objectToWorld.M42 = Transform[13];

                objectToWorld.M13 = Transform[2];
                objectToWorld.M23 = Transform[6];
                objectToWorld.M33 = Transform[10];
                objectToWorld.M43 = Transform[14];

                objectToWorld.M44 = 1;

                Matrix4x4.Invert(objectToWorld, out worldToObject);
                objectToWorld.M14 = Transform[3];
                objectToWorld.M24 = Transform[7];
                objectToWorld.M34 = Transform[11];
                objectToWorld.M44 = Transform[15];

                worldToObject.M14 = -Transform[3];
                worldToObject.M24 = -Transform[7];
                worldToObject.M34 = -Transform[11];
                worldToObject.M44 = Transform[15];
            }

            Node = new SceneNode
            {
                ObjectToWorld = objectToWorld,
                Name = "Inst " + ObjectName,
                ForceEven = true
            };

            Node.Children.AddRange(Children.Select(name=> InstanceList.InstancedGeometry.Section.InstancedGeometries
                .SelectMany(x => x.InstanceLists).Single(x => x.Name == name)
                .GetSceneNode(buffers)));

            if (ObjectName != null)
            {
                Node.Children.Add(InstanceList.InstancedGeometry
                    .Geometries.Single(x => x.Name == ObjectName)
                    .GetSceneNode(buffers));
            }

            buffers.Nodes.Add(Node);
            return Node;
        }
        public void SetReference(InstanceList instanceList)
        {
            InstanceList = instanceList;
        }
    }

    public class Geometry
    {
        public InstancedGeometry InstancedGeometry { get; set; }
        public string Name { get; set; }
        public List<Mesh> Meshes { get; set; } = new();
        public SceneNode Node { get; set; }
        public SceneNode GetSceneNode(SceneBuffers buffers)
        {
            if (Node != null)
                return Node;

            Node = new SceneNode();
            Node.Name = "Geom " + Name;
            Node.ForceEven = true;
            Node.Children.AddRange(Meshes.Select(x=>x.GetSceneNode(buffers)).Where(x=>x != null));
#if CLEANUP
            Meshes.Clear();
#endif
            bool merge = true; // we need to do this, else programm will construct a blas for
                               // every single mesh and we exceed vkDeviceMaxAllocations (4096)
            if (merge && Node.Children.Count > 0) // merges the meshes of the children all into one big mesh
            {
                Node.NumTriangles = Node.Children.Sum(x => x.NumTriangles);
                Node.IndexBufferIndex = Node.Children.Min(x => x.IndexBufferIndex);
                Node.ForceOdd = true;
                Node.ForceEven = false;
                Node.Children.Clear();
            }
            buffers.Nodes.Add(Node);
            return Node;
        }
        public void SetReference(InstancedGeometry instancedGeometry)
        {
            InstancedGeometry = instancedGeometry; 
            foreach (var mesh in Meshes)
            {
                mesh.SetReference(this);
            }
        }
    }

    public class Mesh
    {
        public Geometry Geometry { get; set; }
        public string Name { get; set; }
        public Shape Shape { get; set; }
        public string MaterialName { get; set; }
        public SceneNode Node { get; set; }
        public SceneNode GetSceneNode(SceneBuffers buffers)
        {    
            if (Node != null)
                return Node;

            var materialIndex = -1;
            if (MaterialName != null)
            {
                materialIndex = Geometry.InstancedGeometry.Section.Materials.Single(x => x.Name == MaterialName).BufferIndex;
                if (materialIndex == -1)
                {
                    materialIndex = Geometry.InstancedGeometry.Section.Materials
                        .FirstOrDefault(x => x.Name.Contains(MaterialName))?
                        .BufferIndex ?? -1;
                }
                if (materialIndex >= 0)
                {
                    var mat = buffers.MaterialBuffer[materialIndex];
                    //Console.WriteLine(Name+ " Mapped Material: " + MaterialName + " to " + mat.Name);
                }
                else
                {
                    Console.WriteLine(Name + " Unmapped Material: " + MaterialName);
                }
            }

            var start = buffers.VertexBuffer.Count; // add this to all indices
            Node = new SceneNode
            {
                IndexBufferIndex = buffers.IndexBuffer.Count,
                NumTriangles = Shape.Indices.Count / 3,
                Name = "Mesh " + Name,
                ForceOdd = true
            };
            buffers.IndexBuffer.AddRange(Shape.Indices.Select(x=>(uint) (x+start)));

            var vertices = new List<Vertex>(Shape.Positions.Count);
            for (var i = 0; i < Shape.Positions.Count; i++)
            {
                var vertex = new Vertex(Shape.Positions[i], Shape.Normals[i], Shape.Tex[i], materialIndex);
                vertices.Add(vertex);
            }

            buffers.VertexBuffer.AddRange(vertices);

#if CLEANUP
            Shape.Positions.Clear();
            Shape.Normals.Clear();
            Shape.Tex.Clear();
            Shape.Indices.Clear();
#endif
            buffers.Nodes.Add(Node);
            return Node;
        }

        public void SetReference(Geometry geometry)
        {
            Geometry = geometry;
        }
    }

    public class Shape
    {
        public List<float[]> Positions { get; set; } = new();
        public List<float[]> Normals { get; set; } = new();
        public List<float[]> Tex { get; set; } = new();
        public List<uint> Indices { get; set; } = new();
    }

    public class Material
    {
        public string Name { get; set; }
        public string Type { get; set; }
        public float[] Color { get; set; }
        public float Spectrans { get; set; }
        public float Clearcoatgloss { get; set; }
        public float Speculartint { get; set; }
        public float Eta { get; set; }
        public float Sheentint { get; set; }
        public float Metallic { get; set; }
        public float Anisotropic { get; set; }
        public float Clearcoat { get; set; }
        public float Roughness { get; set; }
        public float Sheen { get; set; }
        public bool Thin { get; set; }
        public float Difftrans { get; set; }
        public float Flatness { get; set; }
        public float[] Scatterdistance { get; set; }
        public Scene.SceneTypes.Material SceneMaterial { get; set; }
        public int BufferIndex { get; set; }

        public void ToSceneMaterial(SceneBuffers buffers, List<Texture> textures)
        {
            var index = textures.Count;
            textures.Add(new Texture
            {
                Source = this
            });
            SceneMaterial = new Scene.SceneTypes.Material
            {
                DoubleSided = true,
                PbrMetallicRoughness = new MaterialProperties
                {
                    BaseColorTexture = new ColorTexture
                    {
                        Index = index
                    },
                    MetallicFactor = Metallic,
                    RoughnessFactor = Roughness
                },
                Name = Name
            };
            BufferIndex = buffers.MaterialBuffer.Count;
            buffers.MaterialBuffer.Add(SceneMaterial);
        }
    }

    public class Texture
    { 
        public Material Source { get; set; }
    }
}