#include "Header.h"

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp ={};
static HWND	g_LDPlayer;
static HWND g_LDPlayerRender;
static shared_ptr<ImageData> img;
static Mat target;
static bool test = 0;


//hwnd varaible 
static HWND g_GameHwnd;
static HWND g_ControlHwnd;
static LPCWSTR title = L"LDPlayer(64)";
HWND newhwnd;
HWND lpPlayerHWND;
HWND ldPlayerCrtlHWND;



// Forward declarations of helper functions
bool CreateDeviceD3D( HWND hWnd );
void CleanupDeviceD3D();
void ResetDevice();

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

Mat getMat( HWND hWnd ) {

	HDC deviceContext = GetDC( hWnd );
	HDC memoryDeviceContext = CreateCompatibleDC( deviceContext );

	RECT clientRect;
	GetClientRect( hWnd, &clientRect );

	int height = clientRect.bottom;
	int width = clientRect.right;

	HBITMAP bitmap = CreateCompatibleBitmap( deviceContext, width, height );

	SelectObject( memoryDeviceContext, bitmap );

	/*   int x           = x1;
	   int y           = y2;
	   int width2      = cx-x;
	   int height2       = cy-y;*/
	   //copy data into bitmap
	BitBlt( memoryDeviceContext, 0, 0, width, height, deviceContext, 0, 0, SRCCOPY );


	//specify format by using bitmapinfoheader!
	BITMAPINFOHEADER bi;
	bi.biSize = sizeof( BITMAPINFOHEADER );
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0; //because no compression
	bi.biXPelsPerMeter = 1; //we
	bi.biYPelsPerMeter = 2; //we
	bi.biClrUsed = 3; //we ^^
	bi.biClrImportant = 4; //still we

	cv::Mat mat = cv::Mat( height, width, CV_8UC4 ); // 8 bit unsigned ints 4 Channels -> RGBA

	//transform data and store into mat.data
	GetDIBits( memoryDeviceContext, bitmap, 0, height, mat.data, ( BITMAPINFO* )&bi, DIB_RGB_COLORS );

	//clean up!
	DeleteObject( bitmap );
	DeleteDC( memoryDeviceContext ); //delete not release!
	ReleaseDC( hWnd, deviceContext );

	return mat;
}

std::shared_ptr<ImageData> Load_Image( const string& ImageFile ) {
	int image_width = 0;
	int image_height = 0;
	int channel = 0;
	BYTE* image_data ={};
	PDIRECT3DTEXTURE9 tex;
	D3DLOCKED_RECT lock_rect;

	if (!( image_data = stbi_load( ImageFile.c_str(), &image_width, &image_height, &channel, 0 ) ))
		return nullptr;

	if (g_pd3dDevice->CreateTexture( image_width, image_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL ) != D3D_OK)
		return nullptr;

	auto out = std::make_shared<ImageData>();
	auto& src = out->Data;
	out->Texture = tex;
	src.resize( image_width * image_height * 4 );

	for (int i = 0; i < image_height; i++) {
		for (int j = 0; j < image_width; j++) {
			src[( i * image_width + j ) * 4 + 0] = image_data[( i * image_width + j ) * 3 + 2];
			src[( i * image_width + j ) * 4 + 1] = image_data[( i * image_width + j ) * 3 + 1];
			src[( i * image_width + j ) * 4 + 2] = image_data[( i * image_width + j ) * 3 + 0];
			src[( i * image_width + j ) * 4 + 3] = 255;
		}
	}

	out->Width = image_width;
	out->Height = image_height;
	stbi_image_free( image_data );

	tex->LockRect( 0, &lock_rect, NULL, 0 );

	for (int y = 0; y < image_height; y++)
		memcpy( ( BYTE* )lock_rect.pBits + lock_rect.Pitch * y, src.data() + ( image_width * 4 ) * y, ( image_width * 4 ) );

	tex->UnlockRect( 0 );

	return out;
}

void tool() {
	ImGui::Begin( "Hello, world!" );

	static char buf[100] = "LDPlayer(64)";

	/*if (ImGui::InputText( "window string", buf, IM_ARRAYSIZE( buf ) )) {
		newHwnd = FindWindowA( NULL, buf );
	}*/

	while (ImGui::Button( "Screenshot", ImVec2( 100.0f, 100.0f ) )) {
		g_LDPlayer       = FindWindowW( L"LDPlayerMainFrame", NULL );
		if (!g_LDPlayer || IsIconic( g_LDPlayer )) { MessageBeep( 0 );  break; }
		g_LDPlayerRender = FindWindowExW( g_LDPlayer, 0, L"RenderWindow", L"TheRender" );
		if (!g_LDPlayerRender) { MessageBeep( 0 );  break; }
		target           = getMat( g_LDPlayerRender );
		imwrite( "out.jpg", target );
		img  = Load_Image( "out.jpg" );
		test = true;
		break;
	}

	ImGui::End();
}


