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
#include "random.h"
#include "helpers.h"
#include "microfacet.h"

struct PerRayData_radiance
{
    float3 result;
    float3 radiance;
    float3 attenuation;

    float3 origin;
    float3 direction;

    float current_index_of_refraction;
    unsigned int seed;
    int depth;
    int done;
};

struct PerRayData_shadow
{
    bool inShadow;
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

// Scene wide variables

rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint2, launch_dim,   rtLaunchDim, );
rtDeclareVariable(float, time_view_scale, , ) = 1e-6f;


//-----------------------------------------------------------------------------
//  Camera program -- main ray tracing loop
//-----------------------------------------------------------------------------

rtDeclareVariable(float3,        eye, , );
rtDeclareVariable(float3,        U, , );
rtDeclareVariable(float3,        V, , );
rtDeclareVariable(float3,        W, , );
rtDeclareVariable(float3,        bad_color, , );
rtDeclareVariable(float3, 		 bg_color, , );
rtDeclareVariable(unsigned int,  frame_number, , );
rtDeclareVariable(unsigned int,  sqrt_num_samples, , );
rtDeclareVariable(unsigned int,  rr_begin_depth, , );

rtBuffer<float4, 2> input_rayDirections;
rtBuffer<float4, 2> output_buffer;

rtDeclareVariable(float3, back_hit_point,   attribute back_hit_point, ); 
rtDeclareVariable(float3, front_hit_point,  attribute front_hit_point, ); 
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

rtDeclareVariable(float, t_hit, rtIntersectionDistance, );
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow, rtPayload, );

rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable(float3, shading_tangent, attribute shading_tangent, ); 
rtDeclareVariable(float3, shading_bitangent, attribute shading_bitangent, ); 

rtBuffer<BasicLight>                 lights;
rtDeclareVariable(unsigned int,      radiance_ray_type, , );
rtDeclareVariable(unsigned int,      shadow_ray_type, , );
rtDeclareVariable(float,             scene_epsilon, , );
rtDeclareVariable(rtObject,          top_object, , );
rtDeclareVariable(rtObject,          top_shadower, , );

const float smallest_value = 0.01f; 

//-----------------------------------------------------------------------------
//
//  Camera program -- main ray tracing loop
//
//-----------------------------------------------------------------------------
/*
static __device__ __inline__ void random_concentric_sample_hemisphere(float u, float v, float3& p)
{
    float r = 0.0f;
    float phi = 0.0f;

    u = (2.0f * u) - 1.0f;
    v = (2.0f * v) - 1.0f;

    if (u > -v)
    {
        if (u > v)
        {
            r = u;
            phi = (M_PIf / 4.0f) * (v / u);
        }
        else
        {
            r = v;
            phi = (M_PIf / 4.0f) * (2.0f - (u / v));
        }
    }
    else
    {
        if(u < v)
        {
            r = -u;
            phi = (M_PIf / 4.0f) * (4.0f + (v / u));
        }
        else
        {
            r = -v;
            if (v != 0.0f)
            {                
                phi = (M_PIf / 4.0f) * (6.0f - (u / v));
            }
            else
            {
                phi = 0.0f;
            }
        }
    }
    // Project up to hemisphere.
    p.x = r * cosf( phi );
    p.y = r * sinf( phi );
    p.z = sqrtf( fmaxf( 0.0f, 1.0f - (p.x * p.x) - (p.y * p.y) ) );
}
*/

