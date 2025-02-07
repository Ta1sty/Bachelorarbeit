﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using Scene;

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
                parent.Children = parent.Children.Where(x => x != node);
                parent.AddChild(selector);
                selector.AddParent(parent);
            }
            node.ClearParents();
            node.AddParent(selector);
            selector.AddChild(node);
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
                lod.AddParent(selector);
                selector.AddChild(lod);
                lod.Children = node.Children;
                foreach (var child in node.Children)
                {
                    child.AddParent(lod);
                }

                lods[i] = lod;

                if (lod.NumTriangles < Configuration.LodTriangleStopCount)
                    break;
            }

            foreach (var file in Directory.GetFiles(".").Where(x=>x.EndsWith(".ply")))
            {
                File.Delete(file);
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
            if (Configuration.CutExcess)
                amount = 2;
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
                parent.Children = parent.Children.Where(x => x != node);
                parent.AddChild(selector);
                selector.AddParent(parent);
            }
            node.ClearParents();
            node.AddParent(selector);
            selector.AddChild(node);
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
                lod.AddParent(selector);
                selector.AddChild(lod);
                var children = lods[i - 1].Children.ToList();
                lod.Children = GetRandomChildren(children, Configuration.CutExcess ? // if the excess is cut there is only one lod
                    Configuration.LodInstanceThreshold :
                    children.Count/ factor);
                foreach (var child in lod.Children)
                {
                    child.AddParent(lod);
                }

                lods[i] = lod;

                if (lod.NumChildren < Configuration.LodInstanceStopCount)
                    break;

                if(Configuration.CutExcess)
                    break;
            }

            if (!Configuration.CutExcess) return;

            // if we cut the excess this is not LOD, but just removal of excess
            foreach (var parent in selector.Parents)
            {
                parent.Children = parent.Children.Where(x => x != selector);
                parent.AddChild(lods[1]);
            }
            selector.ClearChildren();
            _buffers.RewriteAllParents();
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