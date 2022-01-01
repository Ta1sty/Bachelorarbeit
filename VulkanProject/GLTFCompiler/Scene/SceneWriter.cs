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
            WriteSceneNodes(compiler.Buffers.Nodes);
            WriteMaterials(compiler.Buffers.MaterialBuffer);
            compiler.WriteTextures(str);
            str.Dispose();
            str = null;
        }

        public void WriteVertices(List<Vertex> vertexBuffer)
        {
            var vertices = new byte[48 * vertexBuffer.Count + 4]; // first 4 bytes is uint32 numVertices
            var pos = -4;
            BitConverter.GetBytes((uint)vertexBuffer.Count).CopyTo(vertices.AsSpan(pos += 4));
            foreach (var vertex in vertexBuffer)
            {
                pos = vertex.WriteToByteArray(vertices, pos);
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
            var indices = new List<int>(nodes.Sum(x=>x.NumChildren));
            // write sceneNodes
            {
                var size = 208; // see C project SceneNode
                var nodeBuffer = new byte[4 + nodes.Count * size];
                var pos = -4;
                BitConverter.GetBytes((uint)nodes.Count).CopyTo(nodeBuffer.AsSpan(pos += 4));
                foreach (var node in nodes)
                {
                    node.ChildrenIndex = node.NumChildren > 0 ? indices.Count : -1;
                    pos = node.WriteToByteArray(nodeBuffer, pos);
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
        
        public void Dispose()
        {
            str?.Dispose();
        }
    }
}