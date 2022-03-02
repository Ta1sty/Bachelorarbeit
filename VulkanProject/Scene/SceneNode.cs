using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Threading.Tasks;

namespace Scene
{
    public class SceneNode
    {
        public static readonly int Size = 64;
        internal SceneBuffers Buffers; // the associated buffer
        public IEnumerable<SceneNode> Children { 
            get => Buffers.GetChildren(this);
            set => Buffers.SetChildren(this, value);
        }
        public IEnumerable<SceneNode> Parents {
            get => Buffers.GetParents(this);
            set => Buffers.SetParents(this, value);
        }
        public SceneNode Brother = null; // this node is practically identical to this one, project others onto this one#

        public string Name { get; set; }

        internal int FirstChild = -1;
        internal int FirstParent = -1;

        public Matrix4x4 ObjectToWorld = Matrix4x4.Identity;
        public Vector3 AABB_min = new(float.MaxValue, float.MaxValue, float.MaxValue);
        internal int Index = -1; // only visible from this namespace
        internal int NewIndex = -1; // use for reordering operations
        public Vector3 AABB_max = new(-float.MaxValue, -float.MaxValue, -float.MaxValue);
        public int Level = -1;
        public int NumTriangles = 0;
        public int IndexBufferIndex = -1;
        public int NumChildren => Buffers.NumChildren(this);
        public int NumParents => Buffers.NumParents(this);
        public int ChildrenIndex = -1;
        public int TlasNumber = -1;
        public bool IsInstanceList = false;
        public bool IsLodSelector = false;
        public uint TransformIndex = 0;

        public bool ForceOdd = false;
        public bool ForceEven = false;
        public long TotalPrimitiveCount = 0;
        public bool isAABBComputed = false;

        public bool NeedsTlas
        {
            get
            {
                if (Level % 2 != 0)
                    return false;
                var parent = Parents.FirstOrDefault();
                var grandparent = parent?.Parents.FirstOrDefault();
                if (parent is { IsInstanceList: true } || grandparent is { IsInstanceList: true})
                    return false;
                if (IsLodSelector)
                    return false;
                return true;
            }
        }

        public bool NeedsBlas
        {
            get
            {
                if (Level % 2 != 1)
                    return false;
                var parent = Parents.FirstOrDefault();
                var grandparent = parent?.Parents.FirstOrDefault();
                if (parent is { IsInstanceList: true } || grandparent is { IsInstanceList: true })
                    return false;
                if (parent is { IsLodSelector: true })
                    return false;
                return true;
            }
        }

        public SceneNode ThisOrBrother()
        {
            return Brother ?? this;
        }
        public void ResetAABB()
        {
            isAABBComputed = false;
            AABB_min = new(float.MaxValue, float.MaxValue, float.MaxValue);
            AABB_max = new(-float.MaxValue, -float.MaxValue, -float.MaxValue);
        }

        public bool Simmilar(SceneNode node)
        {
            var ths = ThisOrBrother();
            if (ths.IndexBufferIndex != node.IndexBufferIndex) return false;
            if (ths.NumTriangles != node.NumTriangles) return false;
            if (ths.NumChildren != node.NumChildren) return false;

            if (!MatrixAlmostZero(node.ObjectToWorld - ths.ObjectToWorld)) return false;

            using var itLeft = ths.Children.GetEnumerator();
            using var itRight = node.Children.GetEnumerator();
            while (itLeft.MoveNext() && itRight.MoveNext())
            {
                if (itLeft.Current.ThisOrBrother() != itRight.Current.ThisOrBrother()) return false;
            }
            return true;
        }

        public int WriteToByteArray(byte[] nodeBuffer, int pos)
        {

            // Vec4 AABB Max, align 16 bytes
            BitConverter.GetBytes(AABB_min.X).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_min.Y).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_min.Z).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(Index).CopyTo(nodeBuffer.AsSpan(pos += 4));

