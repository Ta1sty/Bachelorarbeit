using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text.Json;
using SceneCompiler.GLTFConversion.GltfFileTypes;
using SceneCompiler.Scene;
using SceneCompiler.Scene.SceneTypes;

namespace SceneCompiler.GLTFConversion.Compilation
{
    public class GLTFCompiler : ASceneCompiler
    {
        internal GltfFile File { get; set; }

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
                uint start = (uint) indexBuffer.Count;
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

                    uint vertexStart = (uint)vertexBuffer.Count;

                    for (var i = 0; i < numElements; i++)
                    {
                        var ver = new Vertex
                        {
                            Position = posReader.GetVec3(),
                            Normal = norReader.GetVec3(),
                            TexCoords = texReader.GetVec2(),
                            MaterialIndex = arr.Material
                        };
                        vertexBuffer.Add(ver);
                    }
                    var faceAccessor = File.Accessors[arr.Indices];
                    var faceReader = new BufferReader(this, faceAccessor);

                    usedReaders.Add(faceReader);
                    if (true) // vertex array
                    {
                        for (var i = 0; i < faceAccessor.Count; i+=3)
                        {
                            uint v1 = faceReader.GetUshort();
                            uint v2 = faceReader.GetUshort();
                            uint v3 = faceReader.GetUshort();
                            if (v1 == v2 || v2 == v3 || v3 == v1) continue; // just a line
                            indexBuffer.Add(v1 + vertexStart);
                            indexBuffer.Add(v2 + vertexStart);
                            indexBuffer.Add(v3 + vertexStart);
                        }
                    }
                }

                uint end = (uint) indexBuffer.Count;
                var length = end - start;
                Meshes.Add(new MeshData
                {
                    IndexStart = start,
                    Length = length
                });
            }
        }

        private void BuildSceneGraph()
        {
            var nodes = Buffers.Nodes;
            foreach (var node in File.Nodes)
            {
                var scNode = new SceneNode();
                if (node.Mesh == -1) // this node does not point to any geometry
                {
                    scNode.Children = node.Children.Select(x => nodes[x]).ToList();
                    scNode.NumChildren = scNode.Children.Count;
                }
                else
                {
                    scNode.Children = node.Children.Select(x => nodes[x]).ToList();
                    scNode.NumChildren = scNode.Children.Count;


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
                    Matrix4x4.Invert(rotation, out invRotation);
                }

                if (node.Scale != null)
                {
                    scale = Matrix4x4.CreateScale(node.Scale[0], node.Scale[1], node.Scale[2]);
                    Matrix4x4.Invert(scale, out invScale);
                }

                if (node.Translation != null)
                {
                    translation = Matrix4x4.CreateTranslation(node.Translation[0], node.Translation[1], node.Translation[2]);
                    invTranslation = Matrix4x4.CreateTranslation(-node.Translation[0], -node.Translation[1], -node.Translation[2]);
                }

                scNode.ObjectToWorld =  scale * rotation * translation;
                scNode.WorldToObject = invTranslation * invRotation * invScale;
                // TEST
                /*
                var v = new Vector4(4, -3, 2, 1);
                var res1 = Vector4.Transform(v, scNode.ObjectToWorld * scNode.WorldToObject);
                var res2 = Vector4.Transform(v, scNode.WorldToObject * scNode.ObjectToWorld);
                */

                scNode.Source = node;
                nodes.Add(scNode);
            }
            var end = new SceneNode
            {
                Children = File.Scenes[0].Nodes.Select(x=>nodes[x]).ToList(),
                NumChildren = File.Scenes[0].Nodes.Count,
                ObjectToWorld = Matrix4x4.Identity,
                WorldToObject = Matrix4x4.Identity,
                Name = "ROOT"
            };

            var arr = nodes.ToArray();
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

            foreach (var node in nodes)
            {
                node.Children = node.Children.Select(x => x.ThisOrBrother()).ToList();
            }

            nodes = nodes.Where(x => x.Brother == null).ToList();
            end.Children = end.Children.Select(x => x.ThisOrBrother()).ToList();
            if (end.Children.Count(x => x.Source.Mesh >= 0) == 1) // scene consists of only one mesh with a scene node
            {

            }
            else // scene consists of more than one, assume instancing
            {
                end.Children = end.Children
                    .Where(x =>
                    {
                        if (x.Source.Mesh < 0) return true;
                        if (x.Children.Count > 0) return true;
                        if (x.ObjectToWorld.M41 != 0) return true;
                        if (x.ObjectToWorld.M42 != 0) return true;
                        if (x.ObjectToWorld.M43 != 0) return true;
                        return false;
                    }).ToList();
            }

            end.NumChildren = end.Children.Count;
            nodes.Add(end);

            for (int i = 0; i < nodes.Count; i++)
            {
                nodes[i].Index = i;
            }

            Buffers.Nodes = nodes;
            Buffers.RootNode = Buffers.Nodes.Count - 1; // because the node for the scene is added last
        }

        public override void CompileScene(string path)
        {
            using var str = new StreamReader(path);
            var res = str.ReadToEnd();
            var opt = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            };
             File = JsonSerializer.Deserialize<GltfFile>(res, opt);
             DecodeBuffers();
             ParseMeshes();
             BuildSceneGraph();
             Buffers.MaterialBuffer = File.Materials;
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