﻿using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text.Json;
using Scene;
using SceneCompiler.GLTFConversion.GltfFileTypes;
using SceneCompiler.Scene;

namespace SceneCompiler.GLTFConversion.Compilation
{
    public class GltfCompiler : ASceneCompiler
    {
        internal GltfFile File { get; set; }
        public GltfCompiler(SceneBuffers buffers) : base(buffers)
        {

        }
        private readonly List<BufferReader> usedReaders = new();
        internal List<byte[]> GltfBuffers { get; set; } = new();
        private List<MeshData> Meshes { get; set; } = new();
        private void DecodeBuffers()
        {
            foreach (var buffer in File.Buffers)
            {
                var start = buffer.Uri.IndexOf(";base64,", StringComparison.CurrentCultureIgnoreCase);
                var span = buffer.Uri.Substring(start+8);
                var bytes = Convert.FromBase64String(span);
                GltfBuffers.Add(bytes);
            }
        }
        private void ParseMeshes()
        {
            var indexBuffer = Buffers.IndexBuffer;
            var vertexBuffer = Buffers.VertexBuffer;
            foreach (var mesh in File.Meshes)
            {
                var start = indexBuffer.Count;
                foreach (var arr in mesh.Primitives)
                {
                    var positionAccessor = File.Accessors[arr.Attributes.Position];
                    var normalAccessor = File.Accessors[arr.Attributes.Normal];
                    var texCoordAccessor = File.Accessors[arr.Attributes.TexCoord_0];

                    int numElements = positionAccessor.Count;
                    if (numElements !=normalAccessor.Count || numElements != texCoordAccessor.Count)
                    {
                        throw new Exception("element counts do not match for vertex");
                    }
                    var posReader = new BufferReader(this, positionAccessor);
                    var norReader = new BufferReader(this, normalAccessor);
                    var texReader = new BufferReader(this, texCoordAccessor);

                    usedReaders.Add(posReader);
                    usedReaders.Add(norReader);
                    usedReaders.Add(texReader);

                    int vertexStart = vertexBuffer.Count;

                    for (var i = 0; i < numElements; i++)
                    {
                        var ver = new Vertex(posReader.GetVec3(), norReader.GetVec3(), texReader.GetVec2(), arr.Material, vertexStart);
                        vertexBuffer.Add(ver);
                    }
                    var faceAccessor = File.Accessors[arr.Indices];
                    var faceReader = new BufferReader(this, faceAccessor);

                    usedReaders.Add(faceReader);
                    if (true) // vertex array
                    {
                        uint v1, v2, v3;
                        for (var i = 0; i < faceAccessor.Count; i+=3)
                        {
                            if (faceAccessor.ComponentType == 5123)
                            {
                                v1 = faceReader.GetUshort();
                                v2 = faceReader.GetUshort();
                                v3 = faceReader.GetUshort();
                            }
                            else if (faceAccessor.ComponentType == 5125)
                            {
                                v1 = faceReader.GetUint();
                                v2 = faceReader.GetUint();
                                v3 = faceReader.GetUint();
                            }
                            else throw new Exception("Component type not implemented");

                            if (v1 == v2 || v2 == v3 || v3 == v1) continue; // just a line
                            indexBuffer.Add((uint)(v1 + vertexStart));
                            indexBuffer.Add((uint)(v2 + vertexStart));
                            indexBuffer.Add((uint)(v3 + vertexStart));
                        }
                    }
                }

                uint end = (uint) indexBuffer.Count;
                var length = end - start;
                Meshes.Add(new MeshData
                {
                    IndexStart = (uint)start,
                    Length = (uint)length
                });
            }
        }

