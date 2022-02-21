using SceneCompiler.Scene.SceneTypes;
using System;
using System.Collections.Generic;
using System.Linq;
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
        public void CreateLevelsOfDetail(SceneNode node, int amount) // returns the number of nodes to add to the scene Buffer
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
                    IndexBufferIndex = node.IndexBufferIndex,
                    IsInstanceList = node.IsInstanceList,
                    NumTriangles = node.NumTriangles,
                    ObjectToWorld = node.ObjectToWorld,
                };
                _buffers.Add(lod);
                _buffers.AddParent(lod,selector);
                _buffers.AddChild(selector, lod);
                _buffers.SetChildren(lod, node.Children);
                foreach (var child in node.Children)
                {
                    _buffers.AddParent(child, lod);
                }
            }
            _buffers.AddChild(selector, node);
        }
    }
}
