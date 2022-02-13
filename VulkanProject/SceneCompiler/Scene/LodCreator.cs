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
        public List<SceneNode> CreateLevelsOfDetail(SceneNode node, int amount) // returns the number of nodes to add to the scene Buffer
        {
            if (!node.ForceEven)
                throw new Exception("can only create LOD selector for even nodes (a TLAS)"); // since InstanceHitCompute is exectued for even nodes

            var selector = new SceneNode
            {
                ForceEven = true,
                IsLodSelector = true,
                Name = "LOD-Sel " + node.Name,
            };

            var lods = new SceneNode[amount];
            foreach (var parent in node.Parents)
            {
                parent.Children.Remove(node);
                parent.Children.Add(selector);
                selector.Parents.Add(parent);
            }
            node.Parents.Clear();
            node.Parents.Add(selector);
            selector.Children.Add(node);
            node.Name = "LOD 0 " + node.Name;
            lods[0] = node;
            // create


            // add

            return new List<SceneNode> { selector };
        }
    }
}
