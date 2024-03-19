#include <optix.h>
#include <optix_math.h>
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

static __device__ __inline__ float CosTheta(const float3 &n, const float3 &w) 
{ 
    return dot(n, w); 
} 

static __device__ __inline__ float Cos2Theta(const float3 &n, const float3 &w) 
{ 
    float cosTheta = CosTheta(n, w);
    return cosTheta * cosTheta; 
} 

static __device__ __inline__ float AbsCosTheta(const float3 &n, const float3 &w) 
{ 
    return fabs(CosTheta(n, w)); 
}

static __device__ __inline__ float Sin2Theta(const float3 &n, const float3 &w) 
{
    return fmaxf(0.0f, 1.0f - Cos2Theta(n, w));
}

static __device__ __inline__ float SinTheta(const float3 &n, const float3 &w) 
{
    return sqrt(Sin2Theta(n, w)); 
}

static __device__ __inline__ float TanTheta(const float3 &n, const float3 &w) { 
    return SinTheta(n, w) / CosTheta(n, w); 
}

static __device__ __inline__ float Tan2Theta(const float3 &n, const float3 &w) {
    return Sin2Theta(n, w) / Cos2Theta(n, w);
}

static __device__ __inline__ float CosPhi(const float3 &n, const float3 &w) 
{
    float sinTheta = SinTheta(n, w);

    float3 normal = n;
    float3 binormal;
    if( fabs(normal.x) > fabs(normal.z) )
    {
        binormal.x = -normal.y;
        binormal.y =  normal.x;
        binormal.z =  0;
    }
    else
    {
        binormal.x =  0;
        binormal.y = -normal.z;
        binormal.z =  normal.y;
    }

    binormal = normalize(binormal);
    float3 tangent = cross(binormal, normal);

    optix::Matrix<3, 3> tangentMatrix;
    tangentMatrix.setRow(0, tangent);
    tangentMatrix.setRow(1, binormal);
    tangentMatrix.setRow(2, normal);

    float3 p = tangentMatrix * w;

    return (sinTheta == 0.0f) ? 1.0f : clamp(p.x / sinTheta, -1.0f, 1.0f); 
}

static __device__ __inline__ float SinPhi(const float3 &n, const float3 &w) 
{
    float sinTheta = SinTheta(n, w);
    
    float3 normal = n;
    float3 binormal;
    if( fabs(normal.x) > fabs(normal.z) )
    {
        binormal.x = -normal.y;
        binormal.y =  normal.x;
        binormal.z =  0;
    }
    else
    {
        binormal.x =  0;
        binormal.y = -normal.z;
        binormal.z =  normal.y;
    }

    binormal = normalize(binormal);
    float3 tangent = cross(binormal, normal);

    optix::Matrix<3, 3> tangentMatrix;
    tangentMatrix.setRow(0, tangent);
    tangentMatrix.setRow(1, binormal);
    tangentMatrix.setRow(2, normal);

    float3 p = tangentMatrix * w;

    return (sinTheta == 0.0f) ? 0.0f : clamp(p.y / sinTheta, -1.0f, 1.0f); 
}

static __device__ __inline__ float Cos2Phi(const float3 &n, const float3 &w) 
{
    float cosPhi = CosPhi(n, w);
    return cosPhi * cosPhi;
} 

static __device__ __inline__ float Sin2Phi(const float3 &n, const float3 &w) 
{ 
    float sinPhi = SinPhi(n, w);
    return sinPhi * sinPhi; 
}

static __device__ __inline__ float3 sqrt(const float3 &v) 
{ 
    return make_float3(sqrt(v.x), sqrt(v.y), sqrt(v.z));
}

//-----------------------------------------------------------------------------
//
//  BRDF based on PBRT Book
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  Beckmann
//-----------------------------------------------------------------------------

