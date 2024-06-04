// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H
#define _CRT_SECURE_NO_WARNINGS

#include <boost/process.hpp>
#include <thread>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <d3d9.h>
#include <tchar.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <regex>
#include <zlib.h>
#include <future>
#include <string>
#include <algorithm>
#include <bitset>
#include "reg.h"
#include "imspinner.h"
#include <WS2tcpip.h>
#include <mutex>
#include <libipc/ipc.h>
#include <strsafe.h>
#include <Shellapi.h>
#include <stdio.h>
#include <winioctl.h>
#include <comdef.h>
#include <taskschd.h>
#include "TaskSchedulerLib.h"
#include "ServiceManager.h"
#include "ioctl.h"
#include "enums.h"
#include "DllPath.h"
#include <chrono>
#include <ctime>
#include "IconsMaterialSymbols.h"
#include <boost/process/windows.hpp>

#if DBG
#define DbgPrint(Format, ...) printf(Format, __VA_ARGS__)
#else
#define DbgPrint(Format, ...) void logToFile(const char* format, ...); logToFile(Format, __VA_ARGS__)
#endif

#include "HUD.h"
#include "LibraryManager.h"


#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

#define SemaphoreName "DXVK UI"

static const char* AppClass = "APP CLASS";
static const char* AppName = "APP NAME";
static HWND hwnd = NULL;
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

extern double speedMbps;
static ImFont* DefaultFont = nullptr;
static int WindowWidth = 1280;
static int WindowHeight = 800;

namespace fs = std::filesystem;
using json = nlohmann::json;


extern bool ver_manager;
extern size_t count_download;

extern FILE* logFile;
extern bool ChangeDllPath;

modify_types operator++(modify_types& value);


struct Cbuttonmask {
	size_t i[2]{};
};

extern LibraryManager dGame;

extern Cbuttonmask buttonmask;

/*<---             functions.cpp             --->*/


HRGN CreateRoundRectRgn(int x, int y, int width, int height, int radius);

void SetWindowRoundRect(HWND hwnd, int width, int height, int radius);

bool HasFlag(FlagsState flag, FlagsState& mask);

//std::vector<std::string> getInstalledVersions(std::vector<ReleaseInfo>& ver);

//std::string extractFileName(const std::string& url);

//bool CompareReleases(const ReleaseInfo& release1, const ReleaseInfo& release2);

//void Download(const size_t& taskId, ReleaseInfo& arr);

//void DownloadInThreads(std::vector<ReleaseInfo>& ver, int numThreads);

//int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t /*ultotal*/, curl_off_t /*ulnow*/);

//std::vector<ReleaseInfo> GetReleases(const std::vector<std::string>& repositories, const std::string& token);

bool IsElevated();

bool gettestsigning();

void settestsinging(bool status);

void reboot();

bool lauchgame(bool testSetup = false);

uintmax_t getFolderSize(const fs::path& folderPath);

void getDiskSpaceInfo(const std::string& drive, uintmax_t& totalBytes, uintmax_t& freeBytes);

//size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);


/*<---                                        --->*/



/*<---          imgui addition.cpp            --->*/

void ComboListVer(type typebin, const bool& theard_status, std::string name, ImVec2 size, FlagsState flag = Flag_None);

//void deinmodifyModal(std::vector<ReleaseInfo>& ver, const bool& theard_status);

void DarkTheme();
void LightTheme();

/*<---                                        --->*/
#endif //PCH_H
