#include "DPProctext.h"

#define PALE_BLUE        color (0.25, 0.25, 0.35)
#define MEDIUM_BLUE      color (0.10, 0.10, 0.30)
#define DARK_BLUE        color (0.05, 0.05, 0.26)
#define DARKER_BLUE      color (0.03, 0.03, 0.20)
#define NNOISE           4

color
marble_color(float m)
{
    return color spline(
        clamp(2*m + .75, 0, 1),
        PALE_BLUE, PALE_BLUE,
        MEDIUM_BLUE, MEDIUM_BLUE, MEDIUM_BLUE,
        PALE_BLUE, PALE_BLUE,
        DARK_BLUE, DARK_BLUE,
        DARKER_BLUE, DARKER_BLUE,
        PALE_BLUE, DARKER_BLUE);
}

surface
SpaceTest(
    uniform float Ka = 1;
    uniform float Kd = 0.8;
    uniform float Ks = 0.2;
    uniform float texturescale = 2.5;
    uniform float roughness = 0.1;
	uniform string space = "shader";
     )
{
    color Ct;
    point NN;
    point PP;
    float i, f, marble;

    NN = normalize(faceforward(N, I));
    PP = transform(space, P) * texturescale;

    marble = 0; f = 1;
    for (i = 0; i < NNOISE; i += 1) {
        marble += snoise(PP * f)/f;
        f *= 2.17;
    }
    Ct = marble_color(marble);

    Ci = Os * (Ct * (Ka * ambient() + Kd * diffuse(NN))
        + Ks * specular(NN, normalize(-I), roughness));
}