static __device__ __inline__ float BeckmannDistribution_Lambda(const float3 &n, const float3 &w, const float &alphax, const float &alphay)
{
    float absTanTheta = fabs(TanTheta(n, w));
    if (isinf(absTanTheta)) 
        return 0.0f;

    float alpha = sqrt(Cos2Phi(n, w) * alphax * alphax + Sin2Phi(n, w) * alphay * alphay);
    float a = 1.0f / (alpha * absTanTheta);
    if (a >= 1.6f) 
        return 0.0f;
        
    return (1.0f - (1.259f * a) + (0.396f * a * a)) / ((3.535f * a) + (2.181f * a * a));
}

static __device__ __inline__ float BeckmannDistribution_G(const float3 &n, const float3 &wo, const float3 &wi, const float &alphax, const float &alphay)
{
    return 1.0f / (1.0f + BeckmannDistribution_Lambda(n, wo, alphax, alphay) + BeckmannDistribution_Lambda(n, wi, alphax, alphay));
}

static __device__ __inline__ float BeckmannDistribution_D(const float3 &n, const float3 &wh, const float &alphax, const float &alphay)
{
    float tan2Theta = Tan2Theta(n, wh);
    if (isinf(tan2Theta)) 
        return 0.0f;
        
    const float cos4Theta = Cos2Theta(n, wh) * Cos2Theta(n, wh);
    return exp(-tan2Theta * (Cos2Phi(n, wh) / (alphax * alphax) + Sin2Phi(n, wh) / (alphay * alphay))) / (M_PIf * alphax * alphay * cos4Theta);
}

//-----------------------------------------------------------------------------
//  Trowbridge-Reitz
//-----------------------------------------------------------------------------

static __device__ __inline__ float TrowbridgeReitzDistribution_Lambda(const float3 &n, const float3 &w, const float &alphax, const float &alphay)
{
    float absTanTheta = fabs(TanTheta(n, w));
    if (isinf(absTanTheta)) 
        return 0.0f;

    // Compute _alpha_ for direction _w_
    float alpha = sqrt((Cos2Phi(n, w) * alphax * alphax) + (Sin2Phi(n, w) * alphay * alphay));
    float alpha2Tan2Theta = (alpha * absTanTheta) * (alpha * absTanTheta);
    return (-1.0f + sqrt(1.0f + alpha2Tan2Theta)) / 2.0f;
}

static __device__ __inline__ float TrowbridgeReitzDistribution_G(const float3 &n, const float3 &wo, const float3 &wi, const float &alphax, const float &alphay)
{
    return 1.0f / (1.0f + TrowbridgeReitzDistribution_Lambda(n, wo, alphax, alphay) + TrowbridgeReitzDistribution_Lambda(n, wi, alphax, alphay));
}

static __device__ __inline__ float TrowbridgeReitzDistribution_D(const float3 &n, const float3 &wh, const float &alphax, const float &alphay)
{
    float tan2Theta = Tan2Theta(n, wh);
    if (isinf(tan2Theta)) 
        return 0.0f;

    const float cos4Theta = Cos2Theta(n, wh) * Cos2Theta(n, wh);
    float e = ((Cos2Phi(n, wh) / (alphax * alphax)) + (Sin2Phi(n, wh) / (alphay * alphay))) * tan2Theta;
    return 1.0f / (M_PIf * alphax * alphay * cos4Theta * (1.0f + e) * (1.0f + e));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static __device__ __inline__ float RoughnessToAlpha(float roughness) 
{
    roughness = fmaxf(roughness, 1e-3);
    float x = log(roughness);
    return 1.62142f + (0.819955f * x) + (0.1734f * x * x) + (0.0171201f * x * x * x) + (0.000640711f * x * x * x * x);
}

static __device__ __inline__ float FrDielectric(float cosThetaI, float etaI, float etaT) 
{ 
    cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);
    bool entering = cosThetaI > 0.0f;
    if (!entering) 
    {
        float temp = etaI;
        etaI = etaT;
        etaT = temp;
        
        cosThetaI = fabs(cosThetaI);
    }

    float sinThetaI = sqrt(fmaxf(0.0f, 1.0f - (cosThetaI * cosThetaI)));
    float sinThetaT = etaI / etaT * sinThetaI;

    if (sinThetaT >= 1.0f) 
        return 1.0f;

    float cosThetaT = sqrt(fmaxf(0.0f, 1.0f - (sinThetaT * sinThetaT)));
    float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) / ((etaT * cosThetaI) + (etaI * cosThetaT));
    float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) / ((etaI * cosThetaI) + (etaT * cosThetaT)); 
    return ((Rparl * Rparl) + (Rperp * Rperp)) / 2.0f; 
}

