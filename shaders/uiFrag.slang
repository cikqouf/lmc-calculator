struct vertOut
{
    float4 pos : SV_Position;

    [[vk::location(0)]]
    float2 uv;
};

struct fragOut
{
    float4 color : SV_Target;
};

[[vk::binding(0, 0)]]
SamplerState sampler;

[[vk::binding(1, 0)]]
texture2D tex;

[shader("fragment")]
fragOut main(vertOut vOut)
{
    fragOut out;

    float4 sampled = tex.Sample(sampler, vOut.uv);

    out.color = float4(1, 1, 1, sampled.x);
    return out;
}
