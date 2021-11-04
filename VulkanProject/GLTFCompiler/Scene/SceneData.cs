using System;
using System.Collections.Generic;
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
                SceneNode scNode = new SceneNode();
                if (node.Mesh == -1) // this node does not point to any geometry
                {
                    scNode.Children = node.Children;
                    scNode.NumChildren = scNode.Children?.Length ?? 0;
                }
                else
                {
                    scNode.Children = node.Children;
                    scNode.NumChildren = scNode.Children?.Length ?? 0;
                    scNode.IndexBufferIndex = Meshes[node.Mesh].IndexStart;
                    scNode.NumTriangles = Meshes[node.Mesh].Length / 3;
                }
                Nodes.Add(scNode);
            }
            Nodes.Add(new SceneNode
            {
                Children = file.Scenes[0].Nodes.ToArray(),
                NumChildren = file.Scenes[0].Nodes.Count
            });
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