void snapshot() {
	static ImVec2 crop_min{};
	static ImVec2 crop_min2{};
	static ImVec2 crop_max{};

	static int cropState       = 0;
	static ImVec2 m_pos        ={};
	static bool cropDone       = false;
	static float Aspect        = 16.0f / 9.0f;
	static float displayHeight = 540.0f;
	static float displayWidth  = displayHeight * Aspect;

	auto& style                = ImGui::GetStyle();
	bool onClose               = true;
	float imageAspect          = ( float )img->Width / ( float )img->Height;
	float imageWidth           = displayHeight * imageAspect;
	ImVec2 displaySize( displayWidth, displayHeight );
	ImVec2 imageSize( imageAspect > Aspect ? displayWidth : imageWidth, displayHeight );

	ImGui::SetNextWindowContentSize( displaySize );

	if (!ImGui::Begin( "Result", &onClose, ImGuiWindowFlags_AlwaysAutoResize )) {
		ImGui::End();
		return;
	}

	auto drawList = ImGui::GetWindowDrawList();

	if (test && img) {
		ImGui::SetCursorPosX( ImGui::GetCursorPosX() + fabs( displaySize.x - imageSize.x ) * .5f );
		ImGui::Image( img->Texture, imageSize );
		ImVec2 pos  = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();

		if (ImGui::IsItemHovered()) {
			if (cropState == 2 && ImGui::IsMouseClicked( ImGuiMouseButton_Right, false ))
				cropState = 0;
			if (cropState == 0 && ImGui::IsMouseClicked( ImGuiMouseButton_Left, false )) {
				crop_min   = ImGui::GetMousePos() - pos;
				cropState  = 1;
			}
			else if (cropState == 1 && ImGui::IsMouseClicked( ImGuiMouseButton_Left, false )) {
				crop_max   = ImGui::GetMousePos() - pos;
				cropState  = 0;
				cropDone   = true;

				auto min   = ImVec2( std::min( crop_min.x, crop_max.x ), std::min( crop_min.y, crop_max.y ) );
				auto max   = ImVec2( std::max( crop_min.x, crop_max.x ), std::max( crop_min.y, crop_max.y ) );
				auto uv0   = min / size;
				auto uv1   = max / size;
				auto s0    = uv0 * ImVec2( ( float )img->Width, ( float )img->Height );
				auto s1    = uv1 * ImVec2( ( float )img->Width, ( float )img->Height );
				imwrite( "p.jpg", target( Rect( s0.x, s0.y, s1.x - s0.x, s1.y - s0.y ) ) );
				cropState = 2;
			}
			else if (cropState == 2 && ImGui::IsMouseClicked( ImGuiMouseButton_Left, false )) {
				crop_min2   = ImGui::GetMousePos() - pos;

				auto min   = ImVec2( crop_min2.x, crop_min2.y );
				auto uv0   = min / size;
				auto s0    = uv0 * ImVec2( ( float )img->Width, ( float )img->Height );
				AU3_ControlClickByHandle( lpPlayerHWND, ldPlayerCrtlHWND, L"Left", 2, s0.x, s0.y );
			}


		}

		if (cropState == 1) {
			drawList->AddRect( pos + crop_min, ImGui::GetMousePos(), 0xff0000ff );
		}
		else if (cropDone && cropState == 2) {
			drawList->AddRect( pos + crop_min, pos + crop_max, 0xff00ff00 );
		}

	}

	if (!onClose)
		test = 0;

	ImGui::End();
}
// Main code
int main( int, char** )
{
	AU3_Opt( L"MouseCoordMode", 2 );
	AU3_Opt( L"PixelCoordMode", 2 );

	newhwnd = FindWindow( NULL, title );
	lpPlayerHWND = FindWindowW( L"LDPlayerMainFrame", title );
	ldPlayerCrtlHWND = FindWindowExW( lpPlayerHWND, 0, L"RenderWindow", L"TheRender" );

	//ImGui_ImplWin32_EnableDpiAwareness();
	SetProcessDPIAware();
	WNDCLASSEXW wc ={ sizeof( wc ), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle( NULL ), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
	::RegisterClassExW( &wc );
	HWND hwnd = ::CreateWindowW( wc.lpszClassName, L"Dear ImGui DirectX9 Example", WS_POPUP, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL );

	// Initialize Direct3D
	if (!CreateDeviceD3D( hwnd )) {
		CleanupDeviceD3D();
		::UnregisterClassW( wc.lpszClassName, wc.hInstance );
		return 1;
	}

	// Show the window
	::ShowWindow( hwnd, SW_SHOWDEFAULT );
	::UpdateWindow( hwnd );

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); ( void )io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init( hwnd );
	ImGui_ImplDX9_Init( g_pd3dDevice );

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4( 0.45f, 0.55f, 0.60f, 1.00f );

	// Main loop
	bool done = false;
	MSG msg;

	while (!done) {
		while (::PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE )) {
			::TranslateMessage( &msg );
			::DispatchMessage( &msg );
			if (msg.message == WM_QUIT)
				done = true;
		}

		if (done)
			break;

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		tool();

		if (test && img)
			snapshot();

		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
		g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		g_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA( ( int )( clear_color.x * clear_color.w * 255.0f ), ( int )( clear_color.y * clear_color.w * 255.0f ), ( int )( clear_color.z * clear_color.w * 255.0f ), ( int )( clear_color.w * 255.0f ) );
		g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0 );
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData() );
			g_pd3dDevice->EndScene();
		}

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		HRESULT result = g_pd3dDevice->Present( NULL, NULL, NULL, NULL );

		// Handle loss of D3D9 device
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow( hwnd );
	::UnregisterClassW( wc.lpszClassName, wc.hInstance );

	return 0;
}

// Helper functions

bool CreateDeviceD3D( HWND hWnd )
{
	if (( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory( &g_d3dpp, sizeof( g_d3dpp ) );
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice ) < 0)
		return false;

	return true;
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset( &g_d3dpp );
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT( 0 );
	ImGui_ImplDX9_CreateDeviceObjects();
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if (ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD( lParam );
			g_d3dpp.BackBufferHeight = HIWORD( lParam );
			ResetDevice();
		}
		return 0;
	case WM_SYSCOMMAND:
		if (( wParam & 0xfff0 ) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage( 0 );
		return 0;
	case WM_DPICHANGED:
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
		{
			//const int dpi = HIWORD(wParam);
			//printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
			const RECT* suggested_rect = ( RECT* )lParam;
			::SetWindowPos( hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE );
		}
		break;
	}
	return ::DefWindowProc( hWnd, msg, wParam, lParam );
}
