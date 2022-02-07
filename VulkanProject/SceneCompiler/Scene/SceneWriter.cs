using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Drawing.Imaging;
using System.Drawing;
using SceneCompiler.Scene.SceneTypes;

namespace SceneCompiler.Scene
{
    public class SceneWriter : IDisposable
    {
        private FileStream str;

        public void WriteBuffers(string dst, ASceneCompiler compiler)
        {
            if (File.Exists(dst))
                File.Delete(dst);
            str = File.Create(dst);

            WriteVertices(compiler.Buffers.VertexBuffer);
            WriteIndices(compiler.Buffers.IndexBuffer);
            WriteSceneNodes(compiler.Buffers.Nodes, compiler.Buffers.RootNode);
            WriteMaterials(compiler.Buffers.MaterialBuffer);
            compiler.WriteTextures(str);
            str.Dispose();
            str = null;
        }

        public void WriteVertices(List<Vertex> vertexBuffer)
        {
            var size = 48; // see C project Vertex

            var index = 0;
            while (index < vertexBuffer.Count)
            {
                var pos = -4;
                int batchSize = (int)Math.Min(vertexBuffer.Count - index, 1024);
                int batchByteSize = batchSize * size;
                if (index == 0)
                {
                    batchByteSize += 4;
                }
                var vertices = new byte[batchByteSize];
                if (index == 0)
                {
                    BitConverter.GetBytes((uint)vertexBuffer.Count).CopyTo(vertices.AsSpan(pos += 4));
                }

                for (var i = index; i < index + batchSize; i++)
                {
                    pos = vertexBuffer[i].WriteToByteArray(vertices, pos);
                }
                str.Write(vertices);
                index += batchSize;
            }
            if (index != vertexBuffer.Count)
                throw new Exception("Index after write is not vertex.count");
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
        public void WriteSceneNodes(List<SceneNode> nodes, int rootNodeIndex)
        {
            var indices = new List<int>(nodes.Sum(x=>x.NumChildren));
            // write sceneNodes
            {
                var size = 208; // see C project SceneNode
                var index = 0;

                while (index < nodes.Count)
                {
                    var pos = -4;
                    int batchSize = (int) Math.Min(nodes.Count - index, 1024);
                    int batchByteSize = batchSize * size;
                    if(index == 0)
                    {
                        batchByteSize += 4 + 4;
                    }
                    var nodeBuffer = new byte[batchByteSize];
                    if(index == 0)
                    {
                        BitConverter.GetBytes((uint)nodes.Count).CopyTo(nodeBuffer.AsSpan(pos += 4));
                        BitConverter.GetBytes((uint)rootNodeIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    }

                    for (var i = index;i<index+batchSize;i++)
                    {
                        nodes[i].ChildrenIndex = nodes[i].NumChildren > 0 ? indices.Count : -1;
                        pos = nodes[i].WriteToByteArray(nodeBuffer, pos);
                        indices.AddRange(nodes[i].Children.Select(x => x.Index));
                    }
                    str.Write(nodeBuffer);
                    index += batchSize;
                }
                if (index != nodes.Count)
                    throw new Exception("Index after write is not nodes.count");
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
        
        public void Dispose()
        {
            str?.Dispose();
        }
    }
}