        private void BuildSceneGraph()
        {
            foreach (var node in File.Nodes)
            {
                var scNode = new SceneNode();
                Buffers.Add(scNode);

                if (node.Mesh == -1) // this node does not point to any geometry
                {
                    scNode.Children = node.Children.Select(x => Buffers.NodeByIndex(x)).ToList();
                }
                else
                {
                    scNode.Children = node.Children.Select(x => Buffers.NodeByIndex(x)).ToList();

                    scNode.IndexBufferIndex = (int) Meshes[node.Mesh].IndexStart;
                    scNode.NumTriangles = (int) Meshes[node.Mesh].Length / 3;
                }

                var rotation = Matrix4x4.Identity;
                var invRotation = Matrix4x4.Identity;
                var translation = Matrix4x4.Identity;
                var invTranslation = Matrix4x4.Identity;
                var scale = Matrix4x4.Identity;
                var invScale = Matrix4x4.Identity;
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

                scNode.ObjectToWorld =  scale * rotation * translation;
                // TEST
                /*
                var v = new Vector4(4, -3, 2, 1);
                var res1 = Vector4.Transform(v, scNode.ObjectToWorld * scNode.WorldToObject);
                var res2 = Vector4.Transform(v, scNode.WorldToObject * scNode.ObjectToWorld);
                */

                scNode.Name = node.Name;
            }
            var end = new SceneNode
            {
                ObjectToWorld = Matrix4x4.Identity,
                Name = "ROOT"
            };
            Buffers.Add(end);
            end.Children = File.Scenes[0].Nodes.Select(x => Buffers.NodeByIndex(x)).ToList();

            if (CompilerConfiguration.Configuration.GltfConfiguration.RemoveDuplicates)
            {
                var arr = Buffers.Nodes.ToArray();
                for (int i = 0; i < arr.Length; i++) // might change this
                {
                    for (int j = i + 1; j < arr.Length; j++)
                    {
                        if (arr[i].Brother == null && arr[i].Simmilar(arr[j]))
                        {
                            arr[j].Brother = arr[i];
                        }
                    }
                }

                foreach (var node in Buffers.Nodes)
                {
                    node.Children = node.Children.Select(x => x.ThisOrBrother()).ToList();
                }
                Buffers.SetSceneNodes(Buffers.Nodes.Where(x => x.Brother == null));

                end.Children = end.Children.Select(x => x.ThisOrBrother()).ToList();
            }

            if (CompilerConfiguration.Configuration.GltfConfiguration.RemovePresumedInstances)
            {
                if (end.Children.Count(x => x.NumTriangles >= 0) == 1) // scene consists of only one mesh with a scene node
                {

                }
                else // scene consists of more than one, assume instancing
                {
                    end.Children = end.Children
                        .Where(x =>
                        {
                            if (x.NumTriangles <= 0) return true;
                            if (x.Children.Any()) return true;
                            if (x.ObjectToWorld.M41 != 0) return true;
                            if (x.ObjectToWorld.M42 != 0) return true;
                            if (x.ObjectToWorld.M43 != 0) return true;
                            return false;
                        }).ToList();
                }
            }

            Buffers.RewriteAllParents();

            Buffers.Root = end;
            // because the node for the scene is added last
        }

        public override void CompileScene(string path)
        {
            using var str = new StreamReader(path);
            var res = str.ReadToEnd();
            var opt = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            };
            SceneName = Path.GetFileNameWithoutExtension(path);
            File = JsonSerializer.Deserialize<GltfFile>(res, opt);
            DecodeBuffers();
            ParseMeshes();
            BuildSceneGraph(); 
            Buffers.MaterialBuffer.AddRange(File.Materials.Select(x=>new SceneMaterial
            {
                Name = x.Name,
                TextureIndex = x.PbrMetallicRoughness?.BaseColorTexture?.Index ?? -1,
            }));
        }
        public override void WriteTextures(FileStream str)
        {
            str.Write(BitConverter.GetBytes(File.Textures.Count));
            foreach (var texture in File.Textures)
            {
                var imagePtr = File.Images[texture.Source];
                var view = File.BufferViews[imagePtr.BufferView];
                var bufferReader = new BufferReader(this, view);
                usedReaders.Add(bufferReader);
                var imgData = bufferReader.GetAllBytes();

                using var srcStr = new MemoryStream(imgData);
                using var image = Image.FromStream(srcStr);
                using var bmp = new Bitmap(image);
                var size = bmp.Width * bmp.Height * sizeof(int);
                str.Write(BitConverter.GetBytes(bmp.Width));
                str.Write(BitConverter.GetBytes(bmp.Height));
                str.Write(BitConverter.GetBytes(size));

                var dst = new byte[size];
                var i = 0;
                for (var y = 0; y < bmp.Height; y++)
                {
                    for (var x = 0; x < bmp.Width; x++)
                    {
                        var col = bmp.GetPixel(x, y);
                        dst[i] = col.R;
                        dst[i + 1] = col.G;
                        dst[i + 2] = col.B;
                        dst[i + 3] = col.A;
                        i += 4;
                    }
                }
                str.Write(dst);
            }
        }
    }
}