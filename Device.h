


class Device 
{

	D3D_DRIVER_TYPE         m_driverType;
	D3D_FEATURE_LEVEL       m_featureLevel;
	ID3D11Device*           g_pd3dDevice;
	ID3D11DeviceContext*    g_pImmediateContext;

	//ID3D11Texture2D*		m_pBackBuffer;
	IDXGISwapChain*         g_pSwapChain;

	ID3D11RenderTargetView* g_pRenderTargetView;
	D3D11_VIEWPORT			g_vp;

	UINT m_width, m_height;

	public:


	DXGI_FORMAT             m_format;

	Device() : m_driverType(D3D_DRIVER_TYPE_NULL), m_featureLevel(D3D_FEATURE_LEVEL_11_0),
		g_pd3dDevice(NULL), g_pImmediateContext(NULL), g_pSwapChain(NULL), g_pRenderTargetView(NULL)
		//m_pBackBuffer(NULL)
    {
		//create all objects
    }

	~Device()
	{
		ClearDevice();

	}

	ID3D11Device* GetDevice() {
		return g_pd3dDevice;
	}

	ID3D11DeviceContext* GetContext(){
		return g_pImmediateContext;
	}

		
	void Present(){
		if (m_width > 0 && m_height > 0){
			//g_pSwapChain->Present(1, 0); //run at monitor rate
			g_pSwapChain->Present(0, 0); //run at highest possible rate, 
			//we have to wait for the ALazar input anyway
		}
		else
			Sleep(500);  //window has been minimized
			
		}


		HRESULT InitDevice(HWND hwnd);
		void ClearDevice();
		HRESULT Resize(HWND hwnd);

};