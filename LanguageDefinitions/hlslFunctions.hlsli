
SamplerState internalSampler; 

#define FLT_EPSILON 1.192092896e-07f
#define nMipOffset 3
#define PI 3.14159265358979323846f

//@buffer CommonBuffer
cbuffer CommonBuffer : register(b0)
{
    float4x4 worldToClipSpaceMatrix ;
    float4x4 clipToWorldSpaceMatrix ;
    float4x4 projectionMatrix ;
    float4x4 viewMatrix ;
    float4x4 projMatrixInverse ;
    float4x4 viewMatrixInverse ;
    float4 cameraPosition ;
    uint2 clientResolution ;
    float currentTime ;
    float deltaTime ;
    float2 nearFarPlane ;
    float2 PADDING2 ;
}

//@buffer VFXInstance
cbuffer VFXInstance : register(b6)
{
    float4 color ;
    float2 uvScroll ;
    float2 uvScale ;
    float4 bloomAttributes ;
}

//@texture2D GBufferWorldPosition
Texture2D GBufferWorldPosition    : register(t0);

//@texture2D GBufferAlbedo
Texture2D GBufferAlbedo           : register(t1);

//@texture2D GBufferNormal
Texture2D GBufferNormal           : register(t2);

//@texture2D GBufferMaterial
Texture2D GBufferMaterial         : register(t3);

//@texture2D GBufferEffects
Texture2D GBufferEffects          : register(t4);

//@texture2D GBufferAmbientOcclusion
Texture2D GBufferAmbientOcclusion : register(t5);

//@texture2D GBufferDepth
Texture2D GBufferDepth            : register(t6);

//@texture2D GBufferSSAO
Texture2D GBufferSSAO         : register(t7);

//vfx

//@texture2D VFXAlbedoTex
Texture2D VFXAlbedoTex    : register(t8);

//@texture2D VFXNormalTex
Texture2D VFXNormalTex    : register(t9);

//@texture2D VFXMaterialTex
Texture2D VFXMaterialTex    : register(t10);

//@texture2D VFXEffectsTex
Texture2D VFXEffectsTex    : register(t11);

//

//@texture2D ModelAlbedoTex
Texture2D ModelAlbedoTex    : register(t0);

//@texture2D ModelNormalTex
Texture2D ModelNormalTex    : register(t1);

//@texture2D ModelMaterialTex
Texture2D ModelMaterialTex    : register(t2);

//@texture2D ModelEffectsTex
Texture2D ModelEffectsTex    : register(t3);


//@struct InstancedVertexInput
struct InstancedVertexInput
{
    float4 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tan      : TANGENT;
    float3 bitan    : BITANGENT;

    float4x4 transform          : INSTANCE_TRANSFORM;
    float4   instanceAttributes : INSTANCE_ATTRIBUTES;
};

//@struct PixelInput
struct PixelInput
{
    float4 worldPos   : POSITION;
    float4 position   : SV_POSITION;
    float2 texCoord   : TEXCOORD;
    float3 normal     : NORMAL;
    float3 tangent    : TANGENT;
    float3 bitan      : BITANGENT;
    float4 attributes : ATTRIBUTES;
};

//@struct VFXPixelInput
struct VFXPixelInput
{
    float4 worldPos : POSITION;
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitan : BITANGENT;
    float4 attributes : ATTRIBUTES;

    float4 centerWorldPos : CENTERWORLDPOS;
    float4 minWorldPos : MINWORLDPOS;
    float4 maxWorldPos : MAXWORLDPOS;

    float4 worldUPDir : WORLDUP;
};

//@struct MultiPixelOutput
struct MultiPixelOutput
{
    float4 color1 : SV_TARGET0;
    float4 color2 : SV_TARGET1;
};

//@struct GBufferOutput
struct GBufferOutput
{
    float4 worldPosition             : SV_TARGET0;
    float4 albedo                    : SV_TARGET1;
    float4 normal                    : SV_TARGET2;
    float4 material                  : SV_TARGET3;
    float4 effects                   : SV_TARGET4;
    float4 ambientOcclusionAndCustom : SV_TARGET5;
};

