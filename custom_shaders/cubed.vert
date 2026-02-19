VARYING vec3 pos;

float snap1D(float v, float s) {
    return floor(v / s + 0.5) * s;
}

vec3 snap3D(vec3 p, vec3 cell) {
    return vec3(
        snap1D(p.x, cell.x),
        snap1D(p.y, cell.y),
        snap1D(p.z, cell.z)
    );
}

void MAIN()
{
    pos = VERTEX;

    // voxel size (adjust to your likings)
    const vec3 cell = vec3(8,8,8);

    // Fix the senoidal by column to help keep faces plain per voxel
    float colX = snap1D(pos.x, cell.x);
    float wave = sin(colX * 4.0 + uTime / 10.0) * 2.1;
    
    vec3 displaced = vec3(pos.x, pos.y, pos.z);
    vec3 voxelPos  = snap3D(displaced, cell);

    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(voxelPos, 1.08);
}