RT_PROGRAM void pathtrace_pinhole_camera()
{
    size_t2 screen = output_buffer.size();

    float2 inv_screen = 1.0f/make_float2(screen) * 2.f;
    float2 pixel = (make_float2(launch_index)) * inv_screen - 1.f;

    float2 jitter_scale = inv_screen / sqrt_num_samples;
    unsigned int samples_per_pixel = sqrt_num_samples*sqrt_num_samples;
    float3 result = make_float3(0.0f);

    size_t2 bufferSize = input_rayDirections.size();
    unsigned int seed = tea<16>(screen.x * launch_index.y + launch_index.x, frame_number);
    do 
    {
        //
        // Sample pixel
        //
        float2 jitter = make_float2(0.0f);
        do
        {
            jitter = make_float2((rnd(seed) * 2.0f) - 1.0f, (rnd(seed) * 2.0f) - 1.0f);
        } while(sqrt((jitter.x * jitter.x) + (jitter.y * jitter.y)) > 1.0f);

        float2 d = pixel + (jitter * inv_screen * 1.0f);

        if(d.x > 1.0f) d.x = 1.0f;
        if(d.y > 1.0f) d.y = 1.0f;
        if(d.x < -1.0f) d.x = -1.0f;
        if(d.y < -1.0f) d.y = -1.0f;

        //
        // Sample pixel using jittering
        //
        /*
        unsigned int x = samples_per_pixel % sqrt_num_samples;
        unsigned int y = samples_per_pixel / sqrt_num_samples;
        float2 jitter = make_float2(x - rnd(seed), y - rnd(seed));
        float2 d = pixel + jitter * jitter_scale;
        */

        float2 relativeCoordinate = ((d + 1.0f) / 2.0f);
        float3 calculated_ray = make_float3(input_rayDirections[make_uint2(relativeCoordinate.x * (bufferSize.x - 1), relativeCoordinate.y * (bufferSize.y - 1))]);

        float3 ray_origin = eye;
        float3 ray_direction = normalize(calculated_ray.x*U + calculated_ray.y*V + -calculated_ray.z*W);

        // Initialze per-ray data
        PerRayData_radiance prd;
        prd.result = make_float3(0.f);
        prd.attenuation = make_float3(1.f);

        prd.current_index_of_refraction = 1.00029f; // ior of air
        prd.seed = seed;
        prd.depth = 0;
        prd.done = false;

        // Each iteration is a segment of the ray path.  The closest hit will
        // return new segments to be traced here.
        for(;;)
        {
            Ray ray = make_Ray(ray_origin, ray_direction, radiance_ray_type, scene_epsilon, RT_DEFAULT_MAX);
            rtTrace(top_object, ray, prd);

            if(prd.done)
            {
                // We have hit the background or a luminaire
                prd.result += prd.radiance;
                break;
            }

            // Russian roulette termination 
            if(prd.depth >= rr_begin_depth)
            {
                float pcont = fmaxf(prd.attenuation);
                if(rnd(prd.seed) >= pcont)
                    break;
                    
                prd.attenuation /= pcont;
            }

            prd.depth++;
            prd.result += prd.radiance;

            // Update ray data for the next path segment
            ray_origin = prd.origin;
            ray_direction = prd.direction;
        }

        result += prd.result;

        seed = prd.seed;
    } while (--samples_per_pixel);

    //
    // Update the output buffer
    //
    float3 pixel_color = result/(sqrt_num_samples*sqrt_num_samples);

    if (frame_number > 1)
    {
        float a = 1.0f / (float)frame_number;
        float3 old_color = make_float3(output_buffer[launch_index]);
        output_buffer[launch_index] = make_float4( lerp( old_color, pixel_color, a ), 1.0f );
    }
    else
    {
        output_buffer[launch_index] = make_float4(pixel_color, 1.0f);
    }
}

rtTextureSampler<float4, 2> Kd_map;
rtTextureSampler<float4, 2> Ks_map;
rtTextureSampler<float4, 2> Kn_map;
rtTextureSampler<float4, 2> Tr_map;
rtTextureSampler<float4, 2> Pm_map;
rtTextureSampler<float4, 2> Ke_map;
rtDeclareVariable(float3, texcoord, attribute texcoord, ); 
rtDeclareVariable(float, index_of_refraction, , );
rtDeclareVariable(float, absorption_coefficien, , );

RT_PROGRAM void any_hit_shadow()
{
    const float3 Tr_val = make_float3( tex2D( Tr_map, texcoord.x, texcoord.y ) );
    if(Tr_val.x < 1.0f)
    {
        rtIgnoreIntersection();
    }
    else
    {
        prd_shadow.inShadow = true;
        rtTerminateRay();
    }
}

//-----------------------------------------------------------------------------
//
//  BRDF based on Cook–Torrance and Oren-Nayar
//
//-----------------------------------------------------------------------------