//@entrypoint VS_Instanced
PixelInput main(InstancedVertexInput aInput);

//@entrypoint PS_Deferred
GBufferOutput main(PixelInput aInput);

//@entrypoint PS_VFX
MultiPixelOutput mainVFX(VFXPixelInput aInput);

float4 mainPreview(PixelInput aInput) : SV_TARGET0;
float4 mainPreview(VFXPixelInput aInput) : SV_TARGET0;

//@function MatrixTranspose
float4x4 Transpose(float4x4 a)
{
    return transpose(a);
}

//@function Matrix4to3
float3x3 Matrix4to3(float4x4 a)
{
    return float3x3(
       a._11, a._12, a._13,
       a._21, a._22, a._23,
       a._31, a._32, a._33
    );
}

//@function MulMatrixMatrix4
float4x4 MulMatrixMatrix4(float4x4 a, float4x4 b)
{
    return mul(a, b);
}

//@function MulMatrixVector4
float4 MulMatrixVector4(float4x4 a, float4 b)
{
    return mul(a, b);
}

//@function MulVector4Matrix
float4 MulVector4Matrix(float4 a, float4x4 b)
{
    return mul(a, b);
}

//@function MulMatrixMatrix3
float3x3 MulMatrixMatrix3(float3x3 a, float3x3 b)
{
    return mul(a, b);
}

//@function MulMatrixVector3
float3 MulMatrixVector3(float3x3 a, float3 b)
{
    return mul(a, b);
}

//@function MulVector3Matrix
float3 MulVector3Matrix(float3 a, float3x3 b)
{
    return mul(a, b);
}

//@function float4
float4 AssembleFloat4(float x, float y, float z, float w)
{
    return float4(x, y, z, w);
}

//@function float3
float3 AssembleFloat3(float x, float y, float z)
{
    return float3(x, y, z);
}

//@function float2
float2 AssembleFloat2(float x, float y)
{
    return float2(x, y);
}

//@function float3from4
float3 float3from4(float4 a)
{
    return a.xyz;
}

//@function float4from3
float4 float4from3(float3 a, float w)
{
    return float4(a, w);
}

//@function Sample2D
float4 Sample2D(float2 uv, Texture2D tex)
{
    return tex.Sample(internalSampler, uv);
}

//@function TestFunction
float4 TestFunction(float4 color)
{
    return color;
}

//@function bias
float bias(float value, float b)
{
    return (b > 0.0) ? pow(abs(value), log(b) / log(0.5)) : 0.0f;
}

//@function gain
float gain(float value, float g)
{
    return 0.5 * ((value < 0.5) ? bias(2.0 * value, 1.0 - g) : (2.0 - bias(2.0 - 2.0 * value, 1.0 - g)));
}

//@function RoughnessFromPerceptualRoughness
float RoughnessFromPerceptualRoughness(float perceptualRoughness)
{
    return perceptualRoughness * perceptualRoughness;
}

//@function PerceptualRougnessFromRoughness
float PerceptualRougnessFromRoughness(float roughness)
{
    return sqrt(max(0.0, roughness));
}

//@function SpecularPowerFromPerceptualRoughness
float SpecularPowerFromPerceptualRoughness(float perceptualRoughness)
{
    float roughness = RoughnessFromPerceptualRoughness(perceptualRoughness);
    return (2.0 / max(FLT_EPSILON, roughness * roughness)) - 2.0;
}

//@function PerceptualRougnessFromSpecularPower
float PerceptualRougnessFromSpecularPower(float specularPower)
{
    float roughness = sqrt(2.0 / (specularPower + 2.0));
    return PerceptualRougnessFromRoughness(roughness);
}

