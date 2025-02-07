﻿using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using Scene;
using SceneCompiler.MoanaConversion;

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
            Console.WriteLine("Writing Vertices: " + compiler.Buffers.VertexBuffer.Count);
            WriteVertices(compiler.Buffers.VertexBuffer);
            Console.WriteLine("Writing Indices: " + compiler.Buffers.IndexBuffer.Count);
            WriteIndices(compiler.Buffers.IndexBuffer);
            Console.WriteLine("Writing Nodes: " + compiler.Buffers.Nodes.Count());
            WriteSceneNodes(compiler.Buffers);
            Console.WriteLine("Writing Materials: " + compiler.Buffers.MaterialBuffer.Count);
            WriteMaterials(compiler.Buffers.MaterialBuffer);
            compiler.WriteTextures(str);
            str.Dispose();
            str = null;
        }

        public void WriteVertices(List<Vertex> vertexBuffer)
        {
            var size = Vertex.Size; // see C project Vertex

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
        public void WriteSceneNodes(SceneBuffers buffers)
        {
            var count = buffers.Nodes.Count();
            var indices = new List<int>(buffers.Nodes.Sum(x=>x.NumChildren));
            var transforms = new List<Matrix4x4>();
            transforms.Add(Matrix4x4.Identity);
            // write sceneNodes
            {
                var size = SceneNode.Size; // see C project SceneNode
                var index = 0;

                while (index < count)
                {
                    var pos = -4;
                    var batchSize = Math.Min(count - index, 1024);
                    var batchByteSize = batchSize * size;
                    if(index == 0)
                    {
                        batchByteSize += 4 + 4;
                    }
                    var nodeBuffer = new byte[batchByteSize];
                    if(index == 0)
                    {
                        BitConverter.GetBytes((uint)count).CopyTo(nodeBuffer.AsSpan(pos += 4));
                        BitConverter.GetBytes((uint)buffers.RootIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
                    }

                    for (var i = index;i<index+batchSize;i++)
                    {
                        var node = buffers.NodeByIndex(i);
                        node.ChildrenIndex = node.NumChildren > 0 ? indices.Count : -1;
                        if (node.ObjectToWorld == Matrix4x4.Identity || SceneNode.MatrixAlmostZero(node.ObjectToWorld - Matrix4x4.Identity))
                            node.TransformIndex = 0;
                        else
                        {
                            node.TransformIndex = (uint)transforms.Count;
                            transforms.Add(node.ObjectToWorld);
                        }

                        pos = node.WriteToByteArray(nodeBuffer, pos);
                        indices.AddRange(node.Children.Select(x => x.Index()));
                    }
                    str.Write(nodeBuffer);
                    index += batchSize;
                }
                if (index != count)
                    throw new Exception("Index after write is not nodes.count");
            }

            // write transforms

            {
                var size = 4 * 4 * 3;
                var buf = new byte[4 + transforms.Count * size];
                var pos = -4;
                BitConverter.GetBytes((uint)transforms.Count).CopyTo(buf.AsSpan(pos += 4));
                foreach (var transform in transforms)
                {
                    BitConverter.GetBytes(transform.M11).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M21).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M31).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M41).CopyTo(buf.AsSpan(pos += 4));

                    BitConverter.GetBytes(transform.M12).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M22).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M32).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M42).CopyTo(buf.AsSpan(pos += 4));

                    BitConverter.GetBytes(transform.M13).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M23).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M33).CopyTo(buf.AsSpan(pos += 4));
                    BitConverter.GetBytes(transform.M43).CopyTo(buf.AsSpan(pos += 4));
                }
                str.Write(buf);

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

        public void WriteMaterials(List<SceneMaterial> materials)
        {
            var materialBuffer = new byte[SceneMaterial.Size * materials.Count + 4]; // first 4 bytes is uint32 for numMaterials
            var pos = -4;
            BitConverter.GetBytes((uint)materials.Count).CopyTo(materialBuffer.AsSpan(pos += 4));
            foreach (var material in materials)
            {
                BitConverter.GetBytes(material.DefaultColor.X).CopyTo(materialBuffer.AsSpan(pos += 4)); // color r
                BitConverter.GetBytes(material.DefaultColor.Y).CopyTo(materialBuffer.AsSpan(pos += 4)); // color g
                BitConverter.GetBytes(material.DefaultColor.Z).CopyTo(materialBuffer.AsSpan(pos += 4)); // color b
                BitConverter.GetBytes(material.DefaultColor.W).CopyTo(materialBuffer.AsSpan(pos += 4)); // color a
                BitConverter.GetBytes(material.Ambient).CopyTo(materialBuffer.AsSpan(pos += 4)); // ka
                BitConverter.GetBytes(material.Diffuse).CopyTo(materialBuffer.AsSpan(pos += 4)); // kd
                BitConverter.GetBytes(material.Specular).CopyTo(materialBuffer.AsSpan(pos += 4)); // ks
                BitConverter.GetBytes(material.Reflection).CopyTo(materialBuffer.AsSpan(pos += 4)); // kr
                BitConverter.GetBytes(material.Transmission).CopyTo(materialBuffer.AsSpan(pos += 4)); // kt
                BitConverter.GetBytes(material.PhongExponent).CopyTo(materialBuffer.AsSpan(pos += 4)); // phong
                BitConverter.GetBytes(material.TextureIndex).CopyTo(materialBuffer.AsSpan(pos += 4)); // textureIndex
                BitConverter.GetBytes(0).CopyTo(materialBuffer.AsSpan(pos += 4)); // pad1
            }
            str.Write(materialBuffer);
        }
        
        public void Dispose()
        {
            str?.Dispose();
        }
    }
}