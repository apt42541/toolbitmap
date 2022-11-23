#pragma once

// Windows API
#include <Windows.h>
#include <d3d9.h>
#include <tchar.h>
#include <dwmapi.h>
#include <tchar.h>

// STL
#include <memory>
#include <string>

// OpenCV
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

// ImGui
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

// STB
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h";

// Autoit
#include "autoit/AutoItX3_DLL.h"