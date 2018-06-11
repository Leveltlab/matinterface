
#include "stdafx.h"
#include "createshaders.h"
#include "mytypes.h"
#include "GPUCPU.h"

#define ICB0	0
//#define DIMX  512

struct QUAD_VERTEX
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct CBTEX
{
	UINT Width;
	UINT Height;
	UINT SmplLine;  //Number of pixels in sampling index array
	int Shift;    //pixel shift for sampling 
	float SclG;
	float SclR;
	float OffG;
	float OffR;

};

void GpuCpu::GClear()
{
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pQuadVertexLayout);
	SAFE_RELEASE(m_pQuadVB);
	SAFE_RELEASE(m_pQuadIB);
	SAFE_RELEASE(g_pIBuffer);
	SAFE_RELEASE(g_pIBufferSRV);
	SAFE_RELEASE(g_pOBuffer);
	SAFE_RELEASE(g_pOBufferUAV);
	SAFE_RELEASE(g_pOBufferSRV);
	SAFE_RELEASE(g_pImgBuffer);
	SAFE_RELEASE(g_pImgBufferUAV);
	SAFE_RELEASE(pBuffStaging);
	SAFE_RELEASE(g_pFBuffer);
	SAFE_RELEASE(g_pFBufferSRV);
	SAFE_RELEASE(g_pCBTex);
	SAFE_RELEASE(m_pComputeSampleShader);

}

HRESULT GpuCpu::Init(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, GpuParam Par)
{
	HRESULT hr = S_OK;

	GClear();

	m_NmCol = Par.NumCol; //number of columns in output image
	m_NmRow = Par.NumRows;
	m_NmWide = Par.NumWide; //number of samples in S function
	m_SampleLine = Par.SampleWidth; //number of samples in raw line
	m_Shift = 0; //initial shift should be zero, shift is applied to filter at initialization
	m_bUni = Par.bUni;


	if (m_bUni) m_NmDataLines = m_NmRow;  //unidirectional scan
	else m_NmDataLines = m_NmRow/ 2;  //bidirectional scan: m_NmDataLiness should now be half the actual number of Lines

	V_RETURN(CreateMybuffers(pd3dDevice, Par.pFilter));

	V_RETURN(CreateMyComputeShader(pd3dDevice, Par.strShader));

	V_RETURN(Rendersetup(pd3dDevice, Par.strShader));

	UpdateCB(pContext);

	return hr;
}


void GpuCpu::UpdateShift(short Shift, ID3D11DeviceContext* pContext)
{
	m_Shift += Shift;
	UpdateCB(pContext);
}


void GpuCpu::SetScL(float Scl, bool bCol, ID3D11DeviceContext* pContext)
{
	if (bCol)
	{
		m_SclG = Scl * 0.00001f;  //from UINT16 to float between 0 and 1.0
	}
	else
	{
		m_SclR = Scl * 0.00001f;
	}

	UpdateCB(pContext);
}

void GpuCpu::SetSOff(float Off, bool bCol, ID3D11DeviceContext* pContext)
{
	if (bCol)
	{
		m_OffG = Off * 0.01f; // = 1/32
	}
	else
	{
		m_OffR = Off * 0.01f;
	}

	UpdateCB(pContext);
}

HRESULT GpuCpu::Rendersetup(ID3D11Device* pd3dDevice, LPCWSTR strFilename)
{
	HRESULT hr = S_OK;


	//  Compiles the vertiex shader and then creates it for rendering a texture on a quad
	ID3DBlob* pVSBlob = NULL;
	LPCSTR pProfile = (pd3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "vs_5_0" : "vs_4_0";

	hr = CreateShaderFromFile(pd3dDevice, strFilename, NULL, NULL, "VS_Tex", pProfile, 0, 0,
		(ID3D11DeviceChild**)&m_pVertexShader, &pVSBlob);

	// Create vertex input layout for screen quad
	const D3D11_INPUT_ELEMENT_DESC quadlayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(quadlayout);

	hr = pd3dDevice->CreateInputLayout(quadlayout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pQuadVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;


	//pixel shader : compiles the shader from a file and then creates the pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = CreateShaderFromFile(pd3dDevice, strFilename, NULL, NULL, "PS_Tex", "ps_5_0", 0, 0,
		(ID3D11DeviceChild**)&m_pPixelShader, &pPSBlob);

	QUAD_VERTEX Verts[4];

	Verts[0].Pos = XMFLOAT3(-1, -1, 0.5);
	Verts[0].Tex = XMFLOAT2(0, 1);
	Verts[1].Pos = XMFLOAT3(-1, 1, 0.5);
	Verts[1].Tex = XMFLOAT2(0, 0);
	Verts[2].Pos = XMFLOAT3(1, -1, 0.5);
	Verts[2].Tex = XMFLOAT2(1, 1);
	Verts[3].Pos = XMFLOAT3(1, 1, 0.5);
	Verts[3].Tex = XMFLOAT2(1, 0);

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(QUAD_VERTEX) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;


	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = Verts;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pQuadVB));

	// Create index buffer
	WORD indices[] =
	{
		0, 1, 2,
		2, 1, 3
	};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 6;        // 6 vertices needed for 2 triangles in a triangle list
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pQuadIB));

	return hr;
}