RT_PROGRAM void microfacet_closest_hit()
{
    float3 world_geometric_normal	= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
    float3 world_shading_tangent	= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_tangent ) );
    float3 world_shading_bitangent	= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_bitangent ) );
    float3 world_shading_normal		= normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );

    float3 fftangent = faceforward( world_shading_tangent, -ray.direction, world_geometric_normal );
    float3 ffbitangent = faceforward( world_shading_bitangent, -ray.direction, world_geometric_normal );
    float3 ffnormal = faceforward( world_shading_normal, -ray.direction, world_geometric_normal );

    Matrix<3, 3> tangentMatrix;
    tangentMatrix.setCol(0, fftangent);
    tangentMatrix.setCol(1, ffbitangent);
    tangentMatrix.setCol(2, ffnormal);

    const float3 Kd_val = make_float3( tex2D( Kd_map, texcoord.x, texcoord.y ) );
    const float3 Ks_val = make_float3( tex2D( Ks_map, texcoord.x, texcoord.y ) );
          float3 Kn_val = normalize(make_float3( tex2D( Kn_map, texcoord.x, texcoord.y ) ) * 2.0f - 1.0f);
    const float3 Tr_val = make_float3( tex2D( Tr_map, texcoord.x, texcoord.y ) );
    const float3 Pm_val = make_float3( tex2D( Pm_map, texcoord.x, texcoord.y ) );
    const float3 Ke_val = make_float3( tex2D( Ke_map, texcoord.x, texcoord.y ) );

    Kn_val = tangentMatrix * make_float3(Kn_val.x, Kn_val.y, Kn_val.z);

    if(length(Kn_val) > 0)
        Kn_val = normalize(Kn_val);
    else
        Kn_val = ffnormal;

    if(Tr_val.x < 1.0f)
    {       
        if(prd_radiance.depth > 0 && Tr_val.x == 0.0f)
            prd_radiance.depth = prd_radiance.depth - 1;

        prd_radiance.attenuation = prd_radiance.attenuation * (1.0f - Tr_val.x);
        
        const float3 bhp = rtTransformPoint(RT_OBJECT_TO_WORLD, back_hit_point);
        prd_radiance.origin = bhp;
        prd_radiance.direction = ray.direction;

        prd_radiance.radiance = make_float3(0.0f);
        return;
    }
    
    float roughness = saturate(1.0f - Ks_val.x);
    float metallic = Pm_val.x;

    float3 hit_point = ray.origin + t_hit * ray.direction;


    //
    // Next event estimation (compute direct lighting).
    //
    float3 result = make_float3(0.0f);

    unsigned int num_lights = lights.size();
    for(int i = 0; i < num_lights; ++i)
    {
        // Choose random point on light
        BasicLight light = lights[i];

        // Calculate properties of light sample (for area based pdf)
        float3 lightDir = light.pos - hit_point;
        const float LightdistPow2 = dot(lightDir, lightDir);
        const float Lightdist = sqrt(LightdistPow2);
        lightDir = lightDir / Lightdist;

        const float NdotL = saturate(dot(Kn_val, lightDir));

        // cast shadow ray
        if ( NdotL > smallest_value)
        {
            PerRayData_shadow shadow_prd;
            shadow_prd.inShadow = false;
            // Note: bias both ends of the shadow ray, in case the light is also present as geometry in the scene.
            Ray shadow_ray = make_Ray( hit_point, lightDir, shadow_ray_type, scene_epsilon, Lightdist - scene_epsilon);
            rtTrace(top_object, shadow_ray, shadow_prd);

            if(!shadow_prd.inShadow)
            {
                float3 fresnel;
                if(metallic >= 0.99f)
                {
                    fresnel = Kd_val * FrConductor(dot(-ray.direction, lightDir), prd_radiance.current_index_of_refraction, index_of_refraction, absorption_coefficien);
                }
                else if(metallic <= 0.01f)
                {
                    fresnel = make_float3(FrDielectric(dot(-ray.direction, lightDir), prd_radiance.current_index_of_refraction, index_of_refraction));
                }
                else
                {
                    fresnel = make_float3(0.0f);
                }
        
                // =====================================
                // Visible light
                float3 diffuse = OrenNayar_full_f(Kd_val, Kn_val, -ray.direction, lightDir, roughness);
                diffuse = diffuse * (make_float3(1.0f) - fresnel) * (1.0f - metallic);

                float3 specular = TorranceSparrow_f(Kn_val, -ray.direction, lightDir, fresnel, roughness);
            
                result += prd_radiance.attenuation * (diffuse + specular) * (light.color * (NdotL / LightdistPow2));
            }
        }
    }

    prd_radiance.radiance = result;
    
    if(Ke_val.x > 0.0f || Ke_val.y > 0.0f || Ke_val.z > 0.0f)
    {
        prd_radiance.radiance += prd_radiance.attenuation * Ke_val;
    }

    //
    // Generate a reflection ray.  This will be traced back in ray-gen.
    //
    prd_radiance.origin = hit_point;

    float3 p;

    optix::Onb onb( Kn_val );
    float z1=rnd(prd_radiance.seed);
    float z2=rnd(prd_radiance.seed);
    random_sample_hemisphere(z1, z2, p);

    onb.inverse_transform(p);    
    prd_radiance.direction = normalize(p);
        
    const float NdotL = saturate(dot(Kn_val, prd_radiance.direction));
    if (NdotL > smallest_value)
    {
        // Calculate fresnel
        float3 fresnel;
        float3 diffuse;
        float3 specular;
        if(metallic >= 0.99f)
        {
            fresnel = Kd_val * FrConductor(dot(prd_radiance.direction, normalize(-ray.direction + prd_radiance.direction)), prd_radiance.current_index_of_refraction, index_of_refraction, absorption_coefficien);
           
            diffuse = make_float3(0.0f);
            
            specular = TorranceSparrow_f(Kn_val, -ray.direction, prd_radiance.direction, fresnel, roughness);
        }
        else if(metallic <= 0.01f)
        {
            fresnel = make_float3(FrDielectric(dot(prd_radiance.direction, normalize(-ray.direction + prd_radiance.direction)), prd_radiance.current_index_of_refraction, index_of_refraction));
            
            diffuse = OrenNayar_full_f(Kd_val, Kn_val, -ray.direction, prd_radiance.direction, roughness);
            diffuse = diffuse * (make_float3(1.0f) - fresnel);
            
            specular = TorranceSparrow_f(Kn_val, -ray.direction, prd_radiance.direction, fresnel, roughness);
        }
        else
        {
            float R0 = (prd_radiance.current_index_of_refraction - index_of_refraction) / (prd_radiance.current_index_of_refraction + index_of_refraction);
            R0 *= R0;
            float3 F0 = lerp(make_float3(R0), Kd_val, metallic);
            fresnel = SchlickFresnel(dot(normalize(-ray.direction + prd_radiance.direction), prd_radiance.direction), F0);

            diffuse = OrenNayar_full_f(Kd_val, Kn_val, -ray.direction, prd_radiance.direction, roughness);
            diffuse = diffuse * (make_float3(1.0f) - fresnel) * (1.0f - metallic);
            
            specular = TorranceSparrow_f(Kn_val, -ray.direction, prd_radiance.direction, fresnel, roughness);
        }

        // http://www.rorydriscoll.com/2009/01/07/better-sampling/
        prd_radiance.attenuation = ((diffuse + specular) * prd_radiance.attenuation) * 2.0f * M_PIf * NdotL;
    }
    else
    {
        prd_radiance.attenuation = make_float3(0.0f);
    }
}

//-----------------------------------------------------------------------------
//
//  Exception program
//
//-----------------------------------------------------------------------------

RT_PROGRAM void exception()
{
	const unsigned int code = rtGetExceptionCode();
	rtPrintf( "Caught exception 0x%X at launch index (%d,%d)\n", code, launch_index.x, launch_index.y );
    output_buffer[launch_index] = make_float4(bad_color, 1.0f);
}

//-----------------------------------------------------------------------------
//
//  Miss program
//
//-----------------------------------------------------------------------------

RT_PROGRAM void miss()
{
    prd_radiance.radiance = prd_radiance.attenuation * bg_color;
    prd_radiance.done = true;
}

//
// Environment map background
//
rtTextureSampler<float4, 2> envmap;
RT_PROGRAM void envmap_miss()
{
    float theta = atan2f( ray.direction.x, ray.direction.z );
    float phi   = M_PIf * 0.5f -  acosf( ray.direction.y );
    float u     = (theta + M_PIf) * (0.5f * M_1_PIf);
    float v     = 0.5f * ( 1.0f + sinf(phi) );
    prd_radiance.radiance = prd_radiance.attenuation * make_float3( tex2D(envmap, u, v) );
    prd_radiance.done = true;
}