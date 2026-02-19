VARYING vec3 pos;
VARYING vec2 vPhaseData;

// ===== util =====
float snap1D(float v, float s) { return floor(v / s + 0.5) * s; }

// --------- HEX (axial/cube coords) ---------
vec2 cart_to_axial_pointy(vec2 p, float size) {
    // q = (√3/3 * x - 1/3 * z)/size
    // r = (2/3 * z)/size
    float q = (1.7320508075688772 * 0.3333333333333 * p.x - 0.3333333333333 * p.y) / size;
    float r = (0.6666666666667 * p.y) / size;
    return vec2(q, r);
}

vec2 axial_to_cart_pointy(vec2 a, float size) {
    // x = size * (√3*q + √3/2*r)
    // z = size * (3/2*r)
    float x = size * (1.7320508075688772 * a.x + 0.8660254037844386 * a.y);
    float z = size * (1.5 * a.y);
    return vec2(x, z);
}

vec3 cube_round(vec3 c) {
    vec3 rc = floor(c + 0.5);
    vec3 diff = abs(rc - c);
    if (diff.x > diff.y && diff.x > diff.z) {
        rc.x = -rc.y - rc.z;
    } else if (diff.y > diff.z) {
        rc.y = -rc.x - rc.z;
    } else {
        rc.z = -rc.x - rc.y;
    }
    return rc;
}

vec2 hex_snap_pointy(vec2 xz, float size) {
    vec2 a  = cart_to_axial_pointy(xz, size);
    // axial -> cube
    vec3 c  = vec3(a.x, -a.x - a.y, a.y);
    vec3 cr = cube_round(c);
    vec2 ar = vec2(cr.x, cr.z); // cube -> axial
    return axial_to_cart_pointy(ar, size);
}

// (Optional for "flat-top")
//
// vec2 cart_to_axial_flattop(vec2 p, float size) {
//     float q = (0.6666666666667 * p.x) / size;
//     float r = (-0.3333333333333 * p.x + 0.5773502691896258 * p.y) / size; // √3/3 = 0.57735...
//     return vec2(q, r);
// }
// vec2 axial_to_cart_flattop(vec2 a, float size) {
//     float x = size * (1.5 * a.x);
//     float z = size * (0.8660254037844386 * a.x + 1.7320508075688772 * a.y); // √3/2, √3
//     return vec2(x, z);
// }
// replace hex_snap_pointy for the "flattop" version.

void MAIN()
{
    pos = VERTEX;

    const float HEX_SIZE = 6.0;   // hex radius
    const float STEP_Y   = 8.0;   // Y quantization

    // Hex snap on XZ plane
    vec2 xz = vec2(pos.x, pos.z);
    vec2 hxz = hex_snap_pointy(xz, HEX_SIZE);

    // Quantize Y (optional, change to pos.y if you want continuous Y)
    float yq = snap1D(pos.y, STEP_Y);

    vec3 voxelPos = vec3(hxz.x, yq, hxz.y);

    vPhaseData = vec2(voxelPos.y, voxelPos.z);

    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(voxelPos, 1.12);
}
