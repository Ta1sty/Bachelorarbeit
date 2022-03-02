using System;
using System.Collections.Generic;
using System.ComponentModel.Design.Serialization;
using System.Linq;
using System.Numerics;
using System.Threading.Tasks;
using Scene;

namespace SceneCompiler.Scene
{
    public class RayTracePostCompile
    {
        private readonly SceneBuffers _buffers;

        private static readonly CompilerConfiguration Configuration =
            CompilerConfiguration.Configuration;
        public RayTracePostCompile(SceneBuffers buffers)
        {
            _buffers = buffers;
        }

        public void PostCompile()
        {
            Console.WriteLine("Adjusting Scene Graph");

            var root = _buffers.Root;
            root.Level = 0;
            root.ObjectToWorld = Matrix4x4.Identity;

            if (Configuration.DebugConfiguration.InsertGround)
            {
                Console.WriteLine("Inserting ground");
                InsertGround();
            }

            if (Configuration.OptimizationConfiguration.MergeInstanceLists)
            {
                Console.WriteLine("Merge instance Lists");
                MergeInstanceList();
            }

            if (Configuration.OptimizationConfiguration.CapInstanceLists)
            {
                Console.WriteLine("Ensure Device Limitations");
                CapInstanceLists();
            }

            Console.WriteLine("Writing Levels");
            DepthRecursion(root);
            Console.WriteLine("Number of Triangles: " + root.TotalPrimitiveCount);
            Console.WriteLine("Removing empty children");
            foreach (var node in _buffers.Nodes) node.Children = node.Children.Where(x => x.TotalPrimitiveCount > 0);

            _buffers.SetSceneNodes(_buffers.Nodes.Where(x=>x.TotalPrimitiveCount > 0));

            Console.WriteLine("inserting dummys");
            List<SceneNode> blasAdd = new();
            List<SceneNode> tlasAdd = new();
            var count = _buffers.Count();
            for (var i = 0; i < count; i++)
            {
                var node = _buffers.NodeByIndex(i);
                if (node.Level % 2 == 0)
                    AdjustEvenNode(node, blasAdd);
                else
                    AdjustOddNode(node, tlasAdd);
            }

            Console.WriteLine("added " + (_buffers.Count() - count) + " dummies to the scenegraph");
            Console.WriteLine("computing AABBs");
            foreach (var node in _buffers.Nodes)
                node.ResetAABB();
            root.ComputeAABBs(_buffers);
        }

        private void InsertGround()
        {
            var root = _buffers.Root;
            root.ComputeAABBs(_buffers);

            var add = new SceneNode
            {
                NumTriangles = 2,
                IndexBufferIndex = _buffers.IndexBuffer.Count,
                ForceOdd = true,
                Name = "Ground"
            };
            _buffers.Add(add);

            var extent = root.AABB_max - root.AABB_min;
            var offset = root.AABB_min;

            var start = (uint) _buffers.VertexBuffer.Count;

            float[] vf0 = { 0, 0, 0, 0, 1, 0, 0, 0};
            float[] vf1 = { 1, 0, 0, 0, 1, 0, 1, 0 };
            float[] vf2 = { 0, 0, 1, 0, 1, 0, 0, 1 };
            float[] vf3 = { 1, 0, 1, 0, 1, 0, 1, 1 };

            var material = new SceneMaterial
            {
                Ambient = 0,
                Diffuse = 0,
                Specular = 0,
                PhongExponent = 1,
                Transmission = 0.5f,
                Reflection = 0.5f,
            };
            var materialIndex = _buffers.MaterialBuffer.Count;
            _buffers.MaterialBuffer.Add(material);

            var v0 = new Vertex(0, vf0, materialIndex, (int) start);
            var v1 = new Vertex(0, vf1, materialIndex, (int) start);
            var v2 = new Vertex(0, vf2, materialIndex, (int) start);
            var v3 = new Vertex(0, vf3, materialIndex, (int) start);

            _buffers.VertexBuffer.Add(v0);
            _buffers.VertexBuffer.Add(v1);
            _buffers.VertexBuffer.Add(v2);
            _buffers.VertexBuffer.Add(v3);

            _buffers.IndexBuffer.Add(start + 0);
            _buffers.IndexBuffer.Add(start + 1);
            _buffers.IndexBuffer.Add(start + 3);

            _buffers.IndexBuffer.Add(start + 0);
            _buffers.IndexBuffer.Add(start + 2);
            _buffers.IndexBuffer.Add(start + 3);

            var scale = Matrix4x4.CreateScale(extent.X, 1, extent.Z);
            var trans = Matrix4x4.CreateTranslation(offset);

            add.ObjectToWorld = scale * trans;

            if (root.IsLodSelector)
                throw new Exception("Can not insert ground if ROOT is LOD selector");
            if (root.IsInstanceList) {
                var inst = new SceneNode
                {
                    ObjectToWorld = add.ObjectToWorld,
                    ForceEven = true,
                    Name = "Inst Ground"
                };
                add.ObjectToWorld = Matrix4x4.Identity;
                _buffers.Add(inst); 
                inst.AddChild(add);
                root.AddChild(inst);
            }
            else
            {
                root.AddChild(add);
            }
        }

