struct VSInput {
    [[vk::location(0)]]
    float3 position;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 1.f);
    return output;
}