void GpuCpu::Render(ID3D11DeviceContext* pContext)
{
	UINT stride = sizeof(QUAD_VERTEX);
	UINT offset = 0;
	pContext->IASetInputLayout(m_pQuadVertexLayout);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->IASetVertexBuffers(0, 1, &m_pQuadVB, &stride, &offset);
	pContext->IASetIndexBuffer(m_pQuadIB, DXGI_FORMAT_R16_UINT, 0);

	pContext->PSSetShaderResources(1, 1, &g_pOBufferSRV);

	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->PSSetShader(m_pPixelShader, NULL, 0);

	pContext->DrawIndexed(6, 0, 0);

	pContext->VSSetShader(NULL, NULL, 0);
	pContext->PSSetShader(NULL, NULL, 0);

	// Unbind the input textures from the PS 
	ID3D11ShaderResourceView* nullSRV[] = { NULL};
	pContext->PSSetShaderResources(1, 1, nullSRV);

}

HRESULT GpuCpu::CreateMybuffers(ID3D11Device* pDevice, UINT* pFilter)
{
	HRESULT hr = S_OK;

	// Create a buffer to be bound as Compute Shader input (D3D11_BIND_SHADER_RESOURCE)(GPU readonly, CPU write only)
	//the input samples are UINT16, the buffer is UINT32, each sample in the buffer is actually 2 two PMT (Greeen and Red) samples
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags =  D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = sizeof(UINT) * m_SampleLine * m_NmDataLines; //number of samples in input Buffer > 4 times output
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(UINT);

	V_RETURN(pDevice->CreateBuffer(&desc, NULL, &g_pIBuffer));
	V_RETURN(CreateBufferSRV(pDevice, g_pIBuffer, &g_pIBufferSRV));

	//output Buffer, with number of pixels for the two output images, for screen rendering
	V_RETURN(CreateStructuredBuffer(pDevice, sizeof(UINT), m_NmCol * m_NmRow, NULL, &g_pOBuffer, D3D11_USAGE_DEFAULT));
	V_RETURN(CreateBufferUAV(pDevice, g_pOBuffer, &g_pOBufferUAV));
	V_RETURN(CreateBufferSRV(pDevice, g_pOBuffer, &g_pOBufferSRV));

	//output Buffer, with number of pixels for the two output images but now separated in two pages for saving 
	V_RETURN(CreateStructuredBuffer(pDevice, sizeof(UINT), m_NmCol * m_NmRow, NULL, &g_pImgBuffer, D3D11_USAGE_DEFAULT));
	V_RETURN(CreateBufferUAV(pDevice, g_pImgBuffer, &g_pImgBufferUAV));


	//extra buffer to copy output to CPU memory
	V_RETURN(CreateStagingbuffer(pDevice, g_pImgBuffer, &pBuffStaging));

	//static filter buffer for sampling indices
	V_RETURN(CreateStructuredBuffer(pDevice, sizeof(UINT), m_NmWide, pFilter, &g_pFBuffer, D3D11_USAGE_DEFAULT));
	V_RETURN(CreateBufferSRV(pDevice, g_pFBuffer, &g_pFBufferSRV));

	//constant buffer to define dimensions 
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.ByteWidth = sizeof(CBTEX);
	V_RETURN(pDevice->CreateBuffer(&bd, NULL, &g_pCBTex));


	return hr;
}