        private void MergeInstanceList()
        {
            Console.WriteLine("Merging instance Lists");
            var add = new List<SceneNode>();

            foreach (var node in _buffers.Nodes)
                if (node.Children.Count(x => x.IsInstanceList) > 1 && !node.IsLodSelector)
                {
                    var merge = true;
                    // every instance list must have the same transfrom, ie all use identity(cause thats convenient)
                    foreach (var child in node.Children.Where(x => x.IsInstanceList))
                        if (!SceneNode.MatrixAlmostZero(child.ObjectToWorld - Matrix4x4.Identity))
                        {
                            merge = false;
                            break;
                        }

                    if (!merge)
                        continue;

                    // every parent that references these lists must reference exactly these lists and none else
                    // (could be better, but unimportant for now)
                    var parents = node.Children.SelectMany(x => x.Parents).Distinct().ToList();
                    foreach (var parent in parents)
                        if (!SetEqual(parent.Children.Where(x => x.IsInstanceList),
                                node.Children.Where(x => x.IsInstanceList)))
                        {
                            merge = false;
                            break;
                        }

                    if (!merge)
                        continue;
                    // do the actual merge by inserting all children into the frist instance list and removing the others from the childChain
                    var first = node.Children.First(x => x.IsInstanceList);
                    var children = node.Children.SelectMany(x => x.Children).ToList();
                    foreach (var child in children)
                    {
                        child.ClearParents();
                        child.AddParent(first);
                    }

                    foreach (var list in node.Children.Where(x => x.IsInstanceList))
                        list.ClearChildren();
                    first.ClearChildren();
                    first.Children = children;
                    foreach (var parent in parents)
                    {
                        parent.Children = parent.Children.Where(x => !x.IsInstanceList);
                        parent.AddChild(first);
                    }

                    first.Name += " Merge " + children.Count();
                }
        }

        private bool SetEqual<T>(IEnumerable<T> list1, IEnumerable<T> list2)
        {
            return list1.Count() == list2.Count() && list1.All(list2.Contains) && list2.All(list1.Contains);
        }

        public void AdjustEvenNode(SceneNode node, List<SceneNode> blasAdd)
        {
            List<SceneNode> evenChildren = new();
            List<SceneNode> oddChildren = new();
            foreach (var child in node.Children)
                if (child.Level % 2 == 0)
                    evenChildren.Add(child);
                else
                    oddChildren.Add(child);

            if (node.NumTriangles == 0 && evenChildren.Count == 0) return;

            bool Selector(SceneNode x)
            {
                if (x.NumChildren != evenChildren.Count)
                    return false;
                if (evenChildren.Any(child => !x.Children.Contains(child)))
                {
                    return false;
                }
                if (x.Children.Any(child => !evenChildren.Contains(child)))
                {
                    return false;
                }
                if (x.IndexBufferIndex != node.IndexBufferIndex)
                    return false;
                if (x.NumTriangles != node.NumTriangles)
                    return false;

                return true;
            }

            var dummy = blasAdd.Where(Selector).SingleOrDefault();
            if (dummy == null)
            {
                dummy = new SceneNode
                {
                    // set new Level and index
                    Level = node.Level + 1,
                    Name = "DummyBLAS",
                    // add the geometry
                    IndexBufferIndex = node.IndexBufferIndex,
                    NumTriangles = node.NumTriangles,
                    // add the even children
                    // set transforms to identity
                    ObjectToWorld = Matrix4x4.Identity
                };
                blasAdd.Add(dummy);
                _buffers.Add(dummy);
                dummy.Children = new List<SceneNode>(evenChildren);
            }

            // add odd dummy to buffer and to children of the even node
            oddChildren.Add(dummy);
            // removed geometry
            node.IndexBufferIndex = -1;
            node.NumTriangles = 0;

            // add only the odd children
            node.Children = oddChildren;
        }