            // Vec4 AABB Max
            BitConverter.GetBytes(AABB_max.X).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_max.Y).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_max.Z).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(Level).CopyTo(nodeBuffer.AsSpan(pos += 4));

            // THE REST
            BitConverter.GetBytes(NumTriangles).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(IndexBufferIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
            
            BitConverter.GetBytes(NumChildren).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ChildrenIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
            
            BitConverter.GetBytes(TlasNumber).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes((uint) (IsInstanceList ? 1 : 0)).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes((uint) (IsLodSelector ? 1: 0)).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(TransformIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
            return pos;
        }

        public static bool MatrixAlmostZero(Matrix4x4 mat)
        {
            return Zero(mat.M11) && Zero(mat.M12) && Zero(mat.M13) && Zero(mat.M14) &&
                   Zero(mat.M21) && Zero(mat.M22) && Zero(mat.M23) && Zero(mat.M24) &&
                   Zero(mat.M31) && Zero(mat.M32) && Zero(mat.M33) && Zero(mat.M34) &&
                   Zero(mat.M41) && Zero(mat.M42) && Zero(mat.M43) && Zero(mat.M44);
        }

        private static bool Zero(float t)
        {
            return Math.Abs(t) < 1.0e-4;
        }

        public override string ToString()
        {
            var ret = "";
            if (NeedsTlas)
                ret += "TLAS\t";
            else if (NeedsBlas)
                ret += "BLAS\t";
            else
                ret += "NONE\t";
            ret += "I:" + Index + "\t";
            ret += "P:" + Parents.Count() + "\t";
            ret += "C:" + NumChildren + "\t";
            ret += "T:" + NumTriangles + "\t";
            for (int i = 0; i < Level; i++)
            {
                ret += "  ";
            }
            ret += "L:" + Level + " ";

            if (Name != null)
                ret += Name + " ";

            if (IsInstanceList)
                ret += "InstanceList: " + Children.First().Children.Count();

            return ret;
        }

        public void ComputeAABBs(SceneBuffers buffers)
        {
            if (isAABBComputed)
                return;
            if (!Children.Any() && NumTriangles == 0)
                return;
            if (Children.Skip(10000).Any())
            {
                Parallel.ForEach(Children, child => child.ComputeAABBs(buffers));
            }
            else
            {
                foreach (var child in Children)
                    child.ComputeAABBs(buffers);
            }

            foreach (var child in Children) // children
            {
                if (!child.isAABBComputed)
                    continue;
                var (min, max) = TransformAABB(ObjectToWorld, child.AABB_min, child.AABB_max);
                AABB_min = Min(AABB_min, min);
                AABB_max = Max(AABB_max, max);
            }

            for (var i = 0; i < NumTriangles; i++)
            {
                var index = IndexBufferIndex + i * 3;
                for (var j = 0; j < 3; j++)
                {
                    var v = buffers.VertexBuffer[(int)buffers.IndexBuffer[index + j]];
                    var pos = v.Position();
                    var posTr = Vector3.Transform(pos, ObjectToWorld);
                    AABB_min = Min(AABB_min, posTr);
                    AABB_max = Max(AABB_max, posTr);
                }
            }
            isAABBComputed = true;
        }

        private (Vector3 min, Vector3 max) TransformAABB(Matrix4x4 transform, Vector3 min, Vector3 max)
        {
            Vector3[] aabbVertices ={
                new (min.X,min.Y,min.Z),
                new (min.X,min.Y,max.Z),
                new (min.X,max.Y,min.Z),
                new (min.X,max.Y,max.Z),
                new (max.X,min.Y,min.Z),
                new (max.X,min.Y,max.Z),
                new (max.X,max.Y,min.Z),
                new (max.X,max.Y,max.Z)
            };

            min = new(float.MaxValue, float.MaxValue, float.MaxValue);
            max = new(-float.MaxValue, -float.MaxValue, -float.MaxValue);

            foreach (var vert in aabbVertices)
            {
                var vec = Vector3.Transform(vert, transform);
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

        private Vector3 Max(Vector3 a, Vector3 b)
        {
            return new Vector3(
                Math.Max(a.X, b.X),
                Math.Max(a.Y, b.Y),
                Math.Max(a.Z, b.Z)
                );
        }
        private Vector3 Min(Vector3 a, Vector3 b)
        {
            return new Vector3(
                Math.Min(a.X, b.X),
                Math.Min(a.Y, b.Y),
                Math.Min(a.Z, b.Z)
                );
        }
    }
}