libblock lib_brdf : lib_math {

  vec3 importanceSampleGGX(vec2 e, float roughness) {
    float rufsq         = roughness * roughness;
    float rufp4         = rufsq * rufsq;
    float phi           = PI2 * e.x;
    float cosThetaSqNum = (1.0 - e.y);
    float cosThetaSqDiv = (1.0 + (rufp4 - 1.0) * e.y);
    float cosThetaSq    = cosThetaSqNum / cosThetaSqDiv;
    float cosTheta      = sqrt(cosThetaSq);
    float sinTheta      = sqrt(1.0 - cosThetaSq);
    return sphericalToCartesian(phi, cosTheta, sinTheta);
  }

  float geometrySchlickGGX(vec3 normal, vec3 dir, float roughness) {
    float k       = roughness * roughness * 0.5;
    float numer   = saturate(dot(normal,dir));
    float divisor = numer * (1.0 - k) + k;
    return numer / divisor;
  }

  float geometrySmith(vec3 normal, vec3 viewdir, vec3 lightdir, float roughness) {
    return geometrySchlickGGX(normal, viewdir, roughness) * geometrySchlickGGX(normal, lightdir, roughness);
  }

  vec3 fresnelSchlick(vec3 normal, vec3 viewdir, vec3 F0) {
    float ndotv_sat = saturate(dot(normal,viewdir));
    return F0 + (vec3(1, 1, 1) - F0) * pow(1.0 - ndotv_sat, 5.0);
  }

  vec2 integrateGGX(float n_dot_v, float roughness) {
    int numsamples = 1024;
    n_dot_v = saturate(n_dot_v);
    vec3 v(sqrt(1.0 - n_dot_v * n_dot_v), 0, n_dot_v);
    float accum_scale = 0.0;
    float accum_bias  = 0.0;
    for (int i = 0; i < numsamples; i++) {
      vec2 e            = hammersley(i, numsamples);
      vec3 h            = importanceSampleGGX(e, roughness);
      float v_dot_h     = dot(v.h);
      vec3 l            = normalize((h * 2.0 * v_dot_h) - v);
      float n_dot_h_sat = saturate(h.z);
      float v_dot_h_sat = saturate(v_dot_h);
      if (l.z > 0.0) {
        float gsmith = geometrySmith(vec3(0, 0, 1), v, l, roughness);
        float gvis   = (gsmith * v_dot_h) / (n_dot_h_sat * n_dot_v);
        float fc     = pow(1.0 - v_dot_h_sat, 5.0);
        accum_scale += (1.0 - fc) * gvis;
        accum_bias += fc * gvis;
      }
    }
    return vec2(accum_scale / float(numsamples), accum_bias / float(numsamples));
  }

}
