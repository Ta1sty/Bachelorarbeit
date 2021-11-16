using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using GLTFCompiler.GltfFileTypes;

namespace GLTFCompiler.Scene
{
    public class SceneData
    {
        public readonly GltfFile file;
        public SceneData(GltfFile file)
        {
            this.file = file;
        }

        public List<BufferReader> usedBuffers = new();
        public List<Vertex> VertexBuffer { get; set; } = new();
        public List<uint> IndexBuffer { get; set; } = new();
        public List<byte[]> Buffers { get; set; } = new();
        public List<MeshData> Meshes { get; set; } = new();
        public List<SceneNode> Nodes { get; set; } = new(); // the last node is the root node, this is due to the format of gltf
        public void DecodeBuffers()
        {
            foreach (var buffer in file.Buffers)
            {
                var start = buffer.Uri.IndexOf(";base64,", StringComparison.CurrentCultureIgnoreCase);
                var span = buffer.Uri.Substring(start+8);
                var bytes = Convert.FromBase64String(span);
                Buffers.Add(bytes);
            }
        }
        public void ParseMeshes()
        {
            foreach (var mesh in file.Meshes)
            {
                uint start = (uint) IndexBuffer.Count;
                foreach (var arr in mesh.Primitives)
                {
                    var positionAccessor = file.Accessors[arr.Attributes.Position];
                    var normalAccessor = file.Accessors[arr.Attributes.Normal];
                    var texCoordAccessor = file.Accessors[arr.Attributes.TexCoord_0];

                    int numElements = positionAccessor.Count;
                    if (numElements !=normalAccessor.Count || numElements != texCoordAccessor.Count)
                    {
                        throw new Exception("element counts do not match for vertex");
                    }
                    var posReader = new BufferReader(this, positionAccessor);
                    var norReader = new BufferReader(this, normalAccessor);
                    var texReader = new BufferReader(this, texCoordAccessor);

                    usedBuffers.Add(posReader);
                    usedBuffers.Add(norReader);
                    usedBuffers.Add(texReader);

                    uint vertexStart = (uint)VertexBuffer.Count;

                    for (var i = 0; i < numElements; i++)
                    {
                        var ver = new Vertex
                        {
                            Position = posReader.GetVec3(),
                            Normal = norReader.GetVec3(),
                            TexCoords = texReader.GetVec2(),
                            MaterialIndex = arr.Material
                        };
                        VertexBuffer.Add(ver);
                    }
                    var faceAccessor = file.Accessors[arr.Indices];
                    var faceReader = new BufferReader(this, faceAccessor);

                    usedBuffers.Add(faceReader);
                    if (true) // vertex array
                    {
                        for (var i = 0; i < faceAccessor.Count; i+=3)
                        {
                            uint v1 = faceReader.GetUshort();
                            uint v2 = faceReader.GetUshort();
                            uint v3 = faceReader.GetUshort();
                            if (v1 == v2 || v2 == v3 || v3 == v1) continue; // just a line
                            IndexBuffer.Add(v1 + vertexStart);
                            IndexBuffer.Add(v2 + vertexStart);
                            IndexBuffer.Add(v3 + vertexStart);
                        }
                    }
                    else // triangle strip
                    {
                        var v1 = uint.MaxValue;
                        var v2 = uint.MaxValue;
                        var v3 = uint.MaxValue;
                        for (var i = 0; i < faceAccessor.Count; i++)
                        {
                            v1 = v2;
                            v2 = v3;
                            v3 = faceReader.GetUshort();
                            if (v1 != uint.MaxValue)
                            {
                                if(v1 == v2 || v2 == v3 || v3 == v1) continue; // just a line
                                IndexBuffer.Add(v1 + vertexStart);
                                IndexBuffer.Add(v2 + vertexStart);
                                IndexBuffer.Add(v3 + vertexStart);
                            }
                        }
                    }
                }

                uint end = (uint) IndexBuffer.Count;
                var length = end - start;
                Meshes.Add(new MeshData
                {
                    IndexStart = start,
                    Length = length
                });
            }
        }

        public void BuildSceneGraph()
        {
            foreach (var node in file.Nodes)
            {
                var scNode = new SceneNode();
                if (node.Mesh == -1) // this node does not point to any geometry
                {
                    scNode.Children = node.Children.Select(x => Nodes[x]).ToList();
                    scNode.NumChildren = scNode.Children.Count;
                }
                else
                {
                    scNode.Children = node.Children.Select(x => Nodes[x]).ToList();
                    scNode.NumChildren = scNode.Children.Count;


                    scNode.IndexBufferIndex = (int) Meshes[node.Mesh].IndexStart;
                    scNode.NumTriangles = (int) Meshes[node.Mesh].Length / 3;
                }

                var rotation = Matrix4x4.Identity;
                var translation = Matrix4x4.Identity;
                var scale = Matrix4x4.Identity;
                if (node.Rotation != null)
                {
                    rotation = Matrix4x4.CreateFromQuaternion(
                        new Quaternion(node.Rotation[0], node.Rotation[1], node.Rotation[2], node.Rotation[3]));
                }

                if (node.Scale != null)
                {
                    scale = Matrix4x4.CreateScale(node.Scale[0], node.Scale[1], node.Scale[2]);
                }

                if (node.Translation != null)
                {
                    translation = Matrix4x4.CreateTranslation(node.Translation[0], node.Translation[1], node.Translation[2]);
                }

                scNode.Transform =  scale * rotation * translation; 
                scNode.Source = node;
                Nodes.Add(scNode);
            }
            var end = new SceneNode
            {
                Children = file.Scenes[0].Nodes.Select(x=>Nodes[x]).ToList(),
                NumChildren = file.Scenes[0].Nodes.Count,
                Transform = Matrix4x4.Identity,
            };

            var arr = Nodes.ToArray();
            for(int i = 0;i<arr.Length;i++) // might change this
            {
                for (int j = i + 1; j < arr.Length; j++)
                {
                    if (arr[i].Brother == null && arr[i].Simmilar(arr[j]))
                    {
                        arr[j].Brother = arr[i];
                    }
                }
            }

            foreach (var node in Nodes)
            {
                node.Children = node.Children.Select(x => x.ThisOrBrother()).ToList();
            }

            Nodes = Nodes.Where(x => x.Brother == null).ToList();
            end.Children = end.Children.Select(x => x.ThisOrBrother()).ToList();
            end.Children = end.Children
                .Where(x =>
                {
                    return true;
                    if (x.Source.Mesh<0) return true;
                    if (x.Children.Count > 0) return true;
                    if (x.Transform.M41 != 0) return true;
                    if (x.Transform.M42 != 0) return true;
                    if (x.Transform.M43 != 0) return true;
                    return false;
                }).ToList();
            end.NumChildren = end.Children.Count;
            var index = 0;
            Nodes.Add(end);
            foreach (var node in Nodes)
            {
                node.Index = index;
                index++;
            }
        }

        public void WriteBytes(string dst)
        {
            using var writer = new SceneDataWriter(dst);
            writer.WriteVertices(VertexBuffer);
            writer.WriteIndices(IndexBuffer);
            writer.WriteSceneNodes(Nodes);
            writer.WriteMaterials(file.Materials);
            writer.WriteTextures(this);
        }
    }
}