using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Threading.Tasks;

namespace SceneCompiler.Scene.SceneTypes
{
    public class SceneBuffers
    {
        public List<Vertex> VertexBuffer { get; set; } = new();
        public List<uint> IndexBuffer { get; set; } = new();
        public List<Material> MaterialBuffer { get; set; } = new();
        public int RootIndex { get => Root.Index; }
        public SceneNode Root { get; set; }
        public IEnumerable<SceneNode> Nodes { get => SceneNodes; }
        private List<SceneNode> SceneNodes = new();
        private Dictionary<SceneNode, NodeListElement> ChildrenDictionary = new();
        private Dictionary<SceneNode, NodeListElement> ParentDictionary = new(); 

        public void Add(SceneNode node)
        {
            node.Index = SceneNodes.Count;
            SceneNodes.Add(node);
        }
        public void AddRange(IEnumerable<SceneNode> nodes)
        {
            SceneNodes.Capacity += nodes.Count();
            foreach(var node in nodes)
            {
                node.Index = SceneNodes.Count;
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
            foreach (var node in nodes)
            {
                node.Index = newList.Count;
                newList.Add(node);
            }
            SceneNodes = newList;
        }
        public void ValidateSceneNodes()
        {
            foreach (var node in SceneNodes)
            {
                if(SceneNodes[node.Index] != node)
                    throw new Exception("Index doesnt match actual index");
            }
        }

        public IEnumerable<SceneNode> GetChildren(SceneNode node)
        {
            if(ChildrenDictionary.TryGetValue(node, out var elem)) {
                return elem;
            }
            return Array.Empty<SceneNode>();
        }

        public IEnumerable<SceneNode> GetParents(SceneNode node)
        {
            if (ParentDictionary.TryGetValue(node, out var elem))
            {
                return elem;
            }
            return Array.Empty<SceneNode>();
        }

        public void AddParent(SceneNode child, SceneNode parent)
        {
            if (ParentDictionary.TryGetValue(child, out var parents)) {
                ParentDictionary.Remove(child);
            }
            var elem = new NodeListElement
            {
                Node = parent,
                Next = parents
            };
            ParentDictionary.Add(child, elem);
        }

        public void AddChild(SceneNode parent, SceneNode child)
        {
            if (ChildrenDictionary.TryGetValue(parent, out var children))
            {
                ChildrenDictionary.Remove(parent);
            }
            var elem = new NodeListElement
            {
                Node = child,
                Next = children
            };
            ChildrenDictionary.Add(parent, elem);
        }

        public void SetChildren(SceneNode parent, IEnumerable<SceneNode> children)
        {
            ChildrenDictionary.Remove(parent);
            if (!children.Any()) return;
            var current = new NodeListElement
            {
                Node = children.First()
            };
            ChildrenDictionary.Add(parent, current);
            foreach(var next in children.Skip(1))
            {
                var nextElem = new NodeListElement
                {
                    Node = next
                };
                current.Next = nextElem;
                current = nextElem;
            }
        }

        public void SetParents(SceneNode child, IEnumerable<SceneNode> parents)
        {
            ParentDictionary.Remove(child);
            if (!parents.Any()) return;
            var current = new NodeListElement
            {
                Node = parents.First()
            };
            ParentDictionary.Add(child, current);
            foreach (var next in parents.Skip(1))
            {
                var nextElem = new NodeListElement
                {
                    Node = next
                };
                current.Next = nextElem;
                current = nextElem;
            }
        }

        public void ClearParents(SceneNode child)
        {
            ParentDictionary.Remove(child);
        }
        public void ClearChildren(SceneNode parent)
        {
            ChildrenDictionary.Remove(parent);
        }

        public void RewriteAllParents()
        {
            ParentDictionary.Clear();

            foreach (var parent in SceneNodes)
            {
                foreach (var child in parent.Children)
                {
                    AddParent(child, parent);
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
                t.Index = -1;
            });
            root.Index = 0;
            BuildListRecursive(root, 1);

            var arr = new SceneNode[SceneNodes.Count(x => x.Index != -1)];
            var unused = 0;
            foreach (var node in SceneNodes)
            {
                if (node.Index == -1)
                {
                    unused++;
                    continue;
                }
                arr[node.Index] = node;
            }

            Console.WriteLine("Removed " + unused + " unused Nodes");

            SceneNodes = arr.ToList();

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
                if (node.Children.First().Index < 0)
                {
                    node.Children.First().Index = indexOffset;
                    indexOffset++;
                    node = node.Children.First();
                }
            }
            foreach (var child in node.Children)
            {
                if (child.Index < 0)
                {
                    child.Index = indexOffset;
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

        private class NodeListElement : IEnumerable<SceneNode>
        {
            public SceneNode Node;
            public NodeListElement Next;

            public IEnumerator<SceneNode> GetEnumerator()
            {
                return new NodeListEnumerator(this);
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }
            public override string ToString()
            {
                return Node.ToString();
            }
        }
        private class NodeListEnumerator : IEnumerator<SceneNode>
        {
            public SceneNode Current => current.Node;

            private NodeListElement current;
            private NodeListElement start;

            object IEnumerator.Current => Current;

            public NodeListEnumerator(NodeListElement start)
            {
                this.start = start;
            }

            public void Dispose()
            {
            }

            public bool MoveNext()
            {
                if (current == null)
                {
                    current = start;
                    return true;
                };
                current = current.Next;
                return current != null;
            }

            public void Reset()
            {
                current = null;
            }
        }
    }
}