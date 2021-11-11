using System;
using GLTFCompiler.GltfFileTypes;

namespace GLTFCompiler.Scene
{
    public class BufferReader
    {
        private readonly byte[] _buffer;
        private int _currentPos;
        private readonly int _length;
        public BufferReader(SceneData scene, Accessor acc)
        {
            BufferView view = scene.file.BufferViews[acc.BufferView];
            _currentPos = view.ByteOffset;
            _buffer = scene.Buffers[view.Buffer];
            _length = view.ByteLength;
        }
        public BufferReader(SceneData scene, BufferView view)
        {
            _currentPos = view.ByteOffset;
            _buffer = scene.Buffers[view.Buffer];
            _length = view.ByteLength;
        }
        public byte[] GetAllBytes()
        {
            byte[] res = new byte[_length];
            _buffer.AsSpan(_currentPos, _length).CopyTo(res.AsSpan());
            return res;
        }
        public float[] GetVec3()
        {
            float x = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
            _currentPos += 4;
            float y = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
            _currentPos += 4;
            float z = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
            _currentPos += 4;
            return new[] { x, y, z };
        }
        public float[] GetVec2()
        {
            float x = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
            _currentPos += 4;
            float y = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
            _currentPos += 4;
            return new[] { x, y };
        }
        public float[] GetFloat()
        {
            float x = BitConverter.ToSingle(_buffer.AsSpan(_currentPos, 4));
            _currentPos += 4;
            return new[] { x };
        }
        public int GetInt()
        {
            int x = BitConverter.ToInt32(_buffer.AsSpan(_currentPos, 4));
            _currentPos += 4;
            return x;
        }

        public ushort GetUshort()
        {
            ushort x = BitConverter.ToUInt16(_buffer.AsSpan(_currentPos, 2));
            _currentPos += 2;
            return x;
        }
    }
}