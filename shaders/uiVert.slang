struct InstanceDataInput
{
    float2 offset;
    float2 uvOffset;
};

struct VertexInput
{

    [[vk::location(0)]]
    float2 pos;
    [[vk::location(1)]]
    float2 uv;

    [[vk::location(2)]]
    InstanceDataInput instanceInputData;
};

struct VertexOutput
{

    float4 pos : SV_Position;

    [[vk::location(0)]]
    float2 uv;
};

[shader("vertex")]
VertexOutput main(VertexInput in)
{
    VertexOutput out;

    float2 posOffset = in.instanceInputData.offset;
    float2 uvOffset = in.instanceInputData.uvOffset;

    out.pos = float4(in.pos + posOffset, 0, 1);
    out.uv = in.uv + uvOffset;

    return out;
}
