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
        private static readonly LodConfiguration Configuration = CompilerConfiguration.Configuration.LodConfiguration;
        private readonly Random _random = new(23062000);
        private readonly SceneBuffers _buffers;
        public LodCreator(SceneBuffers buffers)
        {
            _buffers = buffers;
        }
        public void CreateTriangleLod(SceneNode node, int amount, int factor) // returns the number of nodes to add to the scene Buffer
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

            if (_buffers.Root == node)
                _buffers.Root = selector;

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
            var prev = GeometryLoader.CreatePly(_buffers, node);
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
                prev = CreateLod(lod, factor, prev);
                _buffers.Add(lod);
                _buffers.AddParent(lod,selector);
                _buffers.AddChild(selector, lod);
                _buffers.SetChildren(lod, node.Children);
                foreach (var child in node.Children)
                {
                    _buffers.AddParent(child, lod);
                }

                lods[i] = lod;

                if (lod.NumTriangles < Configuration.LodTriangleThreshold)
                    break;
            }
        }

        public string CreateLod(SceneNode node, int factor, string input)
        {
            var output = Path.Combine(Path.GetDirectoryName(input)!, node.Name + ".ply");
            var targetNum = (node.NumTriangles / factor).ToString();
            var args = new[]{ input, output, targetNum};
            Console.WriteLine("Create " + node.Name + " T:" + targetNum);
            var proc = Process.Start("tridecimator.exe", args.Concat(Configuration.TriDecimatorArguments));

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
            return output;
        }

        public void CreateInstanceLod(SceneNode node, int amount, int factor)
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

            if (_buffers.Root == node)
                _buffers.Root = selector;

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
                    IndexBufferIndex = lods[i - 1].IndexBufferIndex,
                    IsInstanceList = lods[i - 1].IsInstanceList,
                    NumTriangles = lods[i - 1].NumTriangles,
                    ObjectToWorld = lods[i - 1].ObjectToWorld,
                };
                _buffers.Add(lod);
                _buffers.AddParent(lod, selector);
                _buffers.AddChild(selector, lod);
                var children = lods[i - 1].Children.ToList();
                _buffers.SetChildren(lod, GetRandomChildren(children, children.Count/ factor));
                foreach (var child in lod.Children)
                {
                    _buffers.AddParent(child, lod);
                }

                lods[i] = lod;

                if (lod.NumChildren < CompilerConfiguration.Configuration.LodConfiguration.LodInstanceThreshold)
                    break;
            }
        }

        public IEnumerable<SceneNode> GetRandomChildren(List<SceneNode> children, int targetNum)
        {
            var max = children.Count;

            var set = new HashSet<SceneNode>(targetNum);

            while (set.Count<targetNum)
            {
                var index = _random.Next(0, max);
                var node = children[index];
                set.Add(node);
            }

            return set;
        }
    }
}