static __device__ __inline__ float FrConductor(float cosThetaI, const float etai, const float etat, const float k) 
{
    cosThetaI = fabs(cosThetaI);
    cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);

    float eta = etat / etai;
    float etak = k / etai;

    float cosThetaI2 = cosThetaI * cosThetaI;
    float sinThetaI2 = 1.0f - cosThetaI2;
    float eta2 = eta * eta;
    float etak2 = etak * etak;

    float t0 = eta2 - etak2 - sinThetaI2;
    float a2plusb2 = sqrt((t0 * t0) + (4.0f * eta2 * etak2));
    float t1 = a2plusb2 + cosThetaI2;
    float a = sqrt(0.5f * (a2plusb2 + t0));
    float t2 = 2.0f * cosThetaI * a;
    float Rs = (t1 - t2) / (t1 + t2);

    float t3 = (cosThetaI2 * a2plusb2) + (sinThetaI2 * sinThetaI2);
    float t4 = t2 * sinThetaI2;
    float Rp = Rs * (t3 - t4) / (t3 + t4);

    return 0.5f * (Rp + Rs);
}

static __device__ __inline__ float3 SchlickFresnel(float NdotL, const float3 &R0)
{
    return R0 + (pow(1.0f - NdotL, 5.0f) * (1.0f - R0));
}

static __device__ __inline__ float3 schlick( float nDi, const float3& rgb )
{
    float r = fresnel_schlick(nDi, 5, rgb.x, 1);
    float g = fresnel_schlick(nDi, 5, rgb.y, 1);
    float b = fresnel_schlick(nDi, 5, rgb.z, 1);
    return make_float3(r, g, b);
}

static __device__ __inline__ float3 OrenNayar_fast_f(const float3 &reflectance, const float3 &normal, const float3 &viewDir, const float3 &lightDir, float roughness)
{
    float sinThetaI = SinTheta(normal, lightDir); 
    float sinThetaO = SinTheta(normal, viewDir);

    // Compute cosine term of Oren-Nayar model
    float maxCos = 0.0f;
    if (sinThetaI > 1e-4 && sinThetaO > 1e-4) 
    { 
        float sinPhiI = SinPhi(normal, lightDir);
        float cosPhiI = CosPhi(normal, lightDir); 
        float sinPhiO = SinPhi(normal, viewDir);
        float cosPhiO = CosPhi(normal, viewDir); 
        float dCos = (cosPhiI * cosPhiO) + (sinPhiI * sinPhiO); 
        maxCos = fmaxf(0.0f, dCos); 
    }

    // Compute sine and tangent terms of Oren-Nayar model
    float sinAlpha;
    float tanBeta;
    if (AbsCosTheta(normal, lightDir) > AbsCosTheta(normal, viewDir)) 
    { 
        sinAlpha = sinThetaO; 
        tanBeta = sinThetaI / AbsCosTheta(normal, lightDir); 
    } 
    else 
    { 
        sinAlpha = sinThetaI; 
        tanBeta = sinThetaO / AbsCosTheta(normal, viewDir); 
    }

    float A;
    float B;
    if(roughness < 1e-3)
    {
        A = 1.0f;
        B = 0.0f;
    }
    else
    {
        float sigmaPow2 = roughness * roughness;

        A = 1.0f - (sigmaPow2 / ((2.0f * sigmaPow2) + 0.33f));
        B = 0.45f * sigmaPow2 / (sigmaPow2 + 0.09f);
    }

    return reflectance / M_PIf * saturate(A + (B * maxCos * sinAlpha * tanBeta));
}


