libblock lib_math {

  vec3 rcp(vec3 inp) {
    return vec3(1.0 / inp.x, 1.0 / inp.y, 1.0 / inp.z);
  }
  float saturate(float inp) {
    return clamp(inp,0,1);
  }
  vec3 saturate(vec3 inp) {
    return clamp(inp,0,1);
  }

  uint bitReverse(uint x) {
    x = ((x & 0x55555555) << 1) | ((x & 0xaaaaaaaa) >> 1);
    x = ((x & 0x33333333) << 2) | ((x & 0xcccccccc) >> 2);
    x = ((x & 0x0f0f0f0f) << 4) | ((x & 0xf0f0f0f0) >> 4);
    x = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);
    x = ((x & 0x0000ffff) << 16) | ((x & 0xffff0000) >> 16);
    return x;
  }

  vec2 hammersley(uint index, uint sampleCount) {
    // return point from hammersly point set (on hemisphere)
    float u = float(index) / float(sampleCount);
    float v = float(bitReverse(index)) * 0.00000000023283064365386963;
    return vec2(u, v);
  }

  vec3 hemisphereSample_uniform(float u, float v) {
    float phi      = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  vec3 hemisphereSample_cos(float u, float v) {
    float phi      = v * 2.0 * PI;
    float cosTheta = sqrt(1.0 - u);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  vec3 sphericalToNormal(float phi, float theta) {
    float sintheta = sin(theta);
    float costheta = cos(theta);
    return vec3(sintheta * cos(phi), sintheta * sin(phi), costheta);
  }
  vec2 normalToSpherical(vec3 n) {
    float phi = atan(n.y,n.x);
    float theta = acos(n.z);
    return vec2(phi,theta);
  }
}
