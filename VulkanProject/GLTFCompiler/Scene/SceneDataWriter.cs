using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using GLTFCompiler.GltfFileTypes;
using System.Drawing.Imaging;
using System.Drawing;

namespace GLTFCompiler.Scene
{
    public class SceneDataWriter : IDisposable
    {
        private FileStream str; 
        public SceneDataWriter(string dst)
        {
            if (File.Exists(dst))
                File.Delete(dst);
            str = File.Create(dst);
        }

        public void WriteVertices(List<Vertex> vertexBuffer)
        {
            var vertices = new byte[48 * vertexBuffer.Count + 4]; // first 4 bytes is uint32 numVertices
            var pos = -4;
            BitConverter.GetBytes((uint)vertexBuffer.Count).CopyTo(vertices.AsSpan(pos += 4));
            foreach (var vertex in vertexBuffer)
            {
                BitConverter.GetBytes(vertex.Position[0]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(vertex.Position[1]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(vertex.Position[2]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad1

                BitConverter.GetBytes(vertex.Normal[0]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(vertex.Normal[1]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(vertex.Normal[2]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad2


                BitConverter.GetBytes(vertex.TexCoords[0]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(vertex.TexCoords[1]).CopyTo(vertices.AsSpan(pos += 4));
                BitConverter.GetBytes(vertex.MaterialIndex).CopyTo(vertices.AsSpan(pos += 4));  // material index, TODO
                BitConverter.GetBytes(0).CopyTo(vertices.AsSpan(pos += 4));  // pad3
                if ((pos) % 16 != 0) throw new Exception();
            }
            str.Write(vertices);
        }
        public void WriteIndices(List<uint> indexBuffer)
        {
            var triangles = new byte[4 * indexBuffer.Count + 4]; // first 4 bytes is uint32 for numIndices
            var pos = -4;
            BitConverter.GetBytes((uint)indexBuffer.Count).CopyTo(triangles.AsSpan(pos += 4));
            foreach (var index in indexBuffer)
            {
                BitConverter.GetBytes(index).CopyTo(triangles.AsSpan(pos += 4));
            }
            str.Write(triangles);
        }
        public void WriteSceneNodes(List<SceneNode> nodes)
        {
            var indices = new List<int>();
            // write sceneNodes
            {
                var size = 20 + 64 + 64; // data + transform1 + transform2
                var nodeBuffer = new byte[4 + nodes.Count * size];
                var pos = -4;
                BitConverter.GetBytes((uint)nodes.Count).CopyTo(nodeBuffer.AsSpan(pos += 4));
                foreach (var node in nodes)
                {
                    // OBJECT TO WORLD
                    BitConverter.GetBytes(node.ObjectToWorld.M11).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M21).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M31).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M41).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    BitConverter.GetBytes(node.ObjectToWorld.M12).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M22).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M32).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M42).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    BitConverter.GetBytes(node.ObjectToWorld.M13).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M23).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M33).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M43).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    BitConverter.GetBytes(node.ObjectToWorld.M14).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M24).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M34).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.ObjectToWorld.M44).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    // WORLD TO OBJECT
                    BitConverter.GetBytes(node.WorldToObject.M11).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M21).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M31).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M41).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    BitConverter.GetBytes(node.WorldToObject.M12).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M22).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M32).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M42).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    BitConverter.GetBytes(node.WorldToObject.M13).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M23).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M33).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M43).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    BitConverter.GetBytes(node.WorldToObject.M14).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M24).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M34).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.WorldToObject.M44).CopyTo(nodeBuffer.AsSpan(pos += 4));

                    // THE REST
                    BitConverter.GetBytes(node.IndexBufferIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.NumTriangles).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.NumChildren).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    BitConverter.GetBytes(node.NumChildren > 0 ? indices.Count : -1).CopyTo(nodeBuffer.AsSpan(pos += 4)); // children index
                    BitConverter.GetBytes(node.Index).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    indices.AddRange(node.Children.Select(x => x.Index));
                }
                str.Write(nodeBuffer);
            }
            // write index array
            {
                var pos = -4;
                var buf = new byte[4 + indices.Count * 4];
                BitConverter.GetBytes((uint)indices.Count).CopyTo(buf.AsSpan(pos += 4));
                foreach (var index in indices)
                {
                    BitConverter.GetBytes(index).CopyTo(buf.AsSpan(pos += 4));
                }
                str.Write(buf);
            }
        }

        public void WriteMaterials(List<Material> materials)
        {
            var materialBuffer = new byte[16 * materials.Count + 4]; // first 4 bytes is uint32 for numMaterials
            var pos = -4;
            BitConverter.GetBytes((uint)materials.Count).CopyTo(materialBuffer.AsSpan(pos += 4));
            foreach (var material in materials)
            {
                BitConverter.GetBytes(0.2f).CopyTo(materialBuffer.AsSpan(pos += 4)); // ka
                BitConverter.GetBytes(0.4f).CopyTo(materialBuffer.AsSpan(pos += 4)); // kd
                BitConverter.GetBytes(0.6f).CopyTo(materialBuffer.AsSpan(pos += 4)); // ks
                BitConverter.GetBytes(material.PbrMetallicRoughness?.BaseColorTexture?.Index ?? -1).CopyTo(materialBuffer.AsSpan(pos += 4)); // textureIndex
            }
            str.Write(materialBuffer);
        }

        public void WriteTextures(SceneData data)
        {
            str.Write(BitConverter.GetBytes(data.file.Textures.Count));
            foreach (var texture in data.file.Textures)
            {
                var imagePtr = data.file.Images[texture.Source];
                var view = data.file.BufferViews[imagePtr.BufferView];
                var bufferReader = new BufferReader(data, view);
                data.usedBuffers.Add(bufferReader);
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
                for (var y = 0; y < bmp.Height; y++) {
                    for (var x = 0; x < bmp.Width; x++) { 
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

        public void Dispose()
        {
            str?.Dispose();
        }
    }
}