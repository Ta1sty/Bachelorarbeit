using System;
using System.Numerics;
using Scene;

namespace SceneCompiler.Scene
{
    public class GroundCreator
    {
        public static void AddGround(SceneBuffers buffers)
        {
            var root = buffers.Root;
            root.ComputeAABBs(buffers);

            var add = new SceneNode
            {
                NumTriangles = 2,
                IndexBufferIndex = buffers.IndexBuffer.Count,
                ForceOdd = true,
                Name = "Ground"
            };
            buffers.Add(add);

            var yMin = root.AABB_min.Y;

            var extent = root.AABB_max - root.AABB_min;
            var offset = root.AABB_min;

            var start = (uint)buffers.VertexBuffer.Count;

            float[] vf0 = { 0, 0, 0, 0, 1, 0, 0, 0 };
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
            var materialIndex = buffers.MaterialBuffer.Count;
            buffers.MaterialBuffer.Add(material);

            var v0 = new Vertex(0, vf0, materialIndex, (int)start);
            var v1 = new Vertex(0, vf1, materialIndex, (int)start);
            var v2 = new Vertex(0, vf2, materialIndex, (int)start);
            var v3 = new Vertex(0, vf3, materialIndex, (int)start);

            buffers.VertexBuffer.Add(v0);
            buffers.VertexBuffer.Add(v1);
            buffers.VertexBuffer.Add(v2);
            buffers.VertexBuffer.Add(v3);

            buffers.IndexBuffer.Add(start + 0);
            buffers.IndexBuffer.Add(start + 1);
            buffers.IndexBuffer.Add(start + 3);

            buffers.IndexBuffer.Add(start + 0);
            buffers.IndexBuffer.Add(start + 2);
            buffers.IndexBuffer.Add(start + 3);

            var scale = Matrix4x4.CreateScale(extent.X, 1, extent.Z);
            var trans = Matrix4x4.CreateTranslation(offset);

            add.ObjectToWorld = scale * trans;

            if (root.IsLodSelector)
                throw new Exception("Can not insert ground if ROOT is LOD selector");
            if (root.IsInstanceList)
            {
                var inst = new SceneNode
                {
                    ObjectToWorld = add.ObjectToWorld,
                    ForceEven = true,
                    Name = "Inst Ground"
                };
                add.ObjectToWorld = Matrix4x4.Identity;
                buffers.Add(inst);
                inst.AddChild(add);
                root.AddChild(inst);
            }
            else
            {
                root.AddChild(add);
            }
        }
    }
}