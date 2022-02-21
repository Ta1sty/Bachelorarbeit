using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Threading.Tasks;

namespace SceneCompiler.Scene.SceneTypes
{
    public class SceneBuffers  :IDisposable
    {
        internal static SceneBuffers ActiveBuffer { get; set; }

        public List<Vertex> VertexBuffer { get; set; } = new();
        public List<uint> IndexBuffer { get; set; } = new();
        public List<Material> MaterialBuffer { get; set; } = new();
        public int RootIndex { get => Root.Index; }
        public SceneNode Root { get; set; }
        public IEnumerable<SceneNode> Nodes { get => SceneNodes; }

        private List<SceneNode> SceneNodes = new();
        private Dictionary<int, int[]> ChildrenList = new();
        private Dictionary<int, int[]> ParentList = new();

        public SceneBuffers()
        {
            if (ActiveBuffer != null)
                throw new Exception("There can only be one active SceneBuffer at any time");
            ActiveBuffer = this;
        }

        public void Dispose()
        {
            ActiveBuffer = null;
        }

        public void Add(SceneNode node)
        {
            node.Index = SceneNodes.Count;
            node.FirstChild = -1;
            node.FirstParent = -1;
            SceneNodes.Add(node);
        }
        public void AddRange(IEnumerable<SceneNode> nodes)
        {
            SceneNodes.Capacity += nodes.Count();
            foreach(var node in nodes)
            {
                node.Index = SceneNodes.Count;
                node.FirstChild = -1;
                node.FirstParent = -1;
                SceneNodes.Add(node);
            }
        }
        public SceneNode NodeByIndex(int index)
        {
            return SceneNodes[index];
        }
        public void SetSceneNodes(IEnumerable<SceneNode> nodes)
        {
            var newList = new List<SceneNode>(nodes.Count());
            var indexMap = new Dictionary<int, int>(SceneNodes.Count);
            foreach (var node in nodes)
            {
                var newIndex = newList.Count;
                indexMap.Add(node.Index, newIndex);
                node.Index = newIndex;
                newList.Add(node);
            }
            SceneNodes = newList;

            // update parent and child indices

            UpdateRelations(indexMap);
        }
        private void UpdateRelations(Dictionary<int,int> indexMap)
        {
            {
                var newChildDict = new Dictionary<int, int[]>(ChildrenList.Count);

                foreach(var node in SceneNodes)
                {
                    if(node.FirstChild >= 0) 
                        node.FirstChild = indexMap.GetValueOrDefault(node.FirstChild, -1);
                    if(node.FirstChild >= 0)
                        node.FirstParent = indexMap.GetValueOrDefault(node.FirstParent, -1);
                }

                foreach (var (parent,children) in ChildrenList)
                {
                    var newParent = indexMap.GetValueOrDefault(parent, -1);
                    if (newParent == -1)
                        continue;
                    SceneNodes[newParent].FirstChild = -1;
                    var newChildren = children.Select(x => indexMap.GetValueOrDefault(x, -1))
                        .Where(x => x >= 0).Distinct().ToArray();
                    if (newChildren.Length == 0)
                        continue;
                    if (newChildren.Length == 1)
                        SceneNodes[newParent].FirstChild = newChildren.First();
                    else
                        newChildDict.Add(newParent, newChildren);
                }
                ChildrenList = newChildDict;
            }

            {
                var newParentDict = new Dictionary<int, int[]>(ParentList.Count);

                foreach (var (child, parents) in ParentList)
                {
                    var newChild = indexMap.GetValueOrDefault(child, -1);
                    if (newChild == -1)
                        continue;
                    SceneNodes[newChild].FirstParent = -1;
                    var newPartents = parents.Select(x => indexMap.GetValueOrDefault(x, -1))
                        .Where(x => x >= 0).Distinct().ToArray();
                    if (newPartents.Length == 0)
                        continue;
                    if (newPartents.Length == 1)
                        SceneNodes[newChild].FirstParent = newPartents.First();
                    else
                        newParentDict.Add(newChild, newPartents);
                }
                ParentList = newParentDict;
            }
        }
        public void ValidateSceneNodes()
        {
            foreach (var node in SceneNodes)
            {
                if(SceneNodes[node.Index] != node)
                    throw new Exception("Index doesnt match actual index");
            }
        }

        public IEnumerable<SceneNode> GetChildren(SceneNode parent)
        {
            if (parent.FirstChild >= 0)
                return new SceneNode[1] { SceneNodes[parent.FirstChild] };
            return ChildrenList.GetValueOrDefault(parent.Index, Array.Empty<int>()).Select(x=> SceneNodes[x]);
        }

        public IEnumerable<SceneNode> GetParents(SceneNode child)
        {
            if (child.FirstParent >= 0)
                return new SceneNode[1] { SceneNodes[child.FirstParent] };
            return ParentList.GetValueOrDefault(child.Index, Array.Empty<int>()).Select(x => SceneNodes[x]);
        }

