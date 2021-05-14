#define PBR_RS \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS )," \
"DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t1), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t2), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t3), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t4), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t5), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t6), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t7), visibility = SHADER_VISIBILITY_PIXEL ),"\
"CBV(b0),"\
"CBV(b1),"\
"StaticSampler(s0," \
    "addressU = TEXTURE_ADDRESS_WRAP," \
    "addressV = TEXTURE_ADDRESS_WRAP," \
    "addressW = TEXTURE_ADDRESS_WRAP," \
    "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
    "filter = FILTER_ANISOTROPIC, "\
    "visibility = SHADER_VISIBILITY_PIXEL),"\
"StaticSampler(s1,"\
    "addressU = TEXTURE_ADDRESS_CLAMP," \
    "addressV = TEXTURE_ADDRESS_CLAMP," \
    "filter = FILTER_MIN_MAG_MIP_LINEAR,"\
    "visibility = SHADER_VISIBILITY_PIXEL)"

#define PBR_RS2 \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS )," \
"DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t1), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t2), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t3), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t4), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t5), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t6), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t7), visibility = SHADER_VISIBILITY_PIXEL ),"\
"DescriptorTable ( SRV(t8), visibility = SHADER_VISIBILITY_PIXEL ),"\
"CBV(b0),"\
"CBV(b1),"\
"StaticSampler(s0," \
    "addressU = TEXTURE_ADDRESS_WRAP," \
    "addressV = TEXTURE_ADDRESS_WRAP," \
    "addressW = TEXTURE_ADDRESS_WRAP," \
    "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
    "filter = FILTER_MIN_MAG_MIP_POINT, "\
    "visibility = SHADER_VISIBILITY_PIXEL),"\
"StaticSampler(s1,"\
    "addressU = TEXTURE_ADDRESS_CLAMP," \
    "addressV = TEXTURE_ADDRESS_CLAMP," \
    "filter = FILTER_MIN_MAG_MIP_LINEAR,"\
    "visibility = SHADER_VISIBILITY_PIXEL)"

#define Skybox_RS \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS )," \
"RootConstants(num32BitConstants=33, b0), " \
"DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_PIXEL ),"\
"StaticSampler(s0," \
    "addressU = TEXTURE_ADDRESS_WRAP," \
    "addressV = TEXTURE_ADDRESS_WRAP," \
    "addressW = TEXTURE_ADDRESS_WRAP," \
    "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
    "filter = FILTER_ANISOTROPIC, "\
    "visibility = SHADER_VISIBILITY_PIXEL)"
