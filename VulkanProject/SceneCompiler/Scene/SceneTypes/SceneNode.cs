using System;
using System.Collections.Generic;
using System.Numerics;
using SceneCompiler.GLTFConversion.GltfFileTypes;

namespace SceneCompiler.Scene.SceneTypes
{
    public class SceneNode
    {
        public List<SceneNode> Children = new();
        public SceneNode Brother = null; // this node is practically identical to this one, project others onto this one#
        public Node Source;

        private string _name;

        public string Name
        {
            set => _name = value;
            get => Source?.Name ?? _name;
        }


        public Matrix4x4 ObjectToWorld = Matrix4x4.Identity;
        public Matrix4x4 WorldToObject = Matrix4x4.Identity;
        public Vector4 AABB_min = new(float.MaxValue, float.MaxValue, float.MaxValue, 1); 
        public Vector4 AABB_max = new(-float.MaxValue, -float.MaxValue, -float.MaxValue, 1);
        public int IndexBufferIndex = -1;
        public int NumTriangles = 0;
        public int NumChildren = 0;
        public int ChildrenIndex = -1;
        public int Index = -1;
        public int Level = -1;
        public uint NumEven = 0;
        public uint NumOdd = 0;
        public int TlasNumber = -1;
        public float pad1 = -1;
        public float pad2 = -1;
        public float pad3 = -1;

        public SceneNode ThisOrBrother()
        {
            return Brother ?? this;
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
            // OBJECT TO WORLD
            BitConverter.GetBytes(ObjectToWorld.M11).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M21).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M31).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M41).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(ObjectToWorld.M12).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M22).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M32).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M42).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(ObjectToWorld.M13).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M23).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M33).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M43).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(ObjectToWorld.M14).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M24).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M34).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ObjectToWorld.M44).CopyTo(nodeBuffer.AsSpan(pos += 4));

            // WORLD TO OBJECT
            BitConverter.GetBytes(WorldToObject.M11).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M21).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M31).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M41).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(WorldToObject.M12).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M22).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M32).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M42).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(WorldToObject.M13).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M23).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M33).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M43).CopyTo(nodeBuffer.AsSpan(pos += 4));

            BitConverter.GetBytes(WorldToObject.M14).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M24).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M34).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(WorldToObject.M44).CopyTo(nodeBuffer.AsSpan(pos += 4));

            // Vec4 AABB Max, align 16 bytes
            BitConverter.GetBytes(AABB_min.X).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_min.Y).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_min.Z).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_min.W).CopyTo(nodeBuffer.AsSpan(pos += 4));

            // Vec4 AABB Max
            BitConverter.GetBytes(AABB_max.X).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_max.Y).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_max.Z).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(AABB_max.W).CopyTo(nodeBuffer.AsSpan(pos += 4));

            // THE REST
            BitConverter.GetBytes(IndexBufferIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(NumTriangles).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(NumChildren).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(ChildrenIndex).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(Index).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(Level).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(NumEven).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(NumOdd).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(TlasNumber).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(pad1).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(pad2).CopyTo(nodeBuffer.AsSpan(pos += 4));
            BitConverter.GetBytes(pad3).CopyTo(nodeBuffer.AsSpan(pos += 4));
            return pos;
        }

        private static bool MatrixAlmostZero(Matrix4x4 mat)
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
            ret += "I:" + Index + "\t";
            for (int i = 0; i < Level; i++)
            {
                ret += "  ";
            }
            ret += "L:" + Level + " ";
            ret += "C:" + NumChildren + "\t";
            ret += "T:" + NumTriangles + "\t";

            if (Name != null)
                ret += Name + " ";

            return ret;
        }
    }
}