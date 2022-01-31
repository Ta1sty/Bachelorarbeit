using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Threading.Tasks;
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
            Console.WriteLine("Adjusting Scene Graph");
            var buffer = _buffers.Nodes;
            SceneNode root = buffer[_buffers.RootNode];
            root.Level = 0;

            Console.WriteLine("Writing Levels");
            DepthRecursion(root);
            Console.WriteLine("Number of Triangles: " + root.TotalPrimitiveCount);
            Console.WriteLine("Removing empty children");
            Parallel.ForEach(buffer, x => x.Children = x.Children.Where(x => x.TotalPrimitiveCount > 0).ToList());

            ConcurrentBag<SceneNode> blasAdd = new();
            ConcurrentBag<SceneNode> tlasAdd = new();
            Console.WriteLine("inserting dummys");
            var evenTask = Task.Run(() =>
            {
                foreach (var node in buffer.Where(x => x.Level % 2 == 0))
                    AdjustEvenNode(node, blasAdd);
            });

            var oddTask = Task.Run(() =>
            {
                foreach (var node in buffer.Where(x => x.Level % 2 == 1))
                    AdjustOddNode(node, tlasAdd);
            });

            evenTask.Wait();
            oddTask.Wait();

            buffer.AddRange(tlasAdd);
            buffer.AddRange(blasAdd);
            Console.WriteLine("added " + (tlasAdd.Count +blasAdd.Count) + " dummies to the scenegraph");

            for (var i = 0; i < buffer.Count; i++)
            {
                buffer[i].Index = i;
            }
            Console.WriteLine("computing AABBs");
            ComputeAABBs(root);
        }

        public void AdjustEvenNode(SceneNode node, ConcurrentBag<SceneNode> blasAdd)
        {
            List<SceneNode> evenChildren = new();
            List<SceneNode> oddChildren = new();
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

            if (node.NumTriangles > 0 || evenChildren.Count > 0)
            {
                var dummy = new SceneNode
                {
                    // set new Level and index
                    Level = node.Level + 1,
                    Name = "DummyBLAS",
                    // add the geometry
                    IndexBufferIndex = node.IndexBufferIndex,
                    NumTriangles = node.NumTriangles,
                    // add the even children
                    NumChildren = evenChildren.Count,
                    NumEven = (uint)evenChildren.Count,
                    Children = new List<SceneNode>(evenChildren),
                    // set transforms to identity
                    ObjectToWorld = Matrix4x4.Identity,
                    WorldToObject = Matrix4x4.Identity,
                };
                // add odd dummy to buffer and to children of the even node
                blasAdd.Add(dummy);
                oddChildren.Add(dummy);
                // removed geometry
                node.IndexBufferIndex = -1;
                node.NumTriangles = 0;

                // add only the odd children
                node.NumOdd = (uint)oddChildren.Count;
                node.Children.Clear();
                node.Children.AddRange(oddChildren);
                node.NumChildren = oddChildren.Count;
            }
            else // only odd geometry
            {
                // leave everything, set only the odd count, since even is 0
                node.NumOdd = (uint)oddChildren.Count;
            }
        }

        public void AdjustOddNode(SceneNode node, ConcurrentBag<SceneNode> tlasAdd)
        {
            List<SceneNode> evenChildren = new();
            List<SceneNode> oddChildren = new();
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

            if (oddChildren.Count > 0)
            {
                // we solve this by building a new tlas for this reference
                // but this can happen multiple times for the same blas
                // so if possible we want to reuse the tlas
                var dummy = tlasAdd.Where(x =>
                {
                    if (x.Children.Count != oddChildren.Count)
                        return false;
                    foreach (var child in node.Children)
                    {
                        if (!x.Children.Contains(child))
                            return false;
                    }
                    foreach(var child in x.Children)
                    {
                        if(!oddChildren.Contains(child))
                            return false;
                    }

                    return true;
                }).FirstOrDefault();

                if (dummy == null)
                {
                    dummy = new SceneNode
                    {
                        // set new Level and index
                        Level = node.Level + 1,
                        Name = "DummyTLAS",
                        // add the odd children
                        NumChildren = oddChildren.Count,
                        NumOdd = (uint)oddChildren.Count,
                        Children = new List<SceneNode>(oddChildren),
                        // set transforms to identity
                        ObjectToWorld = Matrix4x4.Identity,
                        WorldToObject = Matrix4x4.Identity,
                    };
                    tlasAdd.Add(dummy);
                }

                // add even dummy to buffer and children of the odd node
                evenChildren.Add(dummy);

                // add only the even children
                node.NumEven = (uint)evenChildren.Count;
                node.Children.Clear();
                node.Children.AddRange(evenChildren);
                node.NumChildren = evenChildren.Count;
            }
            else
            {
                // we only want and only have even children, therefore set numEven
                node.NumEven = (uint)evenChildren.Count;
            }
        }

        public void Validate()
        {
            Console.WriteLine("Validating SceneGraph");
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
                   if (child.NumChildren != child.NumEven + child.NumOdd)
                       throw new Exception("Num odd and num even don't add up to numChildren");
               }

               if (node.IsInstanceList)
               {
                   if (node.Level % 2 == 1)
                       throw new Exception("instance List must have even level");
                   if (node.NumTriangles > 0)
                       throw new Exception("instance List can not contain geometry");

                   if (node.Children.Count != 1)
                       throw new Exception("instance List must have exactly one child (DummyBLAS)");

                   var dummyBLAS = node.Children[0];

                   if (dummyBLAS.Name != "DummyBLAS")
                       throw new Exception("child of instancle List is not a dummyTLAS");

                   foreach (var child in dummyBLAS.Children)
                   {
                       if (child.Level % 2 == 1)
                           throw new Exception("Instance list elements must have an even level");
                       if (child.NumTriangles > 0)
                           throw new Exception("Instance list elements can not reference triangles");
                       foreach (var parent in child.Parents)
                       {
                           if (parent.Name != "DummyBLAS")
                               throw new Exception("Parent of instance list element must be DummyBLAS");
                           foreach (var grandParent in parent.Parents)
                           {
                               if (!grandParent.IsInstanceList)
                                   throw new Exception("Grandparent of instance list element must be instance list");
                           }
                       }
                   }
               }

               if (node.Index < 0)
                   throw new Exception("Index was not set");
               if (_buffers.Nodes[node.Index] != node)
                   throw new Exception("Index doesn't match actual index");
               if (node.Level % 2 == 0 && node.NumTriangles > 0)
                   throw new Exception("Even node references Geometry");
               if (node.NumChildren == 0 && node.NumTriangles == 0)
                   throw new Exception("Node doesn't reference anything");
               if (node.Children.Count != node.NumChildren)
                   throw new Exception("Node NumChildren count differs from actual count");
               if (node.NumTriangles > 0 && node.IndexBufferIndex < 0)
                   throw new Exception("Node says it contains geometry but index buffer pointer is not set");
               if (node.Brother != null)
                   throw new Exception("An identical brother exists, this node was supposed to be removed");
           });
        }

        public void PrintScene()
        {
            foreach (var node in _buffers.Nodes)
            {
                if (node.Name.Contains("Dummy") && node.Parents[0].IsInstanceList)
                    continue;
                var str = node.ToString();
                if (str.Contains("Mesh"))
                    continue;
                if (str.Contains("Inst") && !str.Contains("List"))
                    continue;
                Console.WriteLine(str);
            }
        }

        public void RebuildNodeBufferFromRoot()
        {
            Console.WriteLine("Rebuilding the buffer from the root");
            var root = _buffers.Nodes[_buffers.RootNode];
            Parallel.ForEach(_buffers.Nodes, t =>
            {
                t.Index = -1;
                t.Parents.Clear();
            });
            root.Index = 0;
            BuildListRecursive(root, 1);

            var arr = new SceneNode[_buffers.Nodes.Count(x=>x.Index != -1)];
            var unused = 0;
            foreach (var node in _buffers.Nodes)
            {
                if(node.Index == -1)
                {
                    unused++;
                    continue;
                }
                arr[node.Index] = node;
            }

            Console.WriteLine("Removed " + unused + " unused Nodes");

            _buffers.RootNode = 0;
            _buffers.Nodes.Clear();
            _buffers.Nodes.AddRange(arr);

            foreach (var node in _buffers.Nodes)
            {
                foreach (var child in node.Children)
                {
                    child.Parents.Add(node);
                }
            }
        }
        // use DFS Layout since this also the way traversal is done, should improve caching
        private int BuildListRecursive(SceneNode node, int indexOffset)
        {
            if (!node.Children.Any())
                return indexOffset;

            int start = indexOffset;

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

        private void DepthRecursion(SceneNode node)
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
                }
                DepthRecursion(child);
            }
            node.TotalPrimitiveCount = node.NumTriangles + node.Children.Sum(x => x.TotalPrimitiveCount);
        }

        public void ComputeAABBs(SceneNode node)
        {
            if (node.isAABBComputed)
                return;
            if(node.Children.Count > 10000)
            {
                Parallel.ForEach(node.Children, child => ComputeAABBs(child));
            }
            else
            {
                foreach (var child in node.Children)
                    ComputeAABBs(child);
            }

            foreach (var child in node.Children) // children
            {
                var (min, max) = TransformAABB(node.ObjectToWorld, child.AABB_min, child.AABB_max);
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
                    var posTr = Vector4.Transform(pos, node.ObjectToWorld);
                    node.AABB_min = Min(node.AABB_min, posTr);
                    node.AABB_max = Max(node.AABB_max, posTr);
                }
            }
            node.isAABBComputed = true;
        }

        private (Vector4 min, Vector4 max) TransformAABB(Matrix4x4 transform, Vector4 min, Vector4 max)
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
            /*var middle = (min + max) / 2;
            var extent = max - min;
            min = middle - (extent / 2) * 1.05f;
            max = middle + (extent / 2) * 1.05f;
            */
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
                Math.Min(a.X, b.X),
                Math.Min(a.Y, b.Y),
                Math.Min(a.Z, b.Z),
                1);
        }


        private void SingleLevel()
        {
            // collapse parent nodes
        }
    }
}