#include "stdafx.h"

#include "Device.h"
#include "GPUCPU.h"

#include "basewin.h"


HRESULT MainWindow::Init(GpuParam Par)
{
	HRESULT hr = S_OK;

	V_RETURN(m_Dev.InitDevice(m_hwnd))

	V_RETURN(m_GPCP.Init(m_Dev.GetDevice(), m_Dev.GetContext(), Par))

	return hr;
}


HRESULT MainWindow::Resize(GpuParam Par)
{
	HRESULT hr = S_OK;

//	InvalidateRect(m_hwnd, NULL, FALSE);

	V_RETURN(m_Dev.Resize(m_hwnd))

	V_RETURN(m_GPCP.Init(m_Dev.GetDevice(), m_Dev.GetContext(), Par))

		return hr;
}


void MainWindow::Clear()
{
	ShowWindow(m_hwnd, 0);

	m_GPCP.GClear();
	//m_Dev.ClearDevice();
}

void MainWindow::Close()
{
	if (m_hwnd){
		//m_Dev.ClearDevice();
		DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}

}

void MainWindow::SetScaleOffset(float Scl, bool bTyp, bool bCol)
{
	ID3D11DeviceContext* pContext = m_Dev.GetContext();

	if (bTyp)
		m_GPCP.SetScL(Scl, bCol, pContext);  //set scale, bCol :G or R
	else 
		m_GPCP.SetSOff(Scl, bCol, pContext); //set offset


}

void MainWindow::Shift(short Shift)
{
	ID3D11DeviceContext* pContext = m_Dev.GetContext();
	m_GPCP.UpdateShift(Shift, pContext);
}

void MainWindow::Render(UINT* DataIn, UINT* DataOut)
{
		ID3D11DeviceContext* pContext = m_Dev.GetContext();
		m_GPCP.toGPU(pContext, DataIn);
		m_GPCP.Compute(pContext);
		m_GPCP.toCPU(pContext, DataOut);

		m_GPCP.Render(pContext);
		m_Dev.Present();
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//TCHAR chCharCode;
	static int ColChoice = 0;
	LPWINDOWPOS wp;

	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(m_hwnd);
		return 0;

	//case WM_DESTROY:
	//	PostQuitMessage(0);
	//	return 0;

	//case WM_PAINT:
	//	OnPaint();
	//	return 0;

//	case WM_SIZE:
//		if (FAILED(Resize()))
//			return 1;
	//		PostQuitMessage(0);
//		return 0;

//	case WM_CHAR:

		//chCharCode = (TCHAR)wParam;
		//if (chCharCode == VK_ESCAPE){
		//	PostQuitMessage(0); //just sends a WM_QUIT to the threads message que
		//}
//		return 0;

	case WM_WINDOWPOSCHANGING:
		wp = (LPWINDOWPOS)lParam;
		wp->flags |= SWP_NOZORDER; //don't change the z order of this window
		return 0;

    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

