using System;
using System.Collections.Generic;
using System.Numerics;
using GLTFCompiler.GltfFileTypes;

namespace GLTFCompiler.Scene
{
    public class SceneNode
    {
        public int IndexBufferIndex = -1; // this index points to the first vertex-index in the index buffer
        public int NumTriangles = -1; // amount of triangles
        public Matrix4x4 Transform = Matrix4x4.Identity;
        public int NumChildren = 0;
        public List<SceneNode> Children = new();
        public SceneNode Brother = null; // this node is practically identical to this one, project others onto this one#
        public Node Source;
        public int Index = -1;
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

            if (!MatrixAlmostZero(node.Transform - ths.Transform)) return false;

            using var itLeft = ths.Children.GetEnumerator();
            using var itRight = node.Children.GetEnumerator();
            while (itLeft.MoveNext() && itRight.MoveNext())
            {
                if (itLeft.Current.ThisOrBrother() != itRight.Current.ThisOrBrother()) return false;
            }
            return true;
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
    }
}