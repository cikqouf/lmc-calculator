struct Ubo
{
    float2 iResolution;
    float iTime;
    float iTimeDelta;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Ubo> gUbo;

struct FSInput
{
    float4 position : SV_Position;
};

struct sdfResult
{
    float sdf;
    int id;
};

sdfResult sdfSphere(float3 pos, float radius, float3 Opos)
{
    return { distance(pos, Opos) - radius, 0 };
}

sdfResult sdfCuboid(float3 pos, float3 pars, float3 Opos, int id)
{
    float3 halfSize = pars * 0.5;

    float3 p = pos - Opos;
    float3 q = abs(p) - halfSize;

    float sdf = length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);

    return { sdf, id };
}

sdfResult sdfScene(float3 pos)
{
    sdfResult res1 = sdfSphere(pos, 1, float3(0, -1.5, 0));
    sdfResult res2 = sdfCuboid(pos, float3(1, 5, 2), float3(3, 0, 0), 1);
    sdfResult res3 = sdfCuboid(pos, float3(1, 5, 2), float3(-3, 0, 0), 2);
    sdfResult res4 = sdfCuboid(pos, float3(5, 1, 3), float3(0, -3, 0), 3);
    sdfResult res5 = sdfCuboid(pos, float3(5, 1, 3), float3(0, 3, 0), 4);
    sdfResult res6 = sdfCuboid(pos, float3(5, 5, 1), float3(0, 0, -2.5), 5);

    sdfResult res = res1;
    if (res2.sdf < res.sdf)
        res = res2;
    if (res3.sdf < res.sdf)
        res = res3;
    if (res4.sdf < res.sdf)
        res = res4;
    if (res5.sdf < res.sdf)
        res = res5;
    if (res6.sdf < res.sdf)
        res = res6;

    return res;
}

float3 getMaterialColor(int id)
{
    if (id == 0)
        return float3(1.0, 0.0, 0.0);
    if (id == 1)
        return float3(0.7, 0.5, 0.2);
    if (id == 2)
        return float3(1.0, 0.35, 0.0);
    if (id == 3)
        return float3(0.55, 0.1, 0.8);
    if (id == 4)
        return float3(0.1, 0.8, 0.25);
    if (id == 5)
        return float3(0.1, 0.35, 1.0);

    return float3(0, 0, 0);
}

float3 rayMarch(float3 rayDestination, float3 rayDirection, float3 lightPos, int fix)
{
    float varPhi = 2 * gUbo.iTime;
    float3x3 rotxz =
        float3x3(float3(cos(varPhi), 0, -sin(varPhi)), float3(0, 1, 0), float3(sin(varPhi), 0, cos(varPhi)));

    if (fix == 0)
    {
        lightPos = mul(rotxz, lightPos);
    }

    float3 color = float3(0, 0, 0);
    float3 rayEnd = rayDestination;

    [unroll]
    for (float t = 0; t < 64; ++t)
    {
        float step0 = sdfScene(rayEnd).sdf;
        rayEnd = rayEnd + rayDirection * step0;

        sdfResult stepResult = sdfScene(rayEnd);
        if (stepResult.sdf < 0.001)
        {
            // get surface Norm // //
            float3 sceneNorm = getSceneNormVec(rayEnd);
            float3 lightDirection = normalize(lightPos - rayEnd);

            // lambert 颜色输出 ///
            float cosTheta = max(dot(lightDirection, sceneNorm), 0);

            color = getMaterialColor(stepResult.id);

            color *= cosTheta;

            // 阴影检测 //////
            float3 shadowEnd = rayEnd + sceneNorm * .01;
            float shadowEndDistToLight = distance(shadowEnd, lightPos);
            float shadowLimit = 0;

            for (int tt = 0; tt < 64; ++tt)
            {
                float step1 = sdfScene(shadowEnd).sdf;
                shadowEnd = shadowEnd + lightDirection * step1;
                sdfResult stepShadowResult = sdfScene(shadowEnd);

                if (stepShadowResult.sdf < 0.001 && shadowLimit < shadowEndDistToLight)
                {
                    color = float3(0, 0, 0);
                    break;
                }
                shadowLimit += step1;
            }

            // 反射混合 ///////
            float3 reflectEnd = rayEnd + sceneNorm * 0.01;
            float3 reflectDirection = normalize(reflect(rayDirection, sceneNorm));
            float3 reflectColor = float3(0, 0, 0);

            float3 fresnel = pow(1 - max(dot(-rayDirection, sceneNorm), 0), 5);

            const int RCOUNT = 4;
            float roughness = 0.25;

            for (int ri = 0; ri < RCOUNT; ri++)
            {
                float angle = hash(ri * 12.9898) * 6.2831853;
                float r = hash(ri * 78.233) * roughness;

                float3 jitter = float3(r * cos(angle), r * sin(angle), 0);

                float3 reflectDirectionJittered = normalize(reflectDirection + jitter);
                float3 reflectEndJ = reflectEnd;

                for (int rr = 0; rr < 64; rr++)
                {
                    float stepJ = sdfScene(reflectEndJ).sdf;
                    reflectEndJ += reflectDirectionJittered * stepJ;

                    sdfResult hitJ = sdfScene(reflectEndJ);
                    if (hitJ.sdf < 0.001)
                    {
                        float3 normJ = getSceneNormVec(reflectEndJ);
                        float3 lightDirJ = normalize(lightPos - reflectEndJ);
                        float cosJ = max(dot(lightDirJ, normJ), 0);

                        float3 cJ = getMaterialColor(hitJ.id);

                        cJ *= cosJ;

                        float3 shadowEndJ = reflectEndJ + normJ * 0.05;
                        float distJ = distance(shadowEndJ, lightPos);
                        float limitJ = 0;

                        for (int ss = 0; ss < 64; ss++)
                        {
                            float stepS = sdfScene(shadowEndJ).sdf;
                            shadowEndJ += lightDirJ * stepS;

                            sdfResult shadowHitJ = sdfScene(shadowEndJ);

                            if (shadowHitJ.sdf < 0.001 && limitJ < distJ)
                            {
                                cJ = float3(0, 0, 0);
                                break;
                            }

                            limitJ += stepS;
                        }

                        reflectColor += cJ;
                        break;
                    }
                }
            }

            reflectColor /= RCOUNT;

            color = lerp(color, reflectColor, fresnel * 10);

            break;
        }
    }

    return color;
}

