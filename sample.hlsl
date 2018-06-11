
//External defines
#ifdef UNI
#define YDIM 512   //image height

#else
#define YDIM 256

#endif

#define LW 0xFFFF
#define HW 0xFFFF0000

StructuredBuffer<uint> Filter : register(t0);
StructuredBuffer<uint> InputBuf : register(t1);

RWStructuredBuffer<uint> OutBuffer: register(u0);
RWStructuredBuffer<uint> ImgBuffer: register(u1);

cbuffer cbGlobals : register(b0)
{
	uint Pixwide; //number of samples in image and in (Hor) filter
	uint Pixhigh;
	uint SampleLine;
	int Shift;
	float SclG;
	float SclR;
	float OffG;
	float OffR;
};

struct VS_STRUCT
{
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};

//just copy
PS_INPUT VS_Tex( VS_STRUCT input )
{
    PS_INPUT output = (PS_INPUT)0;
	output.Pos = float4(input.Pos.xyz, 1);
	output.Tex = input.Tex;


	return output;
}

float4 PS_Tex( PS_INPUT input) : SV_Target
{

	float xp = input.Tex.x;
	float yp = input.Tex.y;
	uint crd = floor(xp*Pixwide) + floor(yp*Pixhigh)*Pixwide;
	float Green = SclG * (65536 - (InputBuf[crd] & LW)) + OffG;
	float Red = SclR * (65536 - ((InputBuf[crd] & HW) >> 16)) + OffR;
	

	return float4(Red, Green, 0, 1);

}



#ifdef UNI   //unidirectional scanning
////////////////////////compute shader/////////////////////////////
[numthreads(1, YDIM, 1)]
void CS_Sample(uint3 i : SV_DispatchThreadID)
{

	uint idx;
	uint Green;
	uint Red;
	//get indices and sample from input
	uint Fvidx = Filter[i.x] + i.y*SampleLine;
	for(uint j = 0; j < 4; j++){
		idx = Fvidx + j;  //SampleLine should be greater 5000 or 9000
		Green += (InputBuf[idx] & LW) >> 2;
		Red   += (InputBuf[idx] & HW) >> 2;
	}

	//sum 4 samples for the two individual images
	uint bp = i.x + i.y*Pixwide; //x = threadgroupnum, y = groupthreadId 
	OutBuffer[bp] = Red + Green; // S[j].Green;

	//separate red and green in two blocks. But buffer consists of uint32's, whereas samples are only uint16. The uint32's are filled by uint16's from different threads
	uint Out;
	if (bp % 2){ //atomic operation or
		InterlockedOr(ImgBuffer[bp / 2], Green << 16, Out);
		InterlockedOr(ImgBuffer[bp / 2 + Pixwide*YDIM/2], Red, Out);
	}
	else {
		InterlockedOr(ImgBuffer[bp / 2], Green, Out);
		InterlockedOr(ImgBuffer[bp / 2 + Pixwide*YDIM/2], Red >> 16, Out);
	}

 }

#else  //bidirectional scanning

[numthreads(1, YDIM, 1)]
void CS_Sample(uint3 i : SV_DispatchThreadID)
{

	uint idx;
	uint Green;
	uint Red;
	uint bp;
	//get indices and sample from input
	//xdim is number samples in filter , but less than twice the output image width
	//because it's back and forth. Shift can be positive or negative!
	uint Fvidx = Filter[i.x] + i.y*SampleLine + Shift; 
	for (uint j = 0; j < 4; j++){
		idx = Fvidx + j; //SampleLine should be greater 5000 or 9000
		Green += (InputBuf[idx] & LW) >> 2;
		Red += (InputBuf[idx] & HW) >> 2;
	}

	//sum 4 samples for the two individual images
	//there are twice the lines of the input in the output due to bidirectional scannning
	if (i.x < Pixwide)
		bp = i.y * 2 * Pixwide + i.x; //2*number of lines; increasing index
	else  //i.x goes from Pixwide to 2* pixwide-1, bp decreases from pixwide-1 to 0 on line i.y*2 + 1
		bp = (i.y * 2 + 3) * Pixwide - (i.x + 1); //but never reaches zero because the filter is less than two rows long.

	OutBuffer[bp] = Red + Green; // S[j].Green;

    uint Out;
	if (bp % 2){
		InterlockedOr(ImgBuffer[bp / 2], Green << 16, Out);
		InterlockedOr(ImgBuffer[bp / 2 + Pixwide*YDIM], Red, Out);
	}
	else {
		InterlockedOr(ImgBuffer[bp / 2], Green, Out);
		InterlockedOr(ImgBuffer[bp / 2 + Pixwide*YDIM], Red >> 16, Out);
	}

}

#endif