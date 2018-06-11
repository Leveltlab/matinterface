
HRESULT FindDXSDKShaderFileCch( __in_ecount(cchDest) WCHAR* strDestPath, int cchDest, __in LPCWSTR strFilename );

HRESULT CreateShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                              LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                               ID3D11DeviceChild** ppShader, ID3DBlob** ppShaderBlob );

//buffer creation for computeshaders
HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, VOID* pInitData, ID3D11Buffer** ppBufOut, D3D11_USAGE Use);
HRESULT CreateRawBuffer( ID3D11Device* pDevice, UINT uSize, VOID* pInitData, ID3D11Buffer** ppBufOut );
HRESULT CreateBufferSRV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut );
HRESULT CreateBufferUAV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut );
HRESULT CreateStagingbuffer(ID3D11Device* pDevice, ID3D11Buffer* pBuffDesc, ID3D11Buffer** ppBuffStaging);
ID3D11Buffer* CreateAndCopyToDebugBuf( ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer );

HRESULT CreateTextureResourcesUAV(ID3D11Device* pDevice, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureRV, ID3D11UnorderedAccessView** ppTextureUAV, UINT width, UINT height);
HRESULT GetDims(UINT pixnum, UINT* Dim);