        public void AdjustOddNode(SceneNode node, List<SceneNode> tlasAdd)
        {
            List<SceneNode> evenChildren = new();
            List<SceneNode> oddChildren = new();
            foreach (var child in node.Children)
                if (child.Level % 2 == 0)
                    evenChildren.Add(child);
                else
                    oddChildren.Add(child);

            if (oddChildren.Count > 0)
            {
                // we solve this by building a new tlas for this reference
                // but this can happen multiple times for the same blas
                // so if possible we want to reuse the tlas
                var dummy = tlasAdd.Where(x =>
                {
                    if (x.NumChildren != oddChildren.Count)
                        return false;
                    foreach (var child in oddChildren)
                    {
                        if (!x.Children.Contains(child))
                            return false;
                    }
                    foreach (var child in x.Children)
                    {
                        if (!oddChildren.Contains(child))
                            return false;
                    }

                    return true;
                }).SingleOrDefault();

                if (dummy == null)
                {
                    dummy = new SceneNode
                    {
                        // set new Level and index
                        Level = node.Level + 1,
                        Name = "DummyTLAS",
                        // add the odd children
                        // set transforms to identity
                        ObjectToWorld = Matrix4x4.Identity
                    };
                    tlasAdd.Add(dummy);
                    _buffers.Add(dummy);
                    dummy.Children = new List<SceneNode>(oddChildren);
                }

                // add even dummy to buffer and children of the odd node
                evenChildren.Add(dummy);

                // add only the even children
                node.Children = evenChildren;
            }
        }

        public void Validate()
        {
            Console.WriteLine("Validating SceneGraph");
            _buffers.ValidateSceneNodes();
            Parallel.ForEach(_buffers.Nodes, node =>
            {
                if (node.Level < 0)
                    throw new Exception("Level was not set");
                foreach (var child in node.Children)
                {
                    if (child.Level <= node.Level)
                        throw new Exception("Child level must always be at least one higher than parent");
                    if (child.Level + node.Level % 2 == 0)
                        throw new Exception("Either two odd levels or two even levels are parent and child");
                }

                if (node.IsInstanceList)
                {
                    if (node.NumTriangles > 0)
                        throw new Exception("instance List can not contain geometry");

                    if (node.Children.Count() != 1)
                        throw new Exception("instance List must have exactly one child (DummyBLAS/DummyTLAS)");

                    var dummy = node.Children.First();

                    if (!dummy.Name.StartsWith("Dummy"))
                        throw new Exception("child of instance List is not a dummy");

                    foreach (var child in dummy.Children)
                    {
                        if (child.Level % 2 != node.Level % 2)
                            throw new Exception("Instance list elements must have the same parity as the instance list");
                        if (child.NumTriangles > 0)
                            throw new Exception("Instance list elements can not reference triangles");
                        if (node.Level % 2 == 1 && child.NumChildren > 1)
                            throw new Exception("Children of odd instance lists can only have 1 child");
                        foreach (var parent in child.Parents)
                        {
                            if (!parent.Name.StartsWith("Dummy"))
                                throw new Exception("Parent of instance list element must be a Dummy");
                            foreach (var grandParent in parent.Parents)
                                if (!grandParent.IsInstanceList)
                                    throw new Exception("Grandparent of instance list element must be instance list");
                        }
                    }
                }

                if (node.Index() < 0)
                    throw new Exception("Index was not set");
                if (node.Level % 2 == 0 && node.NumTriangles > 0)
                    throw new Exception("Even node references Geometry");
                if (node.NumChildren == 0 && node.NumTriangles == 0)
                    throw new Exception("Node doesn't reference anything");
                if (node.NumTriangles > 0 && node.IndexBufferIndex < 0)
                    throw new Exception("Node says it contains geometry but index buffer pointer is not set");
                if (node.Brother != null)
                    throw new Exception("An identical brother exists, this node was supposed to be removed");
                if (!Configuration.DebugConfiguration.ValidateParentsAndChildren) return;
                if (!node.Parents.All(x => x.Children.Contains(node)))
                    throw new Exception("This node references a parent that doesn't reference this node as a child");
                if (!node.Children.All(x => x.Parents.Contains(node)))
                    throw new Exception("This node references a child that doesn't reference this node as a parent");
            });
        }

        public void PrintScene()
        {
            var printAll = Configuration.DebugConfiguration.PrintFullScene 
                           || _buffers.Count() < Configuration.DebugConfiguration.PrintFullThreshold;
            foreach (var node in _buffers.Nodes)
            {
                var str = node.ToString();
                if (!printAll)
                {
                    if (!node.IsLodSelector && !node.NeedsBlas && !node.NeedsTlas) continue;
                }

                Console.WriteLine(str);
            }
        }

        private void DepthRecursion(SceneNode node)
        {
            if (node.IsInstanceList)
            {
                foreach (var child in node.Children)
                {
                    if (node.Level + 1 < child.Level) continue;
                    if (child.ForceOdd != node.ForceOdd)
                        throw new Exception("Forcing a list to be odd requires forcing all instances to be odd themselfs");
                    if (child.ForceEven != node.ForceEven)
                        throw new Exception("Forcing a list to be even requires forcing all instances to be even themselfs");
                    child.Level = node.Level + 2;
                    DepthRecursion(child);
                }
            }
            else
            {
                foreach (var child in node.Children)
                {
                    if (node.Level >= child.Level)
                    {
                        child.Level = node.Level + 1;
                        if (child.ForceOdd && child.Level % 2 == 0)
                            child.Level++;
                        if (child.ForceEven && child.Level % 2 == 1)
                            child.Level++;
                        DepthRecursion(child);
                    }
                }
            }


            node.TotalPrimitiveCount = node.NumTriangles + node.Children.Sum(x => x.TotalPrimitiveCount);
        }

