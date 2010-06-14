#include "renderer.h"
#include "surfaces.h"
#include "util.h"

static GeometryPtr createPatch(const float P[12], const float Cs[3],
                               const Mat4& trans = Mat4())
{
    PrimvarStorageBuilder builder;
    builder.add(Primvar::P, P, 12);
    builder.add(PrimvarSpec(PrimvarSpec::Constant, VarSpec::Color, 1,
                            ustring("Cs")), Cs, 3);
    IclassStorage storReq(1,4,4,4,4);
    GeometryPtr patch(new Patch(builder.build(storReq)));
    patch->transform(trans);
    return patch;
}


void renderDofAmountTest()
{
    Options opts;
    opts.xRes = 320;
    opts.yRes = 240;
    opts.gridSize = 8;
    opts.clipNear = 0.1;
    opts.superSamp = Imath::V2i(10,10);
    opts.pixelFilter = makeGaussianFilter(Vec2(2.0,2.0));
    opts.fstop = 100;
    opts.focalLength = 20;
    opts.focalDistance = 3;

    Attributes attrs;
    attrs.shadingRate = 1;
    attrs.smoothShading = true;

    Mat4 camToScreen =
        perspectiveProjection(90, opts.clipNear, opts.clipFar) *
        screenWindow(-2.33333, 0.33333, -1, 1);

    // Output variables.
    VarList outVars;
    outVars.push_back(Stdvar::Cs);

    Renderer r(opts, camToScreen, outVars);

    Mat4 wToO = Mat4().setTranslation(Vec3(-0.5,-0.5,-1)) *
                Mat4().setAxisAngle(Vec3(0,0,1), deg2rad(45)) *
                Mat4().setTranslation(Vec3(-1.5, 0, 2));

    const float P[12] = {0, 0, 0,  1, 0, 0,  0, 1, 0,  1, 1, 0};
    {
        const float Cs[3] = {1, 0.7, 0.7};
        r.add(createPatch(P, Cs, wToO), attrs);
    }
    {
        const float Cs[3] = {0.7, 1, 0.7};
        r.add(createPatch(P, Cs, Mat4().setTranslation(Vec3(0,0,1))*wToO), attrs);
    }
    {
        const float Cs[3] = {0.7, 0.7, 1};
        r.add(createPatch(P, Cs, Mat4().setTranslation(Vec3(0,0,2))*wToO), attrs);
    }
    {
        const float Cs[3] = {1, 0.7, 0.7};
        r.add(createPatch(P, Cs, Mat4().setTranslation(Vec3(0,0,5))*wToO), attrs);
    }
    {
        const float Cs[3] = {0.7, 1, 0.7};
        r.add(createPatch(P, Cs, Mat4().setTranslation(Vec3(0,0,25))*wToO), attrs);
    }

    r.render();
}
