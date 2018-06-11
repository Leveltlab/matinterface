#include "stdafx.h"
#include "Device.h"

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT Device::InitDevice( HWND hwnd)
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( hwnd, &rc );
    m_width = rc.right - rc.left;
    m_height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
		D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

	m_format = DXGI_FORMAT_R8G8B8A8_UNORM; //DXGI_FORMAT_R10G10B10A2_UNORM; //DXGI_FORMAT_R16G16B16A16_FLOAT; //

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = m_width;
    sd.BufferDesc.Height = m_height;
	sd.BufferDesc.Format = m_format;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;


    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, m_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &m_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;


	//determine multithreading capabilities
	//D3D11_FEATURE_DATA_THREADING fdt;
	//ZeroMemory( &fdt, sizeof(fdt) );
	//hr = g_pd3dDevice->CheckFeatureSupport( D3D11_FEATURE_THREADING,  &fdt, sizeof(fdt));

    // Create a render target view
   ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
	pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

	g_vp.Width = (FLOAT)m_width;
    g_vp.Height = (FLOAT)m_height;
    g_vp.MinDepth = 0.0f;
    g_vp.MaxDepth = 1.0f;
    g_vp.TopLeftX = 0;
    g_vp.TopLeftY = 0;

	g_pImmediateContext->RSSetViewports( 1, &g_vp );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void Device::ClearDevice()
{

    SAFE_RELEASE( g_pRenderTargetView );
    SAFE_RELEASE( g_pSwapChain );
	SAFE_RELEASE( g_pImmediateContext );
    SAFE_RELEASE( g_pd3dDevice );
	//SAFE_RELEASE( m_pBackBuffer);
	
}


HRESULT Device::Resize(HWND hwnd)
{
	UINT width, height;
	HRESULT hr = S_OK;

	if (g_pSwapChain != NULL)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;

		if (g_vp.Width == width && g_vp.Height == height){
			return hr;

		} else {

			//SAFE_RELEASE(m_pBackBuffer);
			g_pImmediateContext->ClearState();
			g_pImmediateContext->OMSetRenderTargets(0, 0, 0);
			g_pRenderTargetView->Release();

			// Preserve the existing buffer count and format.
			// Automatically choose the width and height to match the client rect for HWNDs.
			hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			if (FAILED(hr))
				return hr;

			ID3D11Texture2D* pBackBuffer = NULL;
			hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			if (FAILED(hr))
				return hr;

			hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
			pBackBuffer->Release();
			if (FAILED(hr))
				return hr;

			g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

			g_vp.Width = (FLOAT)width;
			g_vp.Height = (FLOAT)height;
			g_vp.MinDepth = 0.0f;
			g_vp.MaxDepth = 1.0f;
			g_vp.TopLeftX = 0;
			g_vp.TopLeftY = 0;

			g_pImmediateContext->RSSetViewports(1, &g_vp);
		}

		return hr;

	}
	return S_FALSE;
}

