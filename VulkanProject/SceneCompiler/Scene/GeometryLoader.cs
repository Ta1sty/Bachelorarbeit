using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using Scene;

namespace SceneCompiler.Scene
{
    public class GeometryLoader
    {
        public static string CreateObj(SceneBuffers buffers, SceneNode node)
        {
            var indexOffset = buffers.VertexBuffer[(int)buffers.IndexBuffer[node.IndexBufferIndex]].IndexOffset;

            var vertexList = buffers.IndexBuffer.GetRange(node.IndexBufferIndex, node.NumTriangles * 3)
                .Select(x => buffers.VertexBuffer[(int)x]);

            var vertices = new HashSet<Vertex>(vertexList);

            var materials = vertices.Select(x => x.MaterialIndex).Distinct().ToList();

            using var stream = new StreamWriter(node.Name+".obj");
            stream.WriteLine("o object");
            foreach (var vertex in vertices)
            {
                var pos = vertex.Position();
                stream.WriteLine("v " + pos.X.ToString(CultureInfo.InvariantCulture) + " " 
                                 + pos.Y.ToString(CultureInfo.InvariantCulture) + " " 
                                 + pos.Z.ToString(CultureInfo.InvariantCulture) + " " 
                                 + vertex.MaterialIndex);
            }

            foreach (var vertex in vertices)
            {
                var n = vertex.Normal();
                stream.WriteLine("vn " + n.X.ToString(CultureInfo.InvariantCulture) + " " + n.Y.ToString(CultureInfo.InvariantCulture) + " " + n.Z.ToString(CultureInfo.InvariantCulture));
            }

            foreach (var vertex in vertices)
            {
                var tex = vertex.Tex();
                stream.WriteLine("vt " + tex.X.ToString(CultureInfo.InvariantCulture) + " " + tex.Y.ToString(CultureInfo.InvariantCulture));
            }

            for (var i = 0; i < node.NumTriangles; i++)
            {
                var v0 = buffers.IndexBuffer[node.IndexBufferIndex + i * 3 + 0] - indexOffset + 1;
                var v1 = buffers.IndexBuffer[node.IndexBufferIndex + i * 3 + 1] - indexOffset + 1;
                var v2 = buffers.IndexBuffer[node.IndexBufferIndex + i * 3 + 2] - indexOffset + 1;
                stream.WriteLine($"f {v0}/{v0}/{v0} {v1}/{v1}/{v1} {v2}/{v2}/{v2}");
            }

            return Path.GetFullPath(node.Name + ".obj");
        }

        public static string CreatePly(SceneBuffers buffers, SceneNode node)
        {
            var indexOffset = buffers.VertexBuffer[(int)buffers.IndexBuffer[node.IndexBufferIndex]].IndexOffset;

            var vertexList = buffers.IndexBuffer.GetRange(node.IndexBufferIndex, node.NumTriangles * 3)
                .Select(x => buffers.VertexBuffer[(int)x]);

            var vertices = new HashSet<Vertex>(vertexList);

            var materials = vertices.Select(x => x.MaterialIndex).Distinct().ToList();
            // write header
            using (var textStream = new StreamWriter(node.Name + ".ply")){
                textStream.WriteLine("ply");
                textStream.WriteLine("format binary_little_endian 1.0");
                textStream.WriteLine("comment VCGLIB generated");
                textStream.WriteLine($"element vertex {vertices.Count}");
                textStream.WriteLine("property float x");
                textStream.WriteLine("property float y");
                textStream.WriteLine("property float z");
                textStream.WriteLine("property float nx");
                textStream.WriteLine("property float ny");
                textStream.WriteLine("property float nz");
                textStream.WriteLine("property uchar red");
                textStream.WriteLine("property uchar green");
                textStream.WriteLine("property uchar blue");
                textStream.WriteLine("property uchar alpha");
                textStream.WriteLine("property float texture_u");
                textStream.WriteLine("property float texture_v");
                textStream.WriteLine($"element face {node.NumTriangles}");
                textStream.WriteLine("property list uchar int vertex_indices");
                textStream.WriteLine("end_header");
            }
            // write contents
            using (var byteStream = File.Open(node.Name + ".ply", FileMode.Append))
            {
                foreach (var vertex in vertices)
                {
                    var pos = vertex.Position();
                    var norm = vertex.Normal();
                    var tex = vertex.Tex();
                    var mat = vertex.MaterialIndex;
                    byteStream.Write(BitConverter.GetBytes(pos.X));
                    byteStream.Write(BitConverter.GetBytes(pos.Y));
                    byteStream.Write(BitConverter.GetBytes(pos.Z));
                    byteStream.Write(BitConverter.GetBytes(norm.X));
                    byteStream.Write(BitConverter.GetBytes(norm.Y));
                    byteStream.Write(BitConverter.GetBytes(norm.Z));
                    byteStream.Write(BitConverter.GetBytes(mat));
                    byteStream.Write(BitConverter.GetBytes(tex.X));
                    byteStream.Write(BitConverter.GetBytes(tex.Y));
                }

                for (var i = 0; i < node.NumTriangles; i++)
                {
                    var v0 = (int)buffers.IndexBuffer[node.IndexBufferIndex + i * 3 + 0] - indexOffset;
                    var v1 = (int)buffers.IndexBuffer[node.IndexBufferIndex + i * 3 + 1] - indexOffset;
                    var v2 = (int)buffers.IndexBuffer[node.IndexBufferIndex + i * 3 + 2] - indexOffset;
                    byteStream.WriteByte(3);
                    byteStream.Write(BitConverter.GetBytes(v0));
                    byteStream.Write(BitConverter.GetBytes(v1));
                    byteStream.Write(BitConverter.GetBytes(v2));
                }
            }
            return Path.GetFullPath(node.Name + ".ply");
        }

