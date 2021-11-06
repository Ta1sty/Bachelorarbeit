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
                int start = IndexBuffer.Count;
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
                    for (var i = 0; i < numElements; i++)
                    {
                        var ver = new Vertex
                        {
                            Position = posReader.GetVec3(),
                            Normal = norReader.GetVec3(),
                            TexCoords = texReader.GetVec2()
                        };
                        VertexBuffer.Add(ver);
                    }
                    var faceAccessor = file.Accessors[arr.Indices];
                    var faceReader = new BufferReader(this, faceAccessor);
                    usedBuffers.Add(faceReader);
                    if (arr.Indices == -1) // vertex array
                    {
                        for (var i = 0; i < faceAccessor.Count; i++)
                        {
                            uint v1 = faceReader.GetUshort();
                            uint v2 = faceReader.GetUshort();
                            uint v3 = faceReader.GetUshort();
                            IndexBuffer.Add(v1);
                            IndexBuffer.Add(v2);
                            IndexBuffer.Add(v3);
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
                                IndexBuffer.Add(v1);
                                IndexBuffer.Add(v2);
                                IndexBuffer.Add(v3);
                            }
                        }
                    }
                }

                var end = IndexBuffer.Count;
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


                    scNode.IndexBufferIndex = Meshes[node.Mesh].IndexStart;
                    scNode.NumTriangles = Meshes[node.Mesh].Length / 3;
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

                scNode.Transform = translation * rotation * scale; 
                scNode.Source = node;
                Nodes.Add(scNode);
            }
            var end = new SceneNode
            {
                Children = file.Scenes[0].Nodes.Select(x=>Nodes[x]).ToList(),
                NumChildren = file.Scenes[0].Nodes.Count
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
                .Where(x => !(x.Source.Translation?[0] == 0 && x.Source.Translation?[1] == 0 && x.Source.Translation?[2] == 0)).ToList();
            var index = 0;
            foreach (var node in Nodes)
            {
                node.Index = index;
                index++;
            }
        }

        public void WriteBytes()
        {
            if(File.Exists("dump.bin"))
                File.Delete("dump.bin");
            using var str = File.Create("dump.bin");
            // first write vertices
            {
                var vertices = new byte[32 * VertexBuffer.Count +4]; // first 4 bytes is uint32 numVertices
                var pos = -4;
                Reverse(BitConverter.GetBytes(VertexBuffer.Count)).CopyTo(vertices.AsSpan(pos += 4));
                foreach (var vertex in VertexBuffer)
                {
                    Reverse(BitConverter.GetBytes(vertex.Position[0])).CopyTo(vertices.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(vertex.Position[1])).CopyTo(vertices.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(vertex.Position[2])).CopyTo(vertices.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(vertex.Normal[0])).CopyTo(vertices.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(vertex.Normal[1])).CopyTo(vertices.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(vertex.Normal[2])).CopyTo(vertices.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(vertex.TexCoords[0])).CopyTo(vertices.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(vertex.TexCoords[1])).CopyTo(vertices.AsSpan(pos += 4));
                }
                str.Write(vertices);
            }
            // write triangle buffer
            {
                var triangles = new byte[4 * IndexBuffer.Count + 4]; // first 4 bytes is uint32 for numIndices
                var pos = -4;
                Reverse(BitConverter.GetBytes((uint)IndexBuffer.Count)).CopyTo(triangles.AsSpan(pos += 4));
                foreach (var index in IndexBuffer)
                {
                    Reverse(BitConverter.GetBytes(index)).CopyTo(triangles.AsSpan(pos += 4));
                }
                str.Write(triangles);
            }
            var indices = new List<int>();
            // write sceneNodes
            {
                var size = 20 + 64; // data + transform
                var nodes = new byte[4 + Nodes.Count * size];
                var pos = -4;
                Reverse(BitConverter.GetBytes((uint)IndexBuffer.Count)).CopyTo(nodes.AsSpan(pos += 4));
                foreach (var node in Nodes)
                {
                    Reverse(BitConverter.GetBytes(node.IndexBufferIndex)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.NumTriangles)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.NumChildren)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.NumChildren > 0 ? indices.Count : -1)).CopyTo(nodes.AsSpan(pos += 4)); // children index
                    Reverse(BitConverter.GetBytes(node.Index)).CopyTo(nodes.AsSpan(pos += 4));
                    indices.AddRange(node.Children.Select(x=>x.Index));

                    Reverse(BitConverter.GetBytes(node.Transform.M11)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M12)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M13)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M14)).CopyTo(nodes.AsSpan(pos += 4));

                    Reverse(BitConverter.GetBytes(node.Transform.M21)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M22)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M23)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M24)).CopyTo(nodes.AsSpan(pos += 4));

                    Reverse(BitConverter.GetBytes(node.Transform.M31)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M32)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M33)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M34)).CopyTo(nodes.AsSpan(pos += 4));

                    Reverse(BitConverter.GetBytes(node.Transform.M41)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M42)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M43)).CopyTo(nodes.AsSpan(pos += 4));
                    Reverse(BitConverter.GetBytes(node.Transform.M44)).CopyTo(nodes.AsSpan(pos += 4));
                }
                str.Write(nodes);
            }
            // write index array
            {
                var pos = -4;
                var buf = new byte[4 + indices.Count * 4];
                Reverse(BitConverter.GetBytes((uint)indices.Count)).CopyTo(buf.AsSpan(pos += 4));
                foreach (var index in indices)
                {
                    Reverse(BitConverter.GetBytes(index)).CopyTo(buf.AsSpan(pos += 4));
                }
                str.Write(buf);
            }
        }

        public byte[] Reverse(byte[] arr)
        {
            byte v0 = arr[0];
            byte v1 = arr[1];
            arr[0] = arr[3];
            arr[1] = arr[2];
            arr[2] = v1;
            arr[3] = v0;
            return arr;
        }
        public class BufferReader
        {
            private readonly byte[] _buffer;
            private int _currentPos;
            public BufferReader(SceneData scene, Accessor acc)
            {
                BufferView view = scene.file.BufferViews[acc.BufferView];
                _currentPos = view.ByteOffset;
                _buffer = scene.Buffers[view.Buffer];
            }
            public float[] GetVec3()
            {
                float x = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
                _currentPos += 4;
                float y = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
                _currentPos += 4;
                float z = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
                _currentPos += 4;
                return new[]{x,y,z};
            }
            public float[] GetVec2()
            {
                float x = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
                _currentPos += 4;
                float y = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
                _currentPos += 4;
                return new[] { x, y};
            } 
            public float[] GetFloat()
            {
                float x = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
                _currentPos += 4;
                return new[] { x };
            }
            public int GetInt()
            {
                int x = BitConverter.ToInt32(_buffer.AsSpan(_currentPos, 4));
                _currentPos += 4;
                return x;
            }

            public ushort GetUshort()
            {
                ushort x = BitConverter.ToUInt16(_buffer.AsSpan(_currentPos, 2));
                _currentPos += 2;
                return x;
            }
        }
    }
}