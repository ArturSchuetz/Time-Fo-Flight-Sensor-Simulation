/* 
* Copyright (c) 2018 NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <optix.h>
#include <optix_math.h>
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

static __device__ __inline__ float3 schlick( float nDi, const float3& rgb )
{
float r = fresnel_schlick(nDi, 5, rgb.x, 1);
float g = fresnel_schlick(nDi, 5, rgb.y, 1);
float b = fresnel_schlick(nDi, 5, rgb.z, 1);
return make_float3(r, g, b);
}

struct PerRayData_radiance
{
float3 result;
float importance;
int depth;
};

struct PerRayData_shadow
{
float3 attenuation;
};

struct BasicLight
{
#if defined(__cplusplus)
typedef optix::float3 float3;
#endif
float3 pos;
float3 color;
int    casts_shadow; 
int    padding;      // make this structure 32 bytes -- powers of two are your friend!
};

using namespace optix;

rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable(float, t_hit, rtIntersectionDistance, );
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow, rtPayload, );

rtDeclareVariable(float3,       Ka, , );
rtDeclareVariable(float3,       Ks, , );
rtDeclareVariable(float,        dissolve, , );
rtDeclareVariable(float,        phong_exp, , );
rtDeclareVariable(float3, 		reflectivity_n, , );

rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable(float3, shading_tangent, attribute shading_tangent, ); 
rtDeclareVariable(float3, shading_bitangent, attribute shading_bitangent, ); 

rtDeclareVariable(int,               max_depth, , );
rtBuffer<BasicLight>                 lights;
rtDeclareVariable(float3,            ambient_light_color, , );
rtDeclareVariable(unsigned int,      radiance_ray_type, , );
rtDeclareVariable(unsigned int,      shadow_ray_type, , );
rtDeclareVariable(float,             scene_epsilon, , );
rtDeclareVariable(rtObject,          top_object, , );
rtDeclareVariable(rtObject,          top_shadower, , );

RT_PROGRAM void any_hit_shadow()
{
    if(dissolve < 1.0)
    {
        prd_shadow.attenuation = optix::make_float3(1.0f);
        rtIgnoreIntersection();
    }
    else
    {
        // this material is opaque, so it fully attenuates all shadow rays
        prd_shadow.attenuation = optix::make_float3(0.0f);
        rtTerminateRay();
    }
}

rtTextureSampler<float4, 2> Kd_map;
rtTextureSampler<float4, 2> Kr_map;
rtTextureSampler<float4, 2> Kn_map;
rtDeclareVariable(float3, texcoord, attribute texcoord, ); 

RT_PROGRAM void closest_hit_radiance()
{
    float3 world_geometric_normal	= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
    float3 world_shading_tangent	= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_tangent ) );
    float3 world_shading_bitangent	= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_bitangent ) );
    float3 world_shading_normal		= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );

    float3 fftangent = faceforward( world_shading_tangent, -ray.direction, world_geometric_normal );
    float3 ffbitangent = faceforward( world_shading_bitangent, -ray.direction, world_geometric_normal );
    float3 ffnormal = faceforward( world_shading_normal, -ray.direction, world_geometric_normal );

    Matrix<3, 3> invtangentMatrix;
    invtangentMatrix.setCol(0, fftangent);
    invtangentMatrix.setCol(1, ffbitangent);
    invtangentMatrix.setCol(2, ffnormal);

    const float3 Kd_val = make_float3( tex2D( Kd_map, texcoord.x, texcoord.y ) );
    const float3 Kr_val = make_float3( tex2D( Kr_map, texcoord.x, texcoord.y ) );
    const float3 Kn_val = invtangentMatrix * normalize(make_float3( tex2D( Kn_map, texcoord.x, texcoord.y ) ) * 2.0f - 1.0f);

    float3 hit_point = ray.origin + t_hit * ray.direction;

    // ambient contribution

    float3 result = Ka * ambient_light_color;

    // reflection ray
    if(dissolve < 1.0) 
    {
        // ray tree attenuation
        PerRayData_radiance new_prd;             
        new_prd.importance = prd_radiance.importance;
        new_prd.depth = prd_radiance.depth + 1;

        optix::Ray glass_ray = optix::make_Ray( hit_point, ray.direction, radiance_ray_type, scene_epsilon, RT_DEFAULT_MAX );
        rtTrace(top_object, glass_ray, new_prd);
        result = (result * dissolve) + (new_prd.result * (1.0 - dissolve));
    }

    // compute direct lighting
    unsigned int num_lights = lights.size();
    for(int i = 0; i < num_lights; ++i) 
    {
        BasicLight light = lights[i];
        float Ldist = optix::length(light.pos - hit_point);
        float3 L = optix::normalize(light.pos - hit_point);
        float nDl = optix::dot( Kn_val, L);

        // cast shadow ray
        float3 light_attenuation = make_float3(static_cast<float>( nDl > 0.0f ));
        if ( nDl > 0.0f && light.casts_shadow ) 
        {
            PerRayData_shadow shadow_prd;
            shadow_prd.attenuation = make_float3(1.0f);
            optix::Ray shadow_ray = optix::make_Ray( hit_point, L, shadow_ray_type, scene_epsilon, Ldist );
            rtTrace(top_shadower, shadow_ray, shadow_prd);
            light_attenuation = shadow_prd.attenuation;
        }

        // If not completely shadowed, light the hit point
        if( fmaxf(light_attenuation) > 0.0f ) 
        {
            float3 Lc = light.color * light_attenuation;

            result += Kd_val * nDl * Lc;

            float3 H = optix::normalize(L - ray.direction);
            float nDh = optix::dot( Kn_val, H );
            if(nDh > 0) 
            {
                float power = pow(nDh, phong_exp);
                result += Kr_val * power * Lc;
            }
        }
    }

    // fresnel reflectance
    float3 r = schlick(-dot(Kn_val, ray.direction), reflectivity_n);

    // calculate reflection
    if( prd_radiance.importance >= 0.01f && prd_radiance.depth <= max_depth ) 
    {
        // ray tree attenuation
        PerRayData_radiance new_prd;             
        new_prd.importance = (prd_radiance.importance * optix::luminance( r ));
        new_prd.depth = prd_radiance.depth + 1;

        // reflection ray
        if( new_prd.importance >= 0.01f && new_prd.depth <= max_depth) 
        {
            float3 R = optix::reflect( ray.direction, Kn_val );
            optix::Ray refl_ray = optix::make_Ray( hit_point, R, radiance_ray_type, scene_epsilon, RT_DEFAULT_MAX );
            rtTrace(top_object, refl_ray, new_prd);
            result +=  (r * new_prd.result);
        }
    }
    
    // pass the color back up the tree
    prd_radiance.result = result;
}