//@function BurleyToMip
float BurleyToMip(float fPerceptualRoughness, int nMips, float NdotR)
{
    float specPower = SpecularPowerFromPerceptualRoughness(fPerceptualRoughness);
    specPower /= (4 * max(NdotR, FLT_EPSILON));
    float scale = PerceptualRougnessFromSpecularPower(specPower);
    return scale * (nMips - 1 - nMipOffset);
}

//@function GetSpecularDominantDir
float3 GetSpecularDominantDir(float3 vN, float3 vR, float roughness)
{
    float invRough = saturate(1 - roughness);
    float alpha = invRough * (sqrt(invRough) + roughness);

    return lerp(vN, vR, alpha);
}

//@function GetReductionInMicrofacets
float GetReductionInMicrofacets(float perceptualRoughness)
{
    float roughness = RoughnessFromPerceptualRoughness(perceptualRoughness);

    return 1.0 / (roughness * roughness + 1.0);
}

//@function EmpiricalSpecularAO
float EmpiricalSpecularAO(float ao, float perceptualRoughness)
{
    float smooth = 1 - perceptualRoughness;
    float specAO = gain(ao, 0.5 + max(0.0, smooth * 0.4));

    return min(1.0, specAO + lerp(0.0, 0.5, smooth * smooth * smooth * smooth));
}

//@function ApproximateSpecularSelfOcclusion
float ApproximateSpecularSelfOcclusion(float3 vR, float3 vertNormalNormalized)
{
    const float fadeParam = 1.3;
    float rimmask = clamp(1 + fadeParam * dot(vR, vertNormalNormalized), 0.0, 1.0);
    rimmask *= rimmask;

    return rimmask;
}

//@function Diffuse
float3 Diffuse(float3 pAlbedo)
{
    return pAlbedo / PI;
}

//@function NormalDistribution_GGX
float NormalDistribution_GGX(float a, float NdH)
{
    // Isotropic ggx
    float a2 = a * a;
    float NdH2 = NdH * NdH;

    float denominator = NdH2 * (a2 - 1.0f) + 1.0f;
    denominator *= denominator;
    denominator *= PI;

    return a2 / denominator;
}

//@function Geometric_Smith_Schlick_GGX
float Geometric_Smith_Schlick_GGX(float a, float NdV, float NdL)
{
    // Smith Schlick-GGX
    float k = a * 0.5f;
    float GV = NdV / (NdV * (1 - k) + k);
    float GL = NdL / (NdL * (1 - k) + k);

    return GV * GL;
}

//@function Fresnel_Schlick
float3 Fresnel_Schlick(float3 specularColor, float3 h, float3 v)
{
    return (specularColor + (1.0f - specularColor) * pow((1.0f - saturate(dot(v, h))), 5));
}

//@function Specular
float3 Specular(float3 specularColor, float3 h, float3 v, float a, float NdL, float NdV, float NdH)
{
    return ((NormalDistribution_GGX(a, NdH) * Geometric_Smith_Schlick_GGX(a, NdV, NdL)) * Fresnel_Schlick(specularColor, h, v)) / (4.0f * NdL * NdV + 0.0001f);
}

//@function EvaluateAmbiance
float3 EvaluateAmbiance(TextureCube lysBurleyCube, float3 vN, float3 VNUnit, float3 toEye, float perceptualRoughness, float ao, float3 dfcol, float3 spccol)
{
    // int numMips = GetNumMips(lysBurleyCube);

    int numMips = 0;
    const int nrBrdMips = numMips - nMipOffset;
    float VdotN = saturate(dot(toEye, vN));//clamp(dot(toEye, vN), 0.0, 1.0f);
    const float3 vRorg = 2 * vN * VdotN - toEye;

    float3 vR = GetSpecularDominantDir(vN, vRorg, RoughnessFromPerceptualRoughness(perceptualRoughness));
    float RdotNsat = saturate(dot(vN, vR));

    //float mipLevel = BurleyToMip(perceptualRoughness, numMips, RdotNsat);
//
    //float3 specRad = lysBurleyCube.SampleLevel(defaultSampler, vR, mipLevel).xyz;
    //float3 diffRad = lysBurleyCube.SampleLevel(defaultSampler, vN, (float)(nrBrdMips - 1)).xyz;

    float fT = 1.0 - RdotNsat;
    float fT5 = fT * fT;
    fT5 = fT5 * fT5 * fT;
    spccol = lerp(spccol, (float3) 1.0, fT5);

    float fFade = GetReductionInMicrofacets(perceptualRoughness);
    fFade *= EmpiricalSpecularAO(ao, perceptualRoughness);
    fFade *= ApproximateSpecularSelfOcclusion(vR, VNUnit);

    //float3 ambientdiffuse = ao * dfcol * diffRad;
    //float3 ambientspecular = fFade * spccol * specRad;

	return float3(1.0f,1.0f,1.0f);// ambientdiffuse + ambientspecular;
}

