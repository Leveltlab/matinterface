

template <class DERIVED> 
class BaseWindow
{
public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DERIVED *pThis = NULL;

		try {
			if (uMsg == WM_NCCREATE)
			{
				CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
				pThis = (DERIVED*)pCreate->lpCreateParams;
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

				pThis->m_hwnd = hwnd;
			}
			else
			{
				pThis = (DERIVED*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			}


			if (pThis)
				return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		catch (std::exception & stdExp)
		{
			printf("Exception Caught: %s\n", stdExp.what());
			//return 1;
		}


		return DefWindowProc(hwnd, uMsg, wParam, lParam);


    }

    BaseWindow() : m_hwnd(NULL) { }

    BOOL Create(
        PCWSTR lpWindowName,
        DWORD dwStyle,
		int nCmdShow,
        DWORD dwExStyle = 0,
        int x = CW_USEDEFAULT,
        int y = CW_USEDEFAULT,
        int nWidth = CW_USEDEFAULT,
        int nHeight = CW_USEDEFAULT,
        HWND hWndParent = 0,
        HMENU hMenu = 0
        )
    {
        WNDCLASS wc = {0};

        wc.lpfnWndProc   = DERIVED::WindowProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.lpszClassName = (LPCWSTR)ClassName();

        RegisterClass(&wc);

		m_hwnd = CreateWindowEx(
				dwExStyle, (LPCWSTR)ClassName(), (LPCWSTR)lpWindowName, dwStyle, x, y,
				nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
				);
		


	    if( !m_hwnd )
			return FALSE;

		ShowWindow( m_hwnd, nCmdShow );

		return TRUE;
    }

    HWND Window() const { return m_hwnd; }

protected:

    virtual PCWSTR  ClassName() const = 0;
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

    HWND m_hwnd;
};

class MainWindow : public BaseWindow<MainWindow>
{


	 Device                 m_Dev;
	 GpuCpu				    m_GPCP;


public:

	MainWindow(){
		m_bWin = false;
	}

	PCWSTR  ClassName() const { return L"Main Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	HRESULT choosefilename(WCHAR* szFile);

	HRESULT Init(GpuParam Par);
	HRESULT Resize(GpuParam Par);

	void Clear();
	void Close();

	void Render(UINT* DataIn, UINT* DataOut);
	void SetScaleOffset(float Scl, bool bTyp, bool bCol);
	void Shift(short Shift);

	UINT					m_width;
	UINT					m_height;
	bool					m_bWin;

};



