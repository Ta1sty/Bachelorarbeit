using SceneCompiler.Scene.SceneTypes;
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
            // we want root to be instanced, therefore root must be odd
            root.ForceOdd = true;

            var min = root.AABB_min;
            var max = root.AABB_max;
            foreach(var node in _buffers.Nodes)
            {
                node.ResetAABB();
            }

            var offsetX = -min.X;
            var offsetZ = -min.Z;
            var extentX = max.X - min.X;
            var extentZ = max.Z - min.Z;

            // construct a new instanceList, we also use this as the root
            var newRoot = new SceneNode
            {
                IsInstanceList = true,
                Name = "List ROOT",
                ForceEven = true,
            };
            _buffers.Add(newRoot);
            _buffers.Root = newRoot;
            for(var x = 0; x < numX; x++)
            {
                for(var z = 0; z < numZ; z++)
                {
                    var xTranslation = offsetX + (x-numX/2) * extentX; // center around 0,0 and in all 4 directions
                    var zTranslation = offsetZ + (z-numZ/2) * extentZ;
                    var add = new SceneNode
                    {
                        ObjectToWorld = Matrix4x4.CreateTranslation(xTranslation, 0, zTranslation),
                        Name = "Inst ROOT",
                        ForceEven = true,
                    };
                    _buffers.AddChild(add, root);
                    _buffers.AddChild(newRoot, add);
                    _buffers.Add(add);
                }
            }
        }
    }
}
