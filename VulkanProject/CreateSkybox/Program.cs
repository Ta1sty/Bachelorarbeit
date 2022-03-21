// See https://aka.ms/new-console-template for more information

using System.Drawing;


Console.WriteLine("Hello, World!");
if(File.Exists("skybox.cubetex"))
    File.Delete("skybox.cubetex");

using var dstStr = File.OpenWrite("skybox.cubetex");



using var srcStr = File.Open("skybox.png", FileMode.Open);
using var image = Image.FromStream(srcStr);
using var bmp = new Bitmap(image);
var width = bmp.Width / 4;
var height = bmp.Height / 3;
var size = width * height * sizeof(uint);
dstStr.Write(BitConverter.GetBytes(width));
dstStr.Write(BitConverter.GetBytes(height));

// left,middle,right,back,top,bottom
var offsetX = new []
{
    0, width, width * 2, width * 3, width, width
};
var offsetY = new[]
{
    height, height, height, height, 0, height*2
};
    
var order = new[] {2,0,4,5,1,3};

foreach (var num in order)
{
    var offX = offsetX[num];
    var offY = offsetY[num];

    var dst = new byte[size];
    var i = 0;
    for (var y = 0; y < height; y++)
    {
        for (var x = 0; x < width; x++)
        {
            var col = bmp.GetPixel(x + offX, y + offY);
            dst[i] = col.R;
            dst[i + 1] = col.G;
            dst[i + 2] = col.B;
            dst[i + 3] = col.A;
            i += 4;
        }
    }
    dstStr.Write(dst);
}