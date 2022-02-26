using System.Collections.Generic;

namespace Scene
{
    public static class SceneNodeExtensions
    {
        public static void AddChild(this SceneNode parent, SceneNode child)
        {
            parent.Buffers.AddChild(parent,child);
        }
        public static void AddParent(this SceneNode child, SceneNode parent)
        {
            child.Buffers.AddParent(child,parent);
        }
        public static void SetChildren(this SceneNode parent, IEnumerable<SceneNode> children)
        {
            parent.Buffers.SetChildren(parent, children);
        }
        public static void SetParents(this SceneNode child, IEnumerable<SceneNode> parents)
        {
            child.Buffers.SetParents(child, parents);
        }
        public static void ClearParents(this SceneNode child)
        {
            child.Buffers.ClearParents(child);
        }
        public static void ClearChildren(this SceneNode parent)
        {
            parent.Buffers.ClearChildren(parent);
        }
        public static int NumParents(this SceneNode child)
        {
            return child.Buffers.NumParents(child);
        }
        public static int NumChildren(this SceneNode parent)
        {
            return parent.Buffers.NumChildren(parent);
        }
        public static IEnumerable<SceneNode> Children(this SceneNode parent)
        {
            return parent.Buffers.GetChildren(parent);
        }
        public static IEnumerable<SceneNode> Parents(this SceneNode child)
        {
            return child.Buffers.GetParents(child);
        }

        public static int Index(this SceneNode node) => node.Index;
    }
}