float3 getSceneNormVec(float3 pos)
{
    float epsilon = 0.001;
    float dx = sdfScene(pos + float3(epsilon, 0, 0)).sdf - sdfScene(pos - float3(epsilon, 0, 0)).sdf;
    float dy = sdfScene(pos + float3(0, epsilon, 0)).sdf - sdfScene(pos - float3(0, epsilon, 0)).sdf;
    float dz = sdfScene(pos + float3(0, 0, epsilon)).sdf - sdfScene(pos - float3(0, 0, epsilon)).sdf;

    return normalize(float3(dx, dy, dz));
}

float hash(float n)
{
    return frac(sin(n) * 43758.5453123);
}

[shader("fragment")]
float4 main(FSInput fsInput) : SV_Target0
{
    float2 fragCoord = fsInput.position.xy;
    fragCoord.y = gUbo.iResolution.y - fragCoord.y;

    float aspect = gUbo.iResolution.x / gUbo.iResolution.y;
    float2 uv = (fragCoord * 2.0 - gUbo.iResolution) / gUbo.iResolution.xy;
    uv.x *= aspect;

    float3 bgColor = float3(0, 0, 0);

    /*
     * 反向光线追踪 depth 变小则进入屏幕, 反之离开屏幕
     */

    const float PI = 3.1415926;

    float phi = .3 * gUbo.iTime;
    float3x3 rotz = float3x3(float3(cos(phi), 0, -sin(phi)), float3(0, 1, 0), float3(sin(phi), 0, cos(phi)));

    // /* viewport */ // 视点 //
    //
    // float3 rayDestination = float3(0, 0, 3);
    float3 rayDestination = mul(rotz, float3(0, 0, 10));

    // /* viewDirection */ // 视线 //
    //
    float3 rayDirection = normalize(mul(rotz, float3(uv.x, uv.y, -2)));
    // float3 rayDirection = float3(uv.x, uv.y, -1);

    // rayMarch //  //
    const int SAMPLE_COUNT = 8;
    float3 lightCenter = float3(0, .2, 4.2);
    float lightRadius = .25;

    float3 finalColor = float3(0, 0, 0);

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float angle = hash(i * 12.9898) * 6.2831853;
        float r = hash(i * 78.233) * lightRadius;

        float3 lightPos = lightCenter + float3(r * cos(angle), 0, r * sin(angle));

        finalColor += rayMarch(rayDestination, rayDirection, lightPos, 0);
    }

    // finalColor /= SAMPLE_COUNT;

    lightCenter = float3(.5, .6, 1.);

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float angle = hash(i * 12.9898) * 6.2831853;
        float r = hash(i * 78.233) * lightRadius;

        float3 lightPos = lightCenter + float3(r * cos(angle), 0, r * sin(angle));

        finalColor += rayMarch(rayDestination, rayDirection, lightPos, 1);
    }

    finalColor /= SAMPLE_COUNT * 2;

    finalColor += bgColor;
    return float4(finalColor, 1.0);
}
