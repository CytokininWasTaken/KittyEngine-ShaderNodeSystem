//@function x
float x(float2 a)
{
    return a.x;
}

//@function x
float x(float3 a)
{
    return a.x;
}

//@function x
float x(float4 a)
{
    return a.x;
}


//@function y
float y(float2 a)
{
    return a.y;
}

//@function y
float y(float3 a)
{
    return a.y;
}

//@function y
float y(float4 a)
{
    return a.y;
}

//@function z
float z(float3 a)
{
    return a.z;
}

//@function z
float z(float4 a)
{
    return a.z;
}

//@function w
float w(float4 a)
{
    return a.w;
}


//@intrinsic dot
//#out  float  float  float  int  int  int
//#in a float2 float3 float4 int2 int3 int4
//#in b float2 float3 float4 int2 int3 int4

//@intrinsic cross
//#out  float3
//#in a float3
//#in b float3

//@intrinsic abs
//#out  float  float2  float3  float4 int  int2  int3  int4
//#in a float  float2  float3  float4 int  int2  int3  int4

//@intrinsic sin
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic cos
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic tan
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic asin
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic acos
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic atan
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic atan2
//#out  float float2 float3 float4
//#in y float float2 float3 float4
//#in x float float2 float3 float4

//@intrinsic sinh
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic cosh
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic tanh
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic floor
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic ceil
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic round
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic exp
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic log
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic reflect
//#out  float2 float3  float4
//#in i float2 float3  float4
//#in n float2 float3  float4

//@intrinsic refract
//#out  float2 float3  float4
//#in i float2 float3  float4
//#in n float2 float3  float4
//#in index float float float

//@intrinsic length
//#out  float  float  float
//#in a float2 float3 float4

//@intrinsic distance
//#out  float  float  float
//#in a float2 float3 float4
//#in b float2 float3 float4

//@intrinsic normalize
//#out  float2 float3 float4
//#in a float2 float3 float4

//@intrinsic lerp
//#out  float  float2  float3  float4  float4  float3  float2
//#in a float  float2  float3  float4  float4  float3  float2
//#in b float  float2  float3  float4  float4  float3  float2
//#in t float  float2  float3  float4  float   float   float

//@intrinsic saturate
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic smoothstep
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4
//#in b float  float2  float3  float4
//#in t float  float2  float3  float4

//@intrinsic step
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4
//#in b float  float2  float3  float4

//@intrinsic sqrt
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@intrinsic rsqrt
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4

//@operator add +
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4
//#in b float  float2  float3  float4

//@operator sub -
//#out  float  float2  float3  float4
//#in a float  float2  float3  float4
//#in b float  float2  float3  float4

//@operator mul *
//#out  float float2 float3 float4 float2 float3 float4 float2 float3 float4 float3   float4   float3   float4
//#in a float float2 float3 float4 float2 float3 float4 float  float  float  float3   float4   float3x3 float4x4
//#in b float float2 float3 float4 float  float  float  float2 float3 float4 float3x3 float4x4 float3   float4

//@operator div /
//#out  float float2 float3 float4 float2 float3 float4 float2 float3 float4
//#in a float float2 float3 float4 float2 float3 float4 float  float  float
//#in b float float2 float3 float4 float  float  float  float2 float3 float4

//@