        public void AddChild(SceneNode parent, SceneNode child)
        {
            if (parent.Index < 0)
                throw new Exception("parent must be part of the Nodes buffer");
            var children = GetChildren(parent).Select(x=>x.Index).ToArray();
            if (!children.Any())
            {
                parent.FirstChild = child.Index;
                return;
            }
            ChildrenList.Remove(parent.Index);
            parent.FirstChild = -1;
            ChildrenList.Add(parent.Index, children.Append(child.Index).ToArray());
        }

        public void SetChildren(SceneNode parent, IEnumerable<SceneNode> children)
        {
            if (parent.Index < 0)
                throw new Exception("parent must be part of the Nodes buffer");
            ChildrenList.Remove(parent.Index);
            parent.FirstChild = -1;
            if (!children.Any()) return;
            if (!children.Skip(1).Any())
            {
                parent.FirstChild = children.First().Index;
                return;
            }
            ChildrenList.Add(parent.Index, children.Select(x=>x.Index).ToArray());
        }

        public void AddParent(SceneNode child, SceneNode parent)
        {
            if (parent.Index < 0)
                throw new Exception("parent must be part of the Nodes buffer");
            var parents = GetParents(child).Select(x => x.Index).ToArray();
            if (!parents.Any())
            {
                child.FirstParent = parent.Index;
                return;
            }
            ParentList.Remove(child.Index);
            child.FirstParent = -1;
            ParentList.Add(child.Index, parents.Append(parent.Index).ToArray());
        }

        public void SetParents(SceneNode child, IEnumerable<SceneNode> parents)
        {
            if (child.Index < 0)
                throw new Exception("parent must be part of the Nodes buffer");
            ParentList.Remove(child.Index);
            child.FirstParent = -1;
            if (!parents.Any()) return;
            if (!parents.Skip(1).Any())
            {
                child.FirstParent = parents.First().Index;
                return;
            }
            ParentList.Add(child.Index, parents.Select(x => x.Index).ToArray());
        }

        public void ClearParents(SceneNode child)
        {
            child.FirstParent = -1;
            ParentList.Remove(child.Index);
        }
        public void ClearChildren(SceneNode parent)
        {
            parent.FirstChild = -1;
            ChildrenList.Remove(parent.Index);
        }

        public void RewriteAllParents()
        {
            ParentList = new Dictionary<int, int[]>(ParentList.Count);

            foreach (var node in SceneNodes)
            {
                node.NewIndex = 0;
                node.FirstParent = -1;
            }
            foreach (var parent in SceneNodes)
            {
                foreach (var child in parent.Children)
                {
                    child.NewIndex++;
                }
            }

            foreach(var node in SceneNodes)
            {
                if (node.NewIndex > 1)
                    ParentList.Add(node.Index, new int[node.NewIndex]);
                else
                    node.NewIndex = -1;
            }
            foreach (var parent in SceneNodes)
            {
                foreach (var child in parent.Children)
                {
                    child.NewIndex--;
                    if (child.NewIndex >= 0)
                        ParentList.GetValueOrDefault(child.Index)[child.NewIndex] = parent.Index;
                    else
                        child.FirstParent = parent.Index;
                }
            }
        }

        public int Count() => SceneNodes.Count;

        public void RebuildNodeBufferFromRoot()
        {
            Console.WriteLine("Rebuilding the buffer from the root");
            var root = Root;
            Parallel.ForEach(Nodes, t =>
            {
                t.NewIndex = -1;
            });
            root.NewIndex = 0;
            BuildListRecursive(root, 1);

            var mapping = new Dictionary<int, int>();

            var arr = new SceneNode[SceneNodes.Count(x => x.NewIndex != -1)];
            var unused = 0;
            foreach (var node in SceneNodes)
            {
                if (node.NewIndex == -1)
                {
                    unused++;
                    continue;
                }
                mapping.Add(node.Index, node.NewIndex);
                arr[node.NewIndex] = node;
                node.Index = node.NewIndex;
            }
            // create the mapping
            SceneNodes = arr.ToList();

            UpdateRelations(mapping);

            Console.WriteLine("Removed " + unused + " unused Nodes");


            RewriteAllParents();

            ValidateSceneNodes();
        }
        // use DFS Layout since this also the way traversal is done, should improve caching
        private int BuildListRecursive(SceneNode node, int indexOffset)
        {

            if (!node.Children.Any())
                return indexOffset;

            int start = indexOffset;

            if (node.IsInstanceList)
            {
                if (node.Children.First().NewIndex < 0)
                {
                    node.Children.First().NewIndex = indexOffset;
                    indexOffset++;
                    node = node.Children.First();
                }
            }
            foreach (var child in node.Children)
            {
                if (child.NewIndex < 0)
                {
                    child.NewIndex = indexOffset;
                    indexOffset++;
                }
            }

            if (start == indexOffset) return indexOffset; // all were already added
                                                          // call for all children
                                                          // TODO might want to avoid calling twice for a node


            foreach (var child in node.Children)
            {
                indexOffset = BuildListRecursive(child, indexOffset);
            }

            return indexOffset;
        }
    }
}