void GpuCpu::UpdateCB(ID3D11DeviceContext* pContext)
{
	HRESULT hr = S_OK;

	CBTEX cbData;

	cbData.Width = m_NmCol;  //for each image
	cbData.Height = m_NmRow;
	cbData.SmplLine = m_SampleLine;  //samples in raw data line
	cbData.Shift = (int)m_Shift;
	cbData.SclG = m_SclG;
	cbData.SclR = m_SclR;
	cbData.OffG = m_OffG;
	cbData.OffR = m_OffR;

	//Set dimensions of texture in shader constant buffer, and make available to computeshaders
	pContext->UpdateSubresource(g_pCBTex, 0, NULL, &cbData, 0, 0);
	pContext->CSSetConstantBuffers(ICB0, 1, &g_pCBTex);
	pContext->PSSetConstantBuffers(ICB0, 1, &g_pCBTex);
}

//copying data to the CPU memory
void GpuCpu::toCPU(ID3D11DeviceContext* pContext, UINT* Image)
{
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	ZeroMemory(&MappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	pContext->CopyResource(pBuffStaging, g_pImgBuffer);

	pContext->Map(pBuffStaging, 0, D3D11_MAP_READ, 0, &MappedResource);
	memcpy((void*)Image, MappedResource.pData, sizeof(UINT) * m_NmCol * m_NmRow);
	pContext->Unmap(pBuffStaging, 0);

}

//copying data to the GPU memory
void GpuCpu::toGPU(ID3D11DeviceContext* pContext, UINT* pBufIn)
{
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	ZeroMemory(&MappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	pContext->Map(g_pIBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	memcpy(MappedResource.pData, (void*)pBufIn, sizeof(UINT)* m_SampleLine * m_NmDataLines); //linewidth * number of lines
	pContext->Unmap(g_pIBuffer, 0);

	//no need to copy resource; IBuffer already has an associated Shader resource view
	//pContext->CopyResource(g_pTexIB, inBuf); //copy to shader resource buffer

}

void GpuCpu::Compute(ID3D11DeviceContext* mDeviceContext)
{
	//each frame
	const UINT values[4] = {0};
	mDeviceContext->ClearUnorderedAccessViewUint(g_pImgBufferUAV, values);

	// Enable Compute Shader
	mDeviceContext->CSSetShader(m_pComputeSampleShader, nullptr, 0);

	ID3D11ShaderResourceView* SRViews[2] = { g_pFBufferSRV, g_pIBufferSRV };
	mDeviceContext->CSSetShaderResources(0, 2, SRViews);

	ID3D11UnorderedAccessView* UAViews[2] = { g_pOBufferUAV, g_pImgBufferUAV };
	mDeviceContext->CSSetUnorderedAccessViews(0, 2, UAViews, 0);

	// Dispatch
	mDeviceContext->Dispatch(m_NmWide, 1, 1);

	// Unbind the input textures from CS 
	ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL };
	mDeviceContext->CSSetShaderResources(0, 2, nullSRV);

	// Unbind output from compute shader
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL};
	mDeviceContext->CSSetUnorderedAccessViews(0, 2, nullUAV, 0);

	// Disable Compute Shader
	mDeviceContext->CSSetShader(nullptr, nullptr, 0);


	// Copy results                 Dest,                Src

}

HRESULT GpuCpu::CreateMyComputeShader(ID3D11Device* pd3dDevice, LPCWSTR ShaderfileStr)
{

	HRESULT hr = S_FALSE;

	LPCSTR pProfile = (pd3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

	if (m_bUni){
		const D3D_SHADER_MACRO pdef[] =
		{
			"UNI", "1",
			NULL, NULL
		};
		hr = CreateShaderFromFile(pd3dDevice, ShaderfileStr, pdef, NULL, "CS_Sample", pProfile, NULL, NULL,
			(ID3D11DeviceChild**)&m_pComputeSampleShader, NULL);

	}
	else {
		const D3D_SHADER_MACRO pdef[] =
		{
			"BI", "1",
			NULL, NULL
		};
		hr = CreateShaderFromFile(pd3dDevice, ShaderfileStr, pdef, NULL, "CS_Sample", pProfile, NULL, NULL,
			(ID3D11DeviceChild**)&m_pComputeSampleShader, NULL);

	}


	return hr;
}