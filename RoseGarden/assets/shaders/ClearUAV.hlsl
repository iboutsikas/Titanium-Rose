
#define ClearUAV_RootSignature \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 1), " \
    "DescriptorTable(UAV(u0))" 

#define BLOCK_DIM 8
#define BLOCK_SIZE BLOCK_DIM * BLOCK_DIM

struct CS_Input 
{
    uint3 GroupID           : SV_GroupID;
    uint3 GroupThreadID     : SV_GroupThreadID ;
    uint3 DispatchThreadID  : SV_DispatchThreadID;
    uint GroupIndex         : SV_GroupIndex ;
};

cbuffer rootConstants: register(b0)
{
    uint ClearValue;
}

RWBuffer<uint> UAVBuffer: register(u0);
 
[RootSignature( ClearUAV_RootSignature )]
[numthreads( BLOCK_SIZE, 1, 1 )]
void CS_Main( CS_Input IN )
{   
    UAVBuffer[IN.DispatchThreadID.x] = ClearValue;
    return;
}