#include <string>
#include <map>
#include <iostream>
#include "Ptexture.h"
#include <cstdlib>
#include <cstdio> // printf()
using namespace Ptex;

typedef struct faceValue
{
    uint32_t value[2][2]; // u v
} FaceValue;

FaceValue DumpData(Ptex::Res res, Ptex::DataType dt, int nchan, void* data, std::string prefix)
{
    float* pixel = (float*)malloc(sizeof(float) * nchan);
    uint8_t* cpixel = (uint8_t*)malloc(sizeof(uint8_t) * nchan);
    printf("%sdata (%d x %d):\n", prefix.c_str(), res.u(), res.v());
    int ures = res.u(), vres = res.v();
    int pixelSize = Ptex::DataSize(dt) * nchan;

    uint8_t* dpixel;

    FaceValue face;
    {
        int u = 0, v = 0;
        dpixel = (uint8_t*)data + (v * ures + u) * pixelSize;
        Ptex::ConvertToFloat(pixel, dpixel, dt, nchan);
        Ptex::ConvertFromFloat(cpixel, pixel, Ptex::dt_uint8, nchan);
        face.value[0][0] = (cpixel[0]<<16) + (cpixel[1] << 8) + (cpixel[2]);
    }

    {
        int u = 0, v = vres-1;
        dpixel = (uint8_t*)data + (v * ures + u) * pixelSize;
        Ptex::ConvertToFloat(pixel, dpixel, dt, nchan);
        Ptex::ConvertFromFloat(cpixel, pixel, Ptex::dt_uint8, nchan);
        face.value[0][1] = (cpixel[0] << 16) + (cpixel[1] << 8) + (cpixel[2]);
    }

    {
        int u = ures-1, v = 0;
        dpixel = (uint8_t*)data + (v * ures + u) * pixelSize;
        Ptex::ConvertToFloat(pixel, dpixel, dt, nchan);
        Ptex::ConvertFromFloat(cpixel, pixel, Ptex::dt_uint8, nchan);
        face.value[1][0] = (cpixel[0] << 16) + (cpixel[1] << 8) + (cpixel[2]);
    }

    {
        int u = ures - 1, v = vres - 1;
        dpixel = (uint8_t*)data + (v * ures + u) * pixelSize;
        Ptex::ConvertToFloat(pixel, dpixel, dt, nchan);
        Ptex::ConvertFromFloat(cpixel, pixel, Ptex::dt_uint8, nchan);
        face.value[1][1] = (cpixel[0] << 16) + (cpixel[1] << 8) + (cpixel[2]);
    }

    free(cpixel);
    free(pixel);

    return face;
}

int mainRead(char* src, char* dst)
{
    FILE* file = fopen(dst, "w");

    Ptex::String error;
    PtexPtr<PtexCache> c(PtexCache::create(0, 0));
    PtexPtr<PtexTexture> r(c->get(src, error));

    if (!r) {
        std::cerr << error.c_str() << std::endl;
        return 1;
    }
    std::cout << "meshType: " << Ptex::MeshTypeName(r->meshType()) << std::endl;
    std::cout << "dataType: " << Ptex::DataTypeName(r->dataType()) << std::endl;
    std::cout << "numChannels: " << r->numChannels() << std::endl;
    std::cout << "alphaChannel: ";
    if (r->alphaChannel() == -1) std::cout << "(none)" << std::endl;
    else std::cout << r->alphaChannel() << std::endl;
    std::cout << "numFaces: " << r->numFaces() << std::endl;

    PtexMetaData* meta = r->getMetaData();
    std::cout << "numMetaKeys: " << meta->numKeys() << std::endl;
    //if (meta->numKeys()) DumpMetaData(meta);
    meta->release();

    uint32_t nfaces = r->numFaces();
    fwrite(&nfaces, sizeof(uint32_t), 1, file);
    for (int i = 0; i < nfaces; i++) {
        const Ptex::FaceInfo& f = r->getFaceInfo(i);

        Ptex::Res res = f.res;
        void* data = malloc(Ptex::DataSize(r->dataType()) * r->numChannels() * res.size());
        r->getData(i, data, 0, res);
        FaceValue face = DumpData(res, r->dataType(), r->numChannels(), data, "  ");
        fwrite(&face.value, sizeof(uint32_t), 4, file);
        free(data);
    }
    fclose(file);
    return 0;
}

int main(int argc, char** argv)
{
	//mainWrite();
    mainRead(argv[1], argv[2]);
}