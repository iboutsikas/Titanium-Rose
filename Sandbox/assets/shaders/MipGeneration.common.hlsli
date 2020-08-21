static const float gamma = 2.2;


#define RS\
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 8), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(UAV(u1, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(UAV(u2, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(UAV(u3, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)"

cbuffer GernerateMipsCB: register(b0)
{
    float4  TexelSize;      // width, height, 1 / dstwidth, 1 / dstheight
    uint    SrcMipLevel;    // Texture level of source mip
    uint    NumMipLevels;   // Number of OutMips to write: [1-4]
    uint2   __padding;
}


#define BLOCK_SIZE 8
