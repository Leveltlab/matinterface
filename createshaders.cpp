#include "stdafx.h"

#include "mytypes.h"
#include "createshaders.h"
//#include <xnamath.h>


HRESULT CreateShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                              LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                             ID3D11DeviceChild** ppShader, ID3DBlob** ppShaderBlob ) // ID3DX11ThreadPump* pPump,
{
    HRESULT   hr = S_OK;
    ID3DBlob* pShaderBlob = NULL;
    ID3DBlob* pErrorBlob = NULL;

    WCHAR strSF[MAX_PATH];
    hr = FindDXSDKShaderFileCch( strSF, MAX_PATH, pSrcFile );
    if ( FAILED(hr) )
        return hr;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | Flags1;
	#if defined( DEBUG ) || defined( _DEBUG )
	    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG | D3D10_SHADER_DEBUG;
	#endif
   // DXUTFindDXSDKMediaFileCch( wcFullPath, 256, pSrcFile );


    // Compile shader into binary blob
    //hr = D3DX11CompileFromFile( pSrcFile, pDefines, pInclude, pFunctionName, pProfile, 
    //                            dwShaderFlags, Flags2, pPump, &pShaderBlob, &pErrorBlob, NULL );

	hr = D3DCompileFromFile( pSrcFile, pDefines, pInclude, pFunctionName, pProfile,
			                     dwShaderFlags, Flags2, &pShaderBlob, &pErrorBlob );

    if( FAILED( hr ) )
    {
        OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    
    // Create shader from binary blob
    if ( ppShader )
    {
        hr = E_FAIL;
        if ( strstr( pProfile, "vs" ) )
        {
            hr = pd3dDevice->CreateVertexShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11VertexShader**)ppShader );
        }
        else if ( strstr( pProfile, "hs" ) )
        {
            hr = pd3dDevice->CreateHullShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11HullShader**)ppShader ); 
        }
        else if ( strstr( pProfile, "ds" ) )
        {
            hr = pd3dDevice->CreateDomainShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11DomainShader**)ppShader );
        }
        else if ( strstr( pProfile, "gs" ) )
        {
            hr = pd3dDevice->CreateGeometryShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11GeometryShader**)ppShader ); 
        }
        else if ( strstr( pProfile, "ps" ) )
        {
            hr = pd3dDevice->CreatePixelShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11PixelShader**)ppShader ); 
        }
        else if ( strstr( pProfile, "cs" ) )
        {
            hr = pd3dDevice->CreateComputeShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11ComputeShader**)ppShader );
        }
        if ( FAILED( hr ) )
        {
            OutputDebugString( L"Shader creation failed\n" );
            SAFE_RELEASE( pErrorBlob );
            SAFE_RELEASE( pShaderBlob );
            return hr;
        }
    }

    //DXUT_SetDebugName( *ppShader, pFunctionName );
	if ( *ppShader )
        (*ppShader)->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(pFunctionName), pFunctionName );

    // If blob was requested then pass it otherwise release it
    if ( ppShaderBlob )
    {
        *ppShaderBlob = pShaderBlob;
    }
    else
    {
        pShaderBlob->Release();
    }

    // Return error code
    return hr;
}

HRESULT FindDXSDKShaderFileCch( __in_ecount(cchDest) WCHAR* strDestPath,
                                int cchDest, 
                                __in LPCWSTR strFilename )
{
    if( NULL == strFilename || strFilename[0] == 0 || NULL == strDestPath || cchDest < 10 )
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] =
    {
        0
    };
    WCHAR strExeName[MAX_PATH] =
    {
        0
    };
	WCHAR strCurrPath[MAX_PATH] =
    {
        0
    };
    WCHAR* strLastSlash = NULL;
    GetModuleFileName( NULL, strExePath, MAX_PATH );
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr( strExePath, TEXT( '\\' ) );
    if( strLastSlash )
    {
        wcscpy_s( strExeName, MAX_PATH, &strLastSlash[1] );

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExeName, TEXT( '.' ) );
        if( strLastSlash )
            *strLastSlash = 0;
    }
	
	 GetCurrentDirectory(MAX_PATH, strCurrPath);

    // Search in directories:
    //      .\
    //      %EXE_DIR%\..\..\%EXE_NAME%

    wcscpy_s( strDestPath, cchDest, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return true;

	wcscpy_s( strCurrPath, cchDest, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return true;

    swprintf_s( strDestPath, cchDest, L"%s\\..\\..\\%s\\%s", strExePath, strExeName, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return true;    

    // On failure, return the file as the path but also return an error code
    wcscpy_s( strDestPath, cchDest, strFilename );

    return E_FAIL;
}

//Texture stuff
HRESULT CreateTextureResourcesUAV(ID3D11Device* pDevice, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureRV, ID3D11UnorderedAccessView** ppTextureUAV, UINT width, UINT height)
{
	HRESULT hr = S_FALSE;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS; // | D3D11_BIND_RENDER_TARGET;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.SampleDesc.Count = 1;
	//desc.MiscFlags =  D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; //D3D11_RESOURCE_MISC_GENERATE_MIPS |

	pDevice->CreateTexture2D(&desc, NULL, ppTexture);

	D3D11_SHADER_RESOURCE_VIEW_DESC rvDesc;
	ZeroMemory(&rvDesc, sizeof(rvDesc));
	rvDesc.Format = desc.Format;
	rvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rvDesc.Texture2D.MipLevels = 1;
	rvDesc.Texture2D.MostDetailedMip = 0;

	pDevice->CreateShaderResourceView(*ppTexture, &rvDesc, ppTextureRV );

	D3D11_UNORDERED_ACCESS_VIEW_DESC uvDesc;
	ZeroMemory(&uvDesc, sizeof(uvDesc));
	uvDesc.Format = desc.Format;
	uvDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uvDesc.Texture2D.MipSlice = 0;
	
	pDevice->CreateUnorderedAccessView(*ppTexture, &uvDesc, ppTextureUAV);

	return hr;
}


//-------------------------------------------------------------------------------------------
//Stuff for compute shader rendering
//-------------------------------------------------------------------------------------------

HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, VOID* pInitData, ID3D11Buffer** ppBufOut, D3D11_USAGE Useas)
{
    *ppBufOut = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
	desc.Usage = Useas; 
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.ByteWidth = uElementSize * uCount;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = uElementSize; 

    if ( pInitData )
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;  //source data
        return pDevice->CreateBuffer( &desc, &InitData, ppBufOut );
    } else
        return pDevice->CreateBuffer( &desc, NULL, ppBufOut );
}