        public class PlyFile
        {
            public float[] VertexFloats;
            public int[] VertexMaterials;
            public int[] Indices;
        }

        public static PlyFile LoadPly(string path) // we dont care about the layout of the ply, we just do our thing
        {
            var ply = new PlyFile();
            var info = new FileInfo(path);
            using var str = new StreamReader(path);
            if (str.ReadLine() != "ply" ||
                str.ReadLine() != "format binary_little_endian 1.0" ||
                str.ReadLine() != "comment VCGLIB generated")
                throw new Exception("wrong file format");

            var line = str.ReadLine()!;
            var numVer = int.Parse(line[line.LastIndexOf(" ", StringComparison.Ordinal)..]);
            if (str.ReadLine() != "property float x" ||
                str.ReadLine() != "property float y" ||
                str.ReadLine() != "property float z" ||
                str.ReadLine() != "property float nx" ||
                str.ReadLine() != "property float ny" ||
                str.ReadLine() != "property float nz" ||
                str.ReadLine() != "property uchar red" ||
                str.ReadLine() != "property uchar green" ||
                str.ReadLine() != "property uchar blue" ||
                str.ReadLine() != "property uchar alpha" ||
                str.ReadLine() != "property float texture_u" ||
                str.ReadLine() != "property float texture_v")
                throw new Exception("wrong file format");

            line = str.ReadLine()!;
            var numFace = int.Parse(line[line.LastIndexOf(" ", StringComparison.Ordinal)..]);

            if (str.ReadLine() != "property list uchar int vertex_indices" ||
                str.ReadLine() != "end_header")
                throw new Exception("wrong file format");

            var buffer = new byte[info.Length];
            var fltDst = new float[numVer * 8];
            var intDst = new int[numVer]; // 

            var bStr = str.BaseStream;
            bStr.Position = 0;
            var count = 0;
            while (count < 19)
            {
                if(bStr.ReadByte() == 0x0a) // skip 19 new line characters
                    count++;
            }
            bStr.Read(buffer);
            var pos = -4;
            for (var i = 0; i < numVer; i++)
            {
                fltDst[i * 8 + 0] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));
                fltDst[i * 8 + 1] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));
                fltDst[i * 8 + 2] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));
                fltDst[i * 8 + 3] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));
                fltDst[i * 8 + 4] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));
                fltDst[i * 8 + 5] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));

                intDst[i] = BitConverter.ToInt32(buffer.AsSpan(pos += 4, 4));

                fltDst[i * 8 + 6] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));
                fltDst[i * 8 + 7] = BitConverter.ToSingle(buffer.AsSpan(pos += 4, 4));
            }

            ply.VertexFloats = fltDst;
            ply.VertexMaterials = intDst;

            var indices = new int[numFace * 3];
            pos += 4;
            for (var i = 0; i < numFace; i++)
            {
                var num = buffer[pos];
                if (num != 3)
                    throw new Exception("only 3 vertices per triangle are allowed");
                pos += 1;
                indices[i * 3 + 0] = BitConverter.ToInt32(buffer.AsSpan(pos, 4));
                pos += 4;
                indices[i * 3 + 1] = BitConverter.ToInt32(buffer.AsSpan(pos, 4));
                pos += 4;
                indices[i * 3 + 2] = BitConverter.ToInt32(buffer.AsSpan(pos, 4));
                pos += 4;
            }

            if (indices.Any(x => x < 0))
                throw new Exception("an index exists that points to a value lower than 0");
            if (indices.Any(x => x >= numVer))
                throw new Exception("an index exists that points to a value larger than numVer");
            ply.Indices = indices;
            return ply;
        }
    }
}