static __device__ __inline__ float3 OrenNayar_full_f(const float3 &reflectance, const float3 &normal, const float3 &viewDir, const float3 &lightDir, float roughness)
{
    float sinThetaI = SinTheta(normal, lightDir); 
    float sinThetaO = SinTheta(normal, viewDir);

    // Compute cosine term of Oren-Nayar model
    float dCos = 0.0f;
    if (sinThetaI > 1e-4 && sinThetaO > 1e-4) 
    { 
        float sinPhiI = SinPhi(normal, lightDir);
        float cosPhiI = CosPhi(normal, lightDir); 
        float sinPhiO = SinPhi(normal, viewDir);
        float cosPhiO = CosPhi(normal, viewDir); 
        dCos = (cosPhiI * cosPhiO) + (sinPhiI * sinPhiO); 
    }

    // Compute sine and tangent terms of Oren-Nayar model
    float alpha;
    float beta;
    if (acosf(dot(normal, lightDir)) > acosf(dot(normal, viewDir))) 
    { 
        alpha = acosf(CosTheta(normal, lightDir)); 
        beta = acosf(CosTheta(normal, viewDir));
    } 
    else 
    { 
        alpha = acosf(CosTheta(normal, viewDir));
        beta = acosf(CosTheta(normal, lightDir)); 
    }

    float sigmaPow2 = (roughness * roughness);
    float C1 = 1.0f - (0.5f * (sigmaPow2 / (sigmaPow2 + 0.33f)));
    float C2;
    if(dCos > 0.0f)
    {
        C2 = 0.45f * (sigmaPow2 / (sigmaPow2 + 0.09f)) * sinf(alpha);
    }
    else
    {
        C2 = 0.45f * (sigmaPow2 / (sigmaPow2 + 0.09f)) * (sinf(alpha) - (((2.0f * beta) / M_PIf) * ((2.0f * beta) / M_PIf) * ((2.0f * beta) / M_PIf)));
    }
    float C3 = 0.125f * (sigmaPow2 / (sigmaPow2 + 0.09f)) * ((4.0f * alpha * beta) / (M_PIf * M_PIf)) * ((4.0f * alpha * beta) / (M_PIf * M_PIf));

    float3 direct = reflectance / M_PIf * (C1 + (dCos * C2 * tanf(beta)) + ((1.0f - fabs(dCos)) * C3 * tanf((alpha + beta) / 2.0f)));
    float3 interreflection = 0.17f * ((reflectance * reflectance) / M_PIf) * (sigmaPow2 / (sigmaPow2 + 0.13f)) * (1.0f - (dCos * (((2.0f * beta) / M_PIf) * ((2.0f * beta) / M_PIf))));
    float3 result = direct + interreflection;
    return make_float3(saturate(result.x), saturate(result.y), saturate(result.z));
}

static __device__ __inline__ float3 TorranceSparrow_f(const float3 &normal, const float3 &viewDir, const float3 &lightDir, const float3 &fresnel, float roughness)
{
    float cosThetaO = AbsCosTheta(normal, viewDir);
    float cosThetaI = AbsCosTheta(normal, lightDir);
    float3 wh = lightDir + viewDir;

    // Handle degenerate cases for microfacet reflection
    if (cosThetaI == 0.0f || cosThetaO == 0.0f) 
        return make_float3(0.0f);
    
    if (wh.x == 0.0f && wh.y == 0 && wh.z == 0.0f) 
        return make_float3(0.0f);

    wh = normalize(wh);
    
    float alpha = RoughnessToAlpha(roughness);

    return (BeckmannDistribution_D(normal, wh, alpha, alpha) * BeckmannDistribution_G(normal, viewDir, lightDir, alpha, alpha) * fresnel / (4.0f * cosThetaI * cosThetaO));
}