//@function EvaluateDirectionalLight
float3 EvaluateDirectionalLight(float3 albedoColor, float3 specularColor, float3 normal, float roughness, float3 lightColor, float3 lightDir, float3 viewDir)
{
    // Compute som useful values
    float NdL = saturate(dot(normal, lightDir));
    float lambert = NdL; // Angle attenuation
    float NdV = saturate(dot(normal, viewDir));
    float3 h = normalize(lightDir + viewDir);
    float NdH = saturate(dot(normal, h));
    float VdH = saturate(dot(viewDir, h));
    float LdV = saturate(dot(lightDir, viewDir));
    float a = max(0.001f, roughness * roughness);

    float3 cDiff = Diffuse(albedoColor);
    float3 cSpec = Specular(specularColor, h, viewDir, a, NdL, NdV, NdH);

    return saturate(lightColor * lambert * (cDiff * (1.0 - cSpec) + cSpec) * PI);
}


//@function EvaluatePointLight
float3 EvaluatePointLight(float3 albedoColor, float3 specularColor, float3 normal, float roughness,
    float3 lightColor, float lightIntensity, float lightRange, float3 lightPos, float3 viewDir, float3 pixelPos)
{
    // Compute som useful values
    float3 lightDir = lightPos.xyz - pixelPos.xyz;
    float lightDistance = length(lightDir);
    lightDir = normalize(lightDir);
	
    float NdL = saturate(dot(normal, lightDir));
    float lambert = NdL; // Angle attenuation
    float NdV = saturate(dot(normal, viewDir));
    float3 h = normalize(lightDir + viewDir);
    float NdH = saturate(dot(normal, h));
    float a = max(0.001f, roughness * roughness);

    float3 cDiff = Diffuse(albedoColor);
    float3 cSpec = Specular(specularColor, h, viewDir, a, NdL, NdV, NdH);

    float linearAttenuation = lightDistance / lightRange;
    linearAttenuation = 1.0f - linearAttenuation;
    linearAttenuation = saturate(linearAttenuation);
    float physicalAttenuation = saturate(1.0f / (lightDistance * lightDistance));
    float ue4Attenuation = ((pow(saturate(1 - pow(lightDistance / lightRange, 4.0f)), 2.0f)) / (pow(lightDistance, 2.0f) + 1)); // Unreal Engine 4 attenuation
    float attenuation = lambert * linearAttenuation * physicalAttenuation;
    attenuation = ue4Attenuation * lambert;

    return saturate(lightColor * lightIntensity * attenuation * ((cDiff * (1.0 - cSpec) + cSpec) * PI));
}