HRESULT CreateRawBuffer( ID3D11Device* pDevice, UINT uSize, VOID* pInitData, ID3D11Buffer** ppBufOut )
{
    *ppBufOut = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = uSize;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    if ( pInitData )
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;
        return pDevice->CreateBuffer( &desc, &InitData, ppBufOut );
    } else
        return pDevice->CreateBuffer( &desc, NULL, ppBufOut );
}

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Structured or Raw Buffers
//--------------------------------------------------------------------------------------
HRESULT CreateBufferSRV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut )
{
    D3D11_BUFFER_DESC descBuf;
    ZeroMemory( &descBuf, sizeof(descBuf) );
    pBuffer->GetDesc( &descBuf );

    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    desc.BufferEx.FirstElement = 0;

    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
    {
        // This is a Raw Buffer

        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
    } else
    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
    {
        // This is a Structured Buffer

        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
    } else
    {
        return E_INVALIDARG;
    }

    return pDevice->CreateShaderResourceView( pBuffer, &desc, ppSRVOut );
}




//--------------------------------------------------------------------------------------
// Create Unordered Access View for Structured or Raw Buffers
//-------------------------------------------------------------------------------------- 
HRESULT CreateBufferUAV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut )
{
    D3D11_BUFFER_DESC descBuf;
    ZeroMemory( &descBuf, sizeof(descBuf) );
    pBuffer->GetDesc( &descBuf );
        
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;

    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
    {
        // This is a Raw Buffer

        desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
        desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
        desc.Buffer.NumElements = descBuf.ByteWidth / 4; 
    } else
    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
    {
        // This is a Structured Buffer

        desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
        desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride; 
    } else
    {
        return E_INVALIDARG;
    }
    
    return pDevice->CreateUnorderedAccessView( pBuffer, &desc, ppUAVOut );
}



//--------------------------------------------------------------------------------------
// Run CS
//-------------------------------------------------------------------------------------- 
//void RunComputeShader( ID3D11DeviceContext* pContext,
//                      ID3D11ComputeShader* pComputeShader,
//                      UINT nNumViews, ID3D11ShaderResourceView** pShaderResourceViews, 
//                      ID3D11Buffer* pCBCS, void* pCSData, DWORD dwNumDataBytes,
//                      ID3D11UnorderedAccessView* pUnorderedAccessView,
//                      UINT X, UINT Y, UINT Z )
//{
//    pContext->CSSetShader( pComputeShader, NULL, 0 );
//    pContext->CSSetShaderResources( 0, nNumViews, pShaderResourceViews );
//    pContext->CSSetUnorderedAccessViews( 0, 1, &pUnorderedAccessView, NULL );
//    if ( pCBCS )
//    {
//        D3D11_MAPPED_SUBRESOURCE MappedResource;
//        pContext->Map( pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
//        memcpy( MappedResource.pData, pCSData, dwNumDataBytes );
//        pContext->Unmap( pCBCS, 0 );
//        ID3D11Buffer* ppCB[1] = { pCBCS };
//        pContext->CSSetConstantBuffers( 0, 1, ppCB );
//    }
//
//    pContext->Dispatch( X, Y, Z );
//
//    pContext->CSSetShader( NULL, NULL, 0 );
//
//    ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
//    pContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );
//
//    ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };
//    pContext->CSSetShaderResources( 0, 2, ppSRVNULL );
//
//    ID3D11Buffer* ppCBNULL[1] = { NULL };
//    pContext->CSSetConstantBuffers( 0, 1, ppCBNULL );
//
//
//
//
//}

HRESULT CreateStagingbuffer(ID3D11Device* pDevice, ID3D11Buffer* pBuffDesc, ID3D11Buffer** ppBuffStaging)
{
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	pBuffDesc->GetDesc(&desc);

	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	//desc.MiscFlags = 0;

	return pDevice->CreateBuffer(&desc, NULL, ppBuffStaging);
}

//--------------------------------------------------------------------------------------
// Create a CPU accessible buffer and download the content of a GPU buffer into it
// This function is very useful for debugging CS programs
//-------------------------------------------------------------------------------------- 
ID3D11Buffer* CreateAndCopyToDebugBuf( ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer )
{
    ID3D11Buffer* debugbuf = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    pBuffer->GetDesc( &desc );

    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    if ( SUCCEEDED(pDevice->CreateBuffer(&desc, NULL, &debugbuf)) )
    {
#if defined(_DEBUG) || defined(PROFILE)
        debugbuf->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Debug" ) - 1, "Debug" );
#endif

        pContext->CopyResource( debugbuf, pBuffer );
    }

    return debugbuf;
}
