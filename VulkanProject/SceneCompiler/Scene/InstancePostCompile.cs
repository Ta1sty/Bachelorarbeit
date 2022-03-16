using Scene;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace SceneCompiler.Scene
{
    class InstancePostCompile
    {
        private static Random _random = new(21092000);
        private static float Rand => (float)_random.NextDouble();
        private readonly SceneBuffers _buffers;
        public InstancePostCompile(SceneBuffers buffers)
        {
            _buffers = buffers;
        }

        public void InstanceMultiple(int numX, int numZ)
        {
            var root = _buffers.Root;

            foreach (var node in _buffers.Nodes)
            {
                node.ResetAABB();
            }
            root.ComputeAABBs(_buffers);

            var min = root.AABB_min;
            var max = root.AABB_max;
            foreach(var node in _buffers.Nodes)
            {
                node.ResetAABB();
            }

            var offsetX = -min.X;
            var offsetZ = -min.Z;
            var offsetY = -min.Y;
            var extentX = max.X - min.X;
            var extentY = max.Y - min.Y;
            var extentZ = max.Z - min.Z;

            var varianceX = 0f;
            var varianceZ = 0f;
            var randomize = CompilerConfiguration.Configuration.InstanceConfiguration.Randomize;
            if (randomize)
            {
                varianceX = extentX;
                varianceZ = extentZ;
            }

            // construct a new instanceList, we also use this as the root
            var newRoot = new SceneNode
            {
                IsInstanceList = true,
                Name = "List ROOT"
            };
            _buffers.Add(newRoot);
            _buffers.Root = newRoot;
            var added = new List<SceneNode>(numX*numZ);

            var factor = CompilerConfiguration.Configuration.InstanceConfiguration.SpacingFactor;

            for(var x = 0; x < numX; x++)
            {
                for(var z = 0; z < numZ; z++)
                {
                    var add = new SceneNode
                    {
                        Name = "Inst ROOT",
                    };

                    if (randomize)
                    {
                        var scaleVector = new Vector3(.8f + Rand*0.5f, .8f + Rand * 0.5f, .8f + Rand * 0.5f);
                        var scale = Matrix4x4.CreateScale(scaleVector);
                        var rot = Matrix4x4.CreateRotationY((float) (Rand * 2 * Math.PI));


                        var xTranslation = offsetX + (x - numX / 2) * extentX * factor;
                        var zTranslation = offsetZ + (z - numZ / 2) * extentZ * factor;
                        xTranslation += varianceX * (Rand - 0.5f);
                        zTranslation += varianceZ * (Rand - 0.5f);
                        
                        var yTranslation = offsetY;

                        var trans = Matrix4x4.CreateTranslation(xTranslation, yTranslation, zTranslation);

                        add.ObjectToWorld = trans * rot * scale;
                    }
                    else
                    {
                        var xTranslation = offsetX + (x - numX / 2) * extentX * factor;
                        var zTranslation = offsetZ + (z - numZ / 2) * extentZ * factor;
                        add.ObjectToWorld = Matrix4x4.CreateTranslation(xTranslation, 0, zTranslation);
                    }

                    _buffers.Add(add);
                    add.AddChild(root);
                    added.Add(add);
                }
            }
            newRoot.Children = added;
        }
    }
}
