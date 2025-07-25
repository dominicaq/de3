cbuffer MVPBuffer : register(b0) {
    row_major float4x4 mvp;
};

struct VSInput {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.position = mul(float4(input.position, 1.0), mvp);
    output.color = input.color;
    return output;
}
