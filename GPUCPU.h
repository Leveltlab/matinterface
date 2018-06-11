
struct GpuParam
{
	UINT NumCol;
	UINT NumRows;
	UINT SampleWidth;
	UINT NumWide;
	UINT* pFilter;
	bool bUni;
	WCHAR* strShader;
};

class GpuCpu {

	//just for rendering the image on our render targetview
	ID3D11VertexShader*     m_pVertexShader;
	ID3D11PixelShader*      m_pPixelShader;
	ID3D11InputLayout*		m_pQuadVertexLayout;
	ID3D11Buffer*			m_pQuadVB;
	ID3D11Buffer*			m_pQuadIB;

	//compute buffers
	ID3D11Buffer*				g_pIBuffer; //unordered access structured (readin) buffer
	ID3D11ShaderResourceView*	g_pIBufferSRV; //it's shader resource view

	ID3D11Buffer*				g_pOBuffer;  //unordered access structured (output) buffer 
	ID3D11UnorderedAccessView*	g_pOBufferUAV; //unorderd access view
	ID3D11ShaderResourceView*	g_pOBufferSRV;

	ID3D11Buffer*			   g_pImgBuffer; //UINT16 buffer with images separated  
	ID3D11UnorderedAccessView* g_pImgBufferUAV;

	ID3D11Buffer*			pBuffStaging; //To CPU copy buffer

	ID3D11Buffer*			g_pFBuffer;  //static buffer (filter) 
	ID3D11ShaderResourceView* g_pFBufferSRV; //it's shader resource view

	ID3D11Buffer*			g_pCBTex;  //constant buffer with dimensions

	ID3D11ComputeShader*    m_pComputeSampleShader;

	UINT  m_NmCol;
	UINT m_NmRow;
	UINT m_NmWide;
	UINT m_SampleLine;
	UINT m_NmDataLines;
	short m_Shift;

	float m_SclG;
	float m_SclR;
	float m_OffG;
	float m_OffR;
	bool m_bUni;

	HRESULT CreateMybuffers(ID3D11Device* pDevice, UINT* pFilter);
	HRESULT Rendersetup(ID3D11Device* pd3dDevice, LPCWSTR ShaderfileStr);
	HRESULT CreateMyComputeShader(ID3D11Device* pd3dDevice, LPCWSTR ShaderfileStr);
	void UpdateCB(ID3D11DeviceContext* pContext);

public:

	GpuCpu() :
		m_pVertexShader(NULL), m_pPixelShader(NULL), m_pQuadVertexLayout(NULL), m_pQuadVB(NULL), m_pQuadIB(NULL),
		g_pIBuffer(NULL), g_pIBufferSRV(NULL), g_pOBuffer(NULL), g_pOBufferUAV(NULL), g_pOBufferSRV(NULL), g_pImgBuffer(NULL), g_pImgBufferUAV(NULL),
		pBuffStaging(NULL), g_pFBuffer(NULL), g_pFBufferSRV(NULL), g_pCBTex(NULL), m_pComputeSampleShader(NULL)
	{
		m_SclG = -0.00001f; //5 * smallest value
		m_SclR = -0.00001f;
		m_OffG = 0.0f;
		m_OffR = 0.0f;
	}

	~GpuCpu() {
		GClear(); 
	}

	void GClear();
	
	void SetScL(float Scl, bool bCOL, ID3D11DeviceContext* pContext);
	void SetSOff(float Off, bool bCOL, ID3D11DeviceContext* pContext);
	void UpdateShift(short shift, ID3D11DeviceContext* pContext);

	void toGPU(ID3D11DeviceContext* pContext, UINT* pBufIn);
	void toCPU(ID3D11DeviceContext* pContext, UINT* Image);

	void Compute(ID3D11DeviceContext* mDeviceContext);
	void Render(ID3D11DeviceContext* pContext);

	HRESULT Init(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, GpuParam Par);
};
