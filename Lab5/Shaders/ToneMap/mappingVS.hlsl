struct VS_INPUT {
    uint vertexId : SV_VertexID;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;

    float4 pos = float4(0, 0, 0, 0);

    switch (input.vertexId) {
    case 0:
        pos = float4(1, 1, 0, 1);
        break;
    case 1:
        pos = float4(1, -1, 0, 1);
        break;
    case 2:
        pos = float4(-1, -1, 0, 1);
        break;
    case 3:
        pos = float4(-1, 1, 0, 1);
        break;
    case 4:
        pos = float4(1, 1, 0, 1);
        break;
    case 5:
        pos = float4(-1, -1, 0, 1);
        break;
    }

    output.position = pos;
    output.uv = float2(pos.x * 0.5 + 0.5, 0.5 - pos.y * 0.5);

    return output;
}
