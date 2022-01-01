using System;
using System.Collections.Generic;
using System.Numerics;
using SceneCompiler.Scene.SceneTypes;

namespace SceneCompiler.Scene
{
    public class RayTracePostCompile
    {
        private readonly SceneBuffers _buffers;
        private readonly bool _multiLevel;
        public RayTracePostCompile(SceneBuffers buffers, bool multiLevel)
        {
            _buffers = buffers;
            _multiLevel = multiLevel;
        }

        public void PostCompile()
        {
            if (_multiLevel)
                MultiLevel();
            else
                SingleLevel();
        }

        private void MultiLevel()
        {
            SceneNode root = _buffers.Nodes[_buffers.RootNode];
            root.Level = 0;
            DepthRecursion(root);
            List<SceneNode> evenChildren = new();
            List<SceneNode> oddChildren = new();
            foreach (var node in _buffers.Nodes)
            {
                foreach (var child in node.Children)
                {
                    if (child.Level % 2 == 0)
                    {
                        evenChildren.Add(child);
                    }
                    else
                    {
                        oddChildren.Add(child);
                    }
                }

                node.NumEven = (uint) evenChildren.Count;
                node.NumOdd = (uint) oddChildren.Count;
                node.Children = new List<SceneNode>();
                node.Children.AddRange(evenChildren);
                node.Children.AddRange(oddChildren);
                evenChildren.Clear();
                oddChildren.Clear();
            }
        }

        public void DepthRecursion(SceneNode node)
        {
            foreach (var child in node.Children)
            {
                child.Level = Math.Max(node.Level + 1, child.Level);
                DepthRecursion(child);
            }
        }

        public void ComputeAABBs(SceneNode node)
        {
            foreach (var child in node.Children) // children
            {
                ComputeAABBs(child);
                var (min, max) = TransformAABB(child.ObjectToWorld, child.AABB_min, child.AABB_max);
                node.AABB_min = Min(node.AABB_min, min);
                node.AABB_max = Max(node.AABB_max, max);
            }

            for (var i = 0; i < node.NumTriangles; i++)
            {
                var index = node.IndexBufferIndex + i * 3;
                for (var j = 0; j < 3; j++)
                {
                    var v = _buffers.VertexBuffer[(int)_buffers.IndexBuffer[index + j]];
                    var pos = new Vector4(v.Position[0], v.Position[1], v.Position[2], 1);
                    node.AABB_min = Min(node.AABB_min, pos);
                    node.AABB_max = Max(node.AABB_max, pos);
                }
            }
        }

        public (Vector4 min, Vector4 max) TransformAABB(Matrix4x4 transform, Vector4 min, Vector4 max)
        {
            Vector4[] aabbVertices ={
                new (min.X,min.Y,min.Z,1),
                new (min.X,min.Y,max.Z,1),
                new (min.X,max.Y,min.Z,1),
                new (min.X,max.Y,max.Z,1),
                new (max.X,min.Y,min.Z,1),
                new (max.X,min.Y,max.Z,1),
                new (max.X,max.Y,min.Z,1),
                new (max.X,max.Y,max.Z,1)
            };

            min = new(float.MaxValue, float.MaxValue, float.MaxValue, 1);
            max = new(-float.MaxValue, -float.MaxValue, -float.MaxValue, 1);

            foreach (var vert in aabbVertices)
            {
                var vec = Vector4.Transform(vert, transform);
                min = Min(min, vec);
                max = Max(max, vec);
            }
            return (min, max);
        }

        private Vector4 Max(Vector4 a, Vector4 b)
        {
            return new Vector4(
                Math.Max(a.X, b.X),
                Math.Max(a.Y, b.Y),
                Math.Max(a.Z, b.Z),
                1);
        }
        private Vector4 Min(Vector4 a, Vector4 b)
        {
            return new Vector4(
                Math.Max(a.X, b.X),
                Math.Max(a.Y, b.Y),
                Math.Max(a.Z, b.Z),
                1);
        }


        private void SingleLevel()
        {
            // collapse parent nodes
        }
    }
}