        public void CapInstanceLists()
        {
            var max = Configuration.OptimizationConfiguration.MaxInstances;
            var root = _buffers.Root;
            var toSplit = _buffers.Nodes
                .Where(node => node.IsInstanceList && node.Children.Skip(max).Any())
                .ToList();

            if (toSplit.Any() && Configuration.OptimizationConfiguration.UseSingleLevelInstancing)
                throw new Exception("Capping instance lists is not possible with single level" +
                                    ", as it requires traversal");

            while (toSplit.Any())
            {
                var left = toSplit[^1];
                toSplit.RemoveAt(toSplit.Count - 1);
                left.ComputeAABBs(_buffers);
                var (leftChildren, rightChildren) = SplitNodes(left);

                var right = new SceneNode
                {
                    ForceEven = left.ForceEven,
                    ForceOdd = left.ForceOdd,
                    IsLodSelector = left.IsLodSelector,
                    IsInstanceList = left.IsInstanceList,
                    ObjectToWorld = left.ObjectToWorld,
                    Name = "R" + left.Name
                };
                _buffers.Add(right);
                right.Children = rightChildren;
                right.Parents = left.Parents;
                left.Name = "L" + left.Name;
                left.Children = leftChildren;

                foreach (var parent in left.Parents) parent.AddChild(right);

                foreach (var child in rightChildren)
                {
                    child.Parents = child.Parents.Where(x => x != left);
                    child.AddParent(right);
                }


                if (left.IsInstanceList && left.Children.Skip(max).Any())
                    toSplit.Add(left);
                if (right.IsInstanceList && right.Children.Skip(max).Any())
                    toSplit.Add(right);
                left.ResetAABB();
                right.ResetAABB();
            }

            foreach (var node in _buffers.Nodes) node.ResetAABB();
        }

        private (List<SceneNode> left, List<SceneNode> right) SplitNodes(SceneNode node)
        {
            var count = node.Children.Count();
            var left = new List<SceneNode>(count / 2 + 10);
            var right = new List<SceneNode>(count / 2 + 10);


            double medX = 0;
            double medY = 0;
            double medZ = 0;
            foreach (var child in node.Children)
            {
                var middle = (child.AABB_max + child.AABB_min) / 2;
                medX += middle.X;
                medY += middle.Y;
                medZ += middle.Z;
            }

            medX = medX / count;
            medY = medY / count;
            medZ = medZ / count;

            var extent = node.AABB_max - node.AABB_min;

            var axis = 0;
            var split = medX;
            if (extent.Y > extent.X || extent.Z > extent.X)
            {
                if (extent.Y > extent.Z)
                {
                    axis = 1;
                    split = medY;
                }
                else
                {
                    axis = 2;
                    split = medZ;
                }
            }

            foreach (var child in node.Children)
            {
                var middle = (child.AABB_max + child.AABB_min) / 2;
                double value = 0;
                if (axis == 0) value = middle.X;
                if (axis == 1) value = middle.Y;
                if (axis == 2) value = middle.Z;
                if (value < split)
                    left.Add(child);
                else
                    right.Add(child);
            }

            return (left, right);
        }

        public void ToSingleLevel()
        {
            var root = _buffers.Root;
            var newRoot = new SceneNode
            {
                IsInstanceList = true,
                Name = "ROOT-SingleLevel"
            };
            var children = new List<SceneNode>();
            Recursion(root, Matrix4x4.Identity, children);
            _buffers.Add(newRoot);
            newRoot.Children = children;
            _buffers.Root = newRoot;
            foreach (var node in _buffers.Nodes)
            {
                if(node.NumTriangles>0)
                    node.ClearChildren();
            }
        }

        private void Recursion(SceneNode node, Matrix4x4 parentTransform, List<SceneNode> nodes)
        {
            var transform = Matrix4x4.Multiply(node.ObjectToWorld, parentTransform);
            if (node.NumTriangles > 0)
                node.ForceOdd = true;
            foreach (var child in node.Children)
            {
                if (child.NumTriangles > 0)
                {
                    var add = new SceneNode
                    {
                        Name = "Inst-Single",
                        ObjectToWorld = transform,
                        ForceEven = true
                    };
                    nodes.Add(add);
                    _buffers.Add(add);
                    add.AddChild(child);
                }
                if (child.NumChildren > 0)
                {
                    Recursion(child, transform, nodes);
                }
            }
        }
    }
}