//@function EvaluateSpotLight
float3 EvaluateSpotLight(float3 albedoColor, float3 specularColor, float3 normal,
    float roughness, float3 lightColor, float lightIntensity, float lightRange,
    float3 lightPos, float3 lightDir, float outerAngle, float innerAngle, float3 viewDir, float3 pixelPos)
{
    float3 toLight = lightPos.xyz - pixelPos.xyz;
    float lightDistance = length(toLight);
    toLight = normalize(toLight);

    float NdL = saturate(dot(normal, toLight));
    float lambert = NdL; // Angle attenuation
    float NdV = saturate(dot(normal, viewDir));
    float3 h = normalize(toLight + viewDir);
    float NdH = saturate(dot(normal, h));
    float a = max(0.001f, roughness * roughness);

    float3 cDiff = Diffuse(albedoColor);
    float3 cSpec = Specular(specularColor, h, viewDir, a, NdL, NdV, NdH);

    float cosOuterAngle = cos(outerAngle);
    float cosInnerAngle = cos(innerAngle);
    float3 lightDirection = lightDir;

    // Determine if pixel is within cone.
    float theta = dot(toLight, normalize(-lightDirection));
	// And if we're in the inner or outer radius.
    float epsilon = cosInnerAngle - cosOuterAngle;
    float intensity = clamp((theta - cosOuterAngle) / epsilon, 0.0f, 1.0f);
    intensity *= intensity;
	
    float ue4Attenuation = ((pow(saturate(1 - pow(lightDistance / lightRange, 4.0f)), 2.0f)) / (pow(lightDistance, 2.0f) + 1)); // Unreal Engine 4 attenuation
	float finalAttenuation = lambert * intensity * ue4Attenuation;

    return saturate(lightColor * lightIntensity * lambert * finalAttenuation * ((cDiff * (1.0 - cSpec) + cSpec) * PI));
}

//@function TBN
float3x3 TBN(float3 normal, float3 tangent, float3 bitangent)
{
    float3x3 TBN = float3x3(
		normalize(tangent.xyz),
		normalize(-bitangent.xyz),
		normalize(normal.xyz)
	);
    TBN = transpose(TBN);
    return TBN;
}

//@function Normalize
float2 Normalize(float aVector)
{
    return normalize(aVector);
}

//@function Normalize
float2 Normalize(float2 aVector)
{
    return normalize(aVector);
}

//@function Normalize
float3 Normalize(float3 aVector)
{
    return normalize(aVector);
}

//@function Normalize
float4 Normalize(float4 aVector)
{
    return normalize(aVector);
}

//@function UnpackNormal
float4 UnpackNormal(float4 normal)
{
    float3 workingNormal = float3(normal.rg, 1.0f);

    workingNormal = 2.0f * workingNormal - 1.0f;
    workingNormal.z = sqrt(1 - saturate(workingNormal.x * workingNormal.x + workingNormal.y * workingNormal.y));
    workingNormal = normalize(workingNormal);
    return float4(workingNormal, 1.0f);
}

//@function PackNormal
float4 PackNormal(float3 normal)
{
    return float4(0.5f + 0.5f * normal, 1.0f);
}


//@function Forward
float4 Forward(float4x4 a)
{
    return a._31;
}

//@function Right
float4 Right(float4x4 a)
{
    return a._11;
}

//@function Up
float4 Up(float4x4 a)
{
    return a._21;
}

//@function Position
float4 Position(float4x4 a)
{
    return a._41;
}

//@function Fresnel
float4 Fresnel(float4 colour, float4 worldNormal, float4 viewDirection, float exponent)
{
    float fresnel = dot(worldNormal.xyz, viewDirection.xyz);
    fresnel = saturate(1.0f - fresnel);
    fresnel = pow(fresnel, exponent);
    return float4(colour.rgb * fresnel, colour.a);
}


// https://en.wikipedia.org/wiki/File:Equirectangular_projection_SW.jpg
//@function sphere2mapUV_Equirectangular
float2 sphere2mapUV_Equirectangular(float3 p)
{
    return float2(
        atan2(p.x, -p.z) / (2 * 3.14159265f) + .5,
        p.y * .5 + .5
    );
}

// https://en.wikipedia.org/wiki/File:Lambert_cylindrical_equal-area_projection_SW.jpg
//@function sphere2mapUV_EqualArea
float2 sphere2mapUV_EqualArea(float3 p)
{
    return float2(
        (atan2(p.x, -p.z) / 3.14159265f + 1) / 2,
        asin(p.y) / PI + .5
    );
}


//@
