
#define ClearUAV_RootSignature \
    "RootFlags(0), " \
    "CBV(b0)," \
    "DescriptorTable(UAV(u0, numDescriptors = 5, flags = DESCRIPTORS_VOLATILE))" 

#define BLOCK_DIM 8
#define BLOCK_SIZE BLOCK_DIM * BLOCK_DIM

struct CS_Input 
{
    uint3 GroupID           : SV_GroupID;
    uint3 GroupThreadID     : SV_GroupThreadID ;
    uint3 DispatchThreadID  : SV_DispatchThreadID;
    uint GroupIndex         : SV_GroupIndex ;
};

cbuffer ClearUAVCB: register(b0)
{
    uint ClearValue;
    uint TextureIndex;
    uint DispatchWidth;
}

RWBuffer<uint> UAVBuffers[5]: register(u0);
 
[RootSignature( ClearUAV_RootSignature )]
[numthreads( BLOCK_SIZE, 1, 1 )]
void CS_Main( CS_Input IN )
{   
    UAVBuffers[TextureIndex][IN.DispatchThreadID.x] = ClearValue;
    return;
}