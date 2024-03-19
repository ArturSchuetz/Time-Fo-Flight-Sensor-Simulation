/* 
 * Copyright (c) 2018 NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disklaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disklaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE diskLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <optix_world.h>
#include "intersection_refinement.h"

using namespace optix;

rtDeclareVariable(float4,  disk_position, , );
rtDeclareVariable(float4,  disk_normal, , );

rtDeclareVariable(float3, texcoord,         attribute texcoord, ); 
rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_tangent,  attribute shading_tangent, ); 
rtDeclareVariable(float3, shading_bitangent, attribute shading_bitangent, ); 
rtDeclareVariable(float3, shading_normal,   attribute shading_normal, ); 

rtDeclareVariable(float3, back_hit_point,   attribute back_hit_point, ); 
rtDeclareVariable(float3, front_hit_point,  attribute front_hit_point, ); 

rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

bool intersectPlane(const float3 &n, const float3 &p0, const float3 &l0, const float3 &l, float &t) 
{ 
  float denom = dot(n, l); 
  if (denom > 1e-6) 
  { 
    float3 p0l0 = p0 - l0; 
    t = dot(p0l0, n) / denom; 
    return (t >= 0.0f); 
  }
  return false; 
} 

 template<bool use_robust_method>
 static __device__
 void intersect_disk(void)
 {
  float3 center = make_float3(disk_position);
  float3 O = ray.origin - center;
  float3 D = ray.direction;
  float radius = disk_position.w;

  float t = 0; 
  if (intersectPlane(n, p0, l0, l, t))
  { 
    float3 p = l0 + l * t; 
    float3 v = p - p0; 
    float d2 = dot(v, v); 
    if (d2 <= radius * radius)
    {

    }
  }
 }
 
 
 RT_PROGRAM void intersect(int primIdx)
 {
  intersect_disk<false>();
 }
 
 
 RT_PROGRAM void robust_intersect(int primIdx)
 {
  intersect_disk<true>();
 }
 
 
 RT_PROGRAM void bounds (int, float result[6])
 {
   const float3 cen = make_float3( sphere );
   const float3 rad = make_float3( sphere.w );
 
   optix::Aabb* aabb = (optix::Aabb*)result;
   
   if( rad.x > 0.0f  && !isinf(rad.x) ) {
     aabb->m_min = cen - rad;
     aabb->m_max = cen + rad;
   } else {
     aabb->invalidate();
   }
 }
 
 