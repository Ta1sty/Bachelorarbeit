using SceneCompiler.Scene.SceneTypes;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace SceneCompiler.Scene
{
    class LodCreator
    {
        private readonly SceneBuffers _buffers;
        public LodCreator(SceneBuffers buffers)
        {
            _buffers = buffers;
        }
        public void CreateLevelsOfDetail(SceneNode node, int amount, int factor) // returns the number of nodes to add to the scene Buffer
        {
            if (!node.ForceEven)
                throw new Exception("can only create LOD selector for even nodes (a TLAS)"); // since InstanceHitCompute is exectued for even nodes

            var selector = new SceneNode
            {
                ForceEven = true,
                IsLodSelector = true,
                Name = "LOD-Sel " + node.Name,
            };
            _buffers.Add(selector);

            var lods = new SceneNode[amount];
            foreach (var parent in node.Parents)
            {
                _buffers.SetChildren(parent, parent.Children.Where(x => x != node));
                _buffers.AddChild(parent, selector);
                _buffers.AddParent(selector, parent);
            }
            _buffers.ClearParents(node);
            _buffers.AddParent(node, selector);
            _buffers.AddChild(selector, node);
            var name = node.Name;
            node.Name = "LOD 0 " + name;
            lods[0] = node;
            // create
            for (var i = 1; i < amount; i++)
            {
                var lod = new SceneNode
                {
                    Name = "LOD " + i + " " + name,
                    ForceEven = true,
                    IndexBufferIndex = lods[i-1].IndexBufferIndex,
                    IsInstanceList = lods[i-1].IsInstanceList,
                    NumTriangles = lods[i-1].NumTriangles,
                    ObjectToWorld = lods[i-1].ObjectToWorld,
                };
                CreateLod(lod, factor);
                _buffers.Add(lod);
                _buffers.AddParent(lod,selector);
                _buffers.AddChild(selector, lod);
                _buffers.SetChildren(lod, node.Children);
                foreach (var child in node.Children)
                {
                    _buffers.AddParent(child, lod);
                }

                lods[i] = lod;

                if (lod.NumTriangles < CompilerConfiguration.Configuration.LodConfiguration.LodTriangleThreshold)
                    break;
            }
        }

        public void CreateLod(SceneNode node, int factor)
        {
            var input = GeometryLoader.CreateObj(_buffers, node);
            var output = Path.Combine(Path.GetDirectoryName(input)!, node.Name + ".ply");
            var targetNum = (node.NumTriangles / factor).ToString();
            var args = new[]{ input, output, targetNum};
            Console.WriteLine("Create " + node.Name + " T:" + targetNum);
            var proc = Process.Start("tridecimator.exe", args.Concat(CompilerConfiguration.Configuration.LodConfiguration.TriDecimatorArguments));

            proc.WaitForExit();
            var ply = GeometryLoader.LoadPly(output);
            var start = _buffers.VertexBuffer.Count;
            var offset = _buffers.IndexBuffer.Count;
            node.IndexBufferIndex = offset;
            node.NumTriangles = ply.Indices.Length / 3;
            var vertices = ply.VertexMaterials
                .Select((mat, i) => new Vertex(i, ply.VertexFloats, mat, start))
                .ToArray();

            var dict = new Dictionary<Vertex, int>();

            for (var i = 0; i < ply.Indices.Length; i++)
            {
                var vertex = vertices[ply.Indices[i]];
                dict.TryAdd(vertex, dict.Keys.Count);
                ply.Indices[i] = dict.GetValueOrDefault(vertex);
            }

            var add = new Vertex[dict.Keys.Count];
            foreach (var (vertex,index) in dict)
            {
                add[index] = vertex;
            }

            _buffers.VertexBuffer.AddRange(add);

            _buffers.IndexBuffer.AddRange(ply.Indices.Select(x => (uint)(x + start)));
        }
    }
}