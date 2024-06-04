#include "pch.h"

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//RECT windowRect;

Cbuttonmask buttonmask;

LibraryManager dGame;

NOTIFYICONDATA notifyIconData;
HMENU hPopupMenu;

#define ID_TRAY_APP_ICON 5000
#define ID_TRAY_EXIT 3000
#define ID_TRAY_OPEN 3001
#define ID_TRAY_DRIVER 3002
#define ID_TRAY_DISABLE_TEST_MODE 3003
#define WM_SYSICON (WM_USER + 1)
#define IDI_ICON1                       101
void InitNotifyIconData(HWND hWnd) {
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));
	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hWnd;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = WM_SYSICON;
	notifyIconData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1)); // Используем вашу иконку
	strcpy_s(notifyIconData.szTip, "Tray Application");
}


void InitTrayMenu() {
	hPopupMenu = CreatePopupMenu();
	AppendMenuW(hPopupMenu, MF_STRING, ID_TRAY_OPEN, L"Открыть");
	AppendMenuW(hPopupMenu, MF_STRING, ID_TRAY_DISABLE_TEST_MODE, L"Отключить тестовый режим");
	AppendMenuW(hPopupMenu, MF_STRING, ID_TRAY_DRIVER, L"Состояние драйвера");
	AppendMenuW(hPopupMenu, MF_STRING, ID_TRAY_EXIT, L"Выйти");
}


FILE* logFile;
bool ChangeDllPath{ false };

//std::string programm_folder;

bool ver_manager = true;
bool winstatusdrive;
bool testsigning;


DWORD WINAPI ReceiveMessagesThread(LPVOID lpParameter) {
	HANDLE device = INVALID_HANDLE_VALUE;
	BOOL status = FALSE;
	DWORD bytesReturned = 0;
	CHAR outBuffer[128] = { 0 };

	device = CreateFileW(L"\\\\.\\BlueStreetDriver", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (device == INVALID_HANDLE_VALUE) {
		std::cerr << "> Could not open device: " << GetLastError() << std::endl;
		return 1;
	}

	while (true) {
		ZeroMemory(outBuffer, sizeof(outBuffer)); // Очистка буфера перед чтением

		status = DeviceIoControl(device, IOCTL_BLUESTREET_DEBUG_PRINT, NULL, 0, outBuffer, sizeof(outBuffer), &bytesReturned, (LPOVERLAPPED)NULL);

		if (status) {
			printf_s("> Received from the kernel land: %s. Received buffer size: %d\n", outBuffer, bytesReturned);
		}
		else {
			DWORD error = GetLastError();
			if (error == ERROR_DEVICE_NOT_CONNECTED) {
				std::cerr << "> Device disconnected, exiting thread." << std::endl;
				break;
			}
			else {
				std::cerr << "> DeviceIoControl failed: " << error << std::endl;
			}
		}
	}

	CloseHandle(device);
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//std::setlocale(LC_NUMERIC, "C");
#pragma region Command Line Processing Arguments
	LPWSTR lpWideCmdLine = GetCommandLineW();
	int argc = 0;
	LPWSTR* wideArgv = CommandLineToArgvW(lpWideCmdLine, &argc);

	std::vector<char*> argv(argc);

	std::vector<std::string> narrowStrings(argc);
	std::locale::global(std::locale(""));
	for (int i = 0; i < argc; ++i)
	{
		int len = WideCharToMultiByte(CP_ACP, 0, wideArgv[i], -1, NULL, 0, NULL, NULL);
		narrowStrings[i].resize(len);
		WideCharToMultiByte(CP_ACP, 0, wideArgv[i], -1, &narrowStrings[i][0], len, NULL, NULL);
		argv[i] = &narrowStrings[i][0];
	}
#pragma endregion

#pragma region Initialization_and_IPC_Processing
	bool updategamewindowpos = true;
	bool main = false;

	bool done = false;

	std::mutex mtx;
	HANDLE semaphore = CreateSemaphore(NULL, 0, 1, SemaphoreName);

	constexpr char const name__[] = "ipc-chat";

	bool server = false;

	if (semaphore == NULL) {
		MessageBoxW(NULL, L"Failed to create or open semaphore", L"Error", MB_ICONERROR | MB_OK);
		return 1;
	}

	else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (argc >= 2)
		{
			if (!server)
			{
				ipc::channel sender__{ name__, ipc::sender };

				std::string args;
				for (int i = 1; i < argc; i++) {
					args += argv[i];
					args += "|";
				}

				sender__.send(args.c_str());
				return 0;
			}
		}
		else
			return 0;
	}

	for (int i = 1; i < argc; i++) {
		if (strcmp("server", argv[i]) == 0)
			server = true;
	}

	if (!IsElevated())
	{
		std::string arg = server ? "" : "server ";

		for (int i = 1; i < argc; i++) {
			arg += argv[i];
			if (i < argc - 1) {
				arg += " ";
			}
		}

		HINSTANCE result = ShellExecute(NULL, "runas", argv[0], arg.c_str(), fs::path(argv[0]).parent_path().string().c_str(), SW_SHOWNORMAL);
		if ((int)result <= 32) {
			std::wostringstream woss;
			woss << L"Failed to Elevate process. Error code: " << GetLastError();
			MessageBoxW(NULL, woss.str().c_str(), L"Error", MB_ICONERROR | MB_OK);
			return 1;
		}

		return 0;
	}

	testsigning = gettestsigning();
	std::thread r;
	std::thread future;
	bool async_theard{ false };
	ServiceManager driver{ "injdrv" };
	InstallDriverStatus DriverStatus;
	if (testsigning) {
		for (int i = 1; i < argc; i++) {
			if (strcmp("game", argv[i]) == 0)
			{
				if (i + 1 < argc)
					dGame.path = argv[i + 1];
				dGame.menu = true;

			}
			else if (strcmp("main", argv[i]) == 0)
			{
				main = true;
			}
		}

		for (size_t i = 0; i < argc; i++)
		{
			std::cout << argv[i] << "\t";
			if (argv[i] == "game")
				dGame.menu = true;

		}

		r = std::thread([&] {
			ipc::channel receiver__{ name__, ipc::receiver };
			while (!done) {
				ipc::buff_t buf = receiver__.recv();
				if (buf.empty()) break;

				std::string dat{ buf.get<char const*>(), buf.size() - 1 };

				std::cout << dat << std::endl;

				std::vector<std::string> substrings;

				std::stringstream ss(dat);

				std::string substring;
				while (std::getline(ss, substring, '|')) {
					substrings.push_back(substring);
				}

				for (size_t i = 0; i < substrings.size(); i++)
				{
					if (substrings[i] == "game")
					{
						if (i + 1 < substrings.size())
						{
							size_t pos = substrings[i + 1].find_last_of('.');

							if (pos != std::string::npos)
								if (substrings[i + 1].substr(pos) == ".exe" || substrings[i + 1].substr(pos) == ".exe\"")
								{
									dGame.path = substrings[i + 1];
									dGame.path.erase(std::remove(dGame.path.begin(), dGame.path.end(), '\"'), dGame.path.end());
								}
								else
									continue;
							else
								continue;

							DbgPrint("gamepath:: %s", dGame.path.c_str());
							dGame.update();
							if (async_theard)
								dGame.parseVersion();

						}
						updategamewindowpos = true;
						dGame.menu = true;

					}
					else if (substrings[i] == "main")
					{
						main = true;
						std::cout << "main = true;" << std::endl;
					}

				}
			}
			std::lock_guard<std::mutex> lock(mtx);
			receiver__.disconnect();
			}
		);
		dGame.update();

		future = std::thread([&] {
			dGame.GetRelease(DXVK, "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");
			dGame.GetRelease(DXVK_GPLASYNC, "glpat-SeehKp1PnXoRQ3heU7ku");

			dGame.getInstalledVersions();
			dGame.parseVersion();
			async_theard = true;
			});

		DriverStatus = driver.Install();
	}






#pragma endregion

#pragma region InitializationImgui
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	WNDCLASSEX wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, AppClass, nullptr };
	RegisterClassEx(&wc);
	hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, AppClass, AppName, WS_POPUP | WS_OVERLAPPED, 0/*(desktop.right / 2) - (WindowWidth / 2)*/, 0, 200, WindowHeight, 0, 0, wc.hInstance, 0);

	InitNotifyIconData(hwnd);
	InitTrayMenu();
	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, ULW_COLORKEY);

	if (CreateDeviceD3D(hwnd) < 0)
	{
		CleanupDeviceD3D();
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	::ShowWindow(hwnd, SW_HIDE);
	::UpdateWindow(hwnd);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetStyle().AntiAliasedLines = true;

	LightTheme();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	ImFontConfig config;
	config.MergeMode = false; // Не объединять с предыдущими шрифтами
	config.GlyphRanges = io.Fonts->GetGlyphRangesCyrillic();


	ImFont* segoeFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 18.0f, &config);

	static const ImWchar icons_ranges[] = { 0xF88C, 0xF88C, 0 };

	config.MergeMode = true; // Объединение с предыдущим шрифтом
	config.PixelSnapH = true;

	// Загрузка файла шрифта с иконками
	ImFont* iconsFont = io.Fonts->AddFontFromFileTTF("C:\\Users\\Roman\\Downloads\\MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf", 18.0f, &config, icons_ranges);

	// Загрузка увеличенного шрифта с иконками
	config.MergeMode = false; // для предотвращения повторного объединения
	ImFont* iconsLargeFont = io.Fonts->AddFontFromFileTTF("C:\\Users\\Roman\\Downloads\\MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf", 35.0f, &config, icons_ranges);
	// Построение шрифтов после добавления всех шрифтов
	io.Fonts->Build();
	io.Fonts->ClearInputData();

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	DWORD dwFlag = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse; //| ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoScrollWithMouse
#pragma endregion


	bool driverActive{ DriverStatus == InstallDriverStatus::SuccessInstalled ||
					   DriverStatus == InstallDriverStatus::ServiceAlreadyRunning ||
					   DriverStatus == InstallDriverStatus::ServiceExists
	};

	winstatusdrive = !driverActive;
	std::wstring taskName = L"DXVK UI Startup";
	bool autostart{ DoesScheduledTaskExist(taskName) };
	while (!done)
	{
#pragma region Message Handler
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				done = true;
				//MessageBoxA(NULL, "Ошибка установки драйвера", "ERROR", MB_ICONERROR);
			}
			if (msg.message == WM_ENDSESSION)

			{
				//MessageBoxA(NULL, "Ошибка установки драйвера", "ERROR", MB_ICONERROR);
				done = true;
			}
			if (msg.message == WM_CLOSE)
			{
				done = true;
				//MessageBoxA(NULL, "Ошибка установки драйвера", "ERROR", MB_ICONERROR);
			}
		}




		if (done)
			break;
#pragma endregion

		//// Handle window resize
		//if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		//{
		//    g_d3dpp.BackBufferWidth = g_ResizeWidth;
		//    g_d3dpp.BackBufferHeight = g_ResizeHeight;
		//    g_ResizeWidth = g_ResizeHeight = 0;
		//    ResetDevice();
		//}


		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (winstatusdrive)
		{
			static ImVec2 winSize(658, 226);
			static auto InstallDriverStatusToString = [&DriverStatus]() -> const char* {
				switch (DriverStatus) {
				case SuccessInstalled:
					return "SuccessInstalled";
				case SuccessUninstalled:
					return "SuccessUninstalled";
				case NotLoaded:
					return "NotLoaded";
				case InsufficientPermissions:
					return "InsufficientPermissions";
				case UnknownError:
					return "UnknownError";
				case ServiceAlreadyRunning:
					return "ServiceAlreadyRunning";
				case ServiceNotFound:
					return "ServiceNotFound";
				case StartServiceFailed:
					return "StartServiceFailed";
				case ServiceMarkedForDelete:
					return "ServiceMarkedForDelete";
				case ServiceExists:
					return "ServiceExists";
				case ServiceControlStopFailed:
					return "ServiceControlStopFailed";
				case ServiceDeleteFailed:
					return "ServiceDeleteFailed";
				case InvalidDriverOrService:
					return "InvalidDriverOrService";
				default:
					return "UnknownStatus";
				}
				};
			static auto getProblemSolvingText = [&DriverStatus]() -> const char* {
				switch (DriverStatus) {
				case SuccessInstalled:
					return "SuccessInstalled";
				case SuccessUninstalled:
					return "SuccessUninstalled";
				case NotLoaded:
					return "NotLoaded";
				case InsufficientPermissions:
					return "InsufficientPermissions";
				case UnknownError:
					return "UnknownError";
				case ServiceAlreadyRunning:
					return "ServiceAlreadyRunning";
				case ServiceNotFound:
					return "ServiceNotFound";
				case StartServiceFailed:
					return "Тестовый режим применяется при перезапуске, требуется перезапустить компьютер!";
				case ServiceMarkedForDelete:
					return "ServiceMarkedForDelete";
				case ServiceExists:
					return "ServiceExists";
				case ServiceControlStopFailed:
					return "ServiceControlStopFailed";
				case ServiceDeleteFailed:
					return "ServiceDeleteFailed";
				case InvalidDriverOrService:
					return "InvalidDriverOrService";
				default:
					return "UnknownStatus";
				}
				};

			static ImVec2 CenteredMain{ ((float)screenWidth - winSize.x) / 2.f ,  ((float)screenHeight - winSize.y) / 2.f };
			ImGui::SetNextWindowPos(CenteredMain, ImGuiCond_Once);
			ImGui::SetNextWindowSize(winSize, ImGuiCond_Once);
			ImGui::Begin("Состояние драйвера", nullptr, dwFlag);
			ImGui::BeginChild("ChildWindow", ImVec2(0, 0), true);

			ImGui::Text("InstallDriverStatus: %s(Код: %d)", InstallDriverStatusToString(), DriverStatus);

			if (!driverActive)
			{
				ImGui::SeparatorText("Возможные решения проблемы:");
				static std::string problemsolving{ getProblemSolvingText() };

				ImGui::Text(problemsolving.c_str());
			}
			else
				ImGui::Text("Драйвер установлен и готов к работе!");

			static ImVec2 window_size = ImGui::GetWindowSize();
			static ImVec2 button_size = ImVec2(200, 50);
			ImGui::SetCursorPos(ImVec2(window_size.x - button_size.x - ImGui::GetStyle().WindowPadding.x - 5, window_size.y - button_size.y - ImGui::GetStyle().WindowPadding.y - 5));

			if (ImGui::Button("Закрыть", button_size)) {
				winstatusdrive = false;
			}

			ImGui::EndChild();
			ImGui::End();
		}

		if (false)
		{
			static ImVec2 winSize(658, 226);
			static ImVec2 CenteredMain{ ((float)screenWidth - winSize.x) / 2.f ,  ((float)screenHeight - winSize.y) / 2.f };
			ImGui::SetNextWindowPos(CenteredMain, ImGuiCond_Once);
			ImGui::SetNextWindowSize(winSize, ImGuiCond_Once);
			ImGui::Begin("Настройки", nullptr, dwFlag);
			ImGui::BeginChild("ChildWindow", ImVec2(0, 0), true);

			if (ImGui::Button(
				std::format(
					"{} автозапуск",
					autostart ? "Выключить" : "Включить"
				).c_str(),
				ImVec2(200, 30)))
			{
				int size_needed = MultiByteToWideChar(CP_ACP, 0, argv[0], -1, NULL, 0);
				std::wstring executablePath(size_needed, 0);
				MultiByteToWideChar(CP_ACP, 0, argv[0], -1, &executablePath[0], size_needed);

				if (!executablePath.empty() && executablePath.back() == L'\0') {
					executablePath.pop_back();
				}

				if (!autostart)
				{
					if (CreateScheduledTask(taskName, executablePath, L"-silent")) {
						DbgPrint("Application startup added to Task Scheduler successfully.\n");
						autostart = true;
					}
					else {
						DbgPrint("Failed to add application to Task Scheduler.\n");
					}
				}
				else
				{
					if (DeleteScheduledTask(taskName)) {
						DbgPrint("Task deleted successfully.\n");
						autostart = false;
					}
					else {
						DbgPrint("Failed to delete the task.\n");
					}
				}
			}


			if (ImGui::Button(testsigning ? "Выключить тестовый режим" : "Включить тестовый режим", ImVec2(200, 30)))
			{
				settestsinging(!testsigning);
				testsigning = gettestsigning();
			}


			static ImVec2 window_size = ImGui::GetWindowSize();
			static ImVec2 button_size = ImVec2(200, 50);
			ImGui::SetCursorPos(ImVec2(window_size.x - button_size.x - ImGui::GetStyle().WindowPadding.x - 5, window_size.y - button_size.y - ImGui::GetStyle().WindowPadding.y - 5));

			if (ImGui::Button("Закрыть", button_size)) {
				winstatusdrive = false;
			}

			ImGui::EndChild();
			ImGui::End();
		}


		if (!testsigning)
		{

			static std::string titlePopup = "Перезагрузка";
			static bool changetestsigning{ false };
			static bool popup{ false };
			static ImVec2 winSize(658, 226);

			static ImVec2 CenteredMain{ ((float)screenWidth - winSize.x) / 2.f ,  ((float)screenHeight - winSize.y) / 2.f };
			static auto displayStateText = [](const std::string& prefix, bool state) {
				std::string stateText = state ? "Включено!" : "Выключено!";
				ImVec4 textColor = state ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

				ImVec2 windowSize = ImGui::GetWindowSize();
				float prefixWidth = ImGui::CalcTextSize(prefix.c_str()).x;
				float stateWidth = ImGui::CalcTextSize(stateText.c_str()).x;
				float totalWidth = prefixWidth + stateWidth;

				ImGui::SetCursorPosX((windowSize.x - totalWidth) * 0.5f);

				ImGui::Text("%s", prefix.c_str());
				ImGui::SameLine();

				ImGui::TextColored(textColor, "%s", stateText.c_str());
				};


			ImGui::SetNextWindowPos(CenteredMain, ImGuiCond_Once);
			ImGui::SetNextWindowSize(winSize, ImGuiCond_Once);
			ImGui::Begin("Установка драйвера", nullptr, dwFlag);

			ImGui::BeginChild("ChildWindow", ImVec2(0, 0), true);

			ImGui::SeparatorText("Для установки драйвера, нужно включить тестовый режим и перезагрузить компьютер!\nТестовый режим сможете выключить -> ПКМ по иконке в трее -> Выключить тестовый режим");

			displayStateText("Состояние автозапуска: ", autostart);
			displayStateText("Тестовый режим: ", testsigning);

			ImGui::Separator();

			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::SetCursorPosX((winSize.x - 320) / 2);
			if (ImGui::Button(
				std::format(
					"{} автозапуск",
					autostart ? "Выключить" : "Включить"
				).c_str(),
				ImVec2(310, 30)))
			{
				int size_needed = MultiByteToWideChar(CP_ACP, 0, argv[0], -1, NULL, 0);
				std::wstring executablePath(size_needed, 0);
				MultiByteToWideChar(CP_ACP, 0, argv[0], -1, &executablePath[0], size_needed);

				if (!executablePath.empty() && executablePath.back() == L'\0') {
					executablePath.pop_back();
				}

				if (!autostart)
				{
					if (CreateScheduledTask(taskName, executablePath, L"-silent")) {
						DbgPrint("Application startup added to Task Scheduler successfully.\n");
						autostart = true;
					}
					else {
						DbgPrint("Failed to add application to Task Scheduler.\n");
					}
				}
				else
				{
					if (DeleteScheduledTask(taskName)) {
						DbgPrint("Task deleted successfully.\n");
						autostart = false;
					}
					else {
						DbgPrint("Failed to delete the task.\n");
					}
				}
			}


			ImGui::Spacing();

			ImGui::SetCursorPosX((winSize.x - 440) / 2);

			// Кнопка для включения тестового режима
			if (ImGui::Button("Включить тестовый режим", ImVec2(200, 30)))
			{
				settestsinging(true);
				changetestsigning = driverActive;
				if (!changetestsigning)
					popup = true;
			}

			ImGui::SameLine();

			ImGui::Dummy(ImVec2(20.0f, 0.0f));

			ImGui::SameLine();

			if (ImGui::Button(changetestsigning ? "Закрыть" : "Выход", ImVec2(200, 30)))
			{
				if (changetestsigning)
					testsigning = gettestsigning();
				else
					done = true;
				changetestsigning = false;
			}

			ImGui::EndChild();

			if (popup)
			{
				ImGui::OpenPopup(titlePopup.c_str());
				popup = false;
			}

			ImGui::SetNextWindowSize(ImVec2(270, 135), ImGuiCond_Always);
			if (ImGui::BeginPopupModal(titlePopup.c_str(), NULL, dwFlag | ImGuiWindowFlags_NoMove))
			{

				ImGui::BeginChild("child", ImVec2(260, 100), true, ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::TextWrapped("Чтобы изменения вступили в силу, необходимо перезагрузить компьютер. Выполнить перезагрузку?");
				if (ImGui::Button("Да", ImVec2(100, 30)))
				{
					reboot();
					ImGui::CloseCurrentPopup();
				}


				ImGui::SameLine();

				ImGui::Dummy(ImVec2(40.0f, 0.0f));

				ImGui::SameLine();
				if (ImGui::Button("Позже", ImVec2(100, 30)))
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
				ImGui::EndChild();
			}

			ImGui::End();
		}
		ImGui::ShowDemoWindow();
		if (false) {
			static bool isFirstFrame = true;
			static float prevProgress = 0.0f;
			static float lastUpdateTime = 0.0f;
			static std::string modifybuttontext;
			static ImVec2 winSize(658, 226);

			static ImVec2 CenteredMain{ ((float)screenWidth - winSize.x) / 2.f ,  ((float)screenHeight - winSize.y) / 2.f };
			ImGui::SetNextWindowSize(ImVec2(WindowWidth, WindowHeight), ImGuiCond_Once);
			ImGui::Begin("fsad", nullptr, dwFlag);

			//deinmodifyModal(dGame.ReleaseInfo, async_theard);

			ImGui::SetCursorPosX(190);
			ImGui::Text("DXVK");

			ComboListVer(DXVK_GPLASYNC, async_theard, "DXVK", ImVec2(150, 90));

			if (LibraryManager::progress > 0)
			{
				float currentTime = ImGui::GetTime();
				float deltaTime = currentTime - lastUpdateTime;
				lastUpdateTime = currentTime;

				float smoothProgress;

				if (isFirstFrame) {

					smoothProgress = LibraryManager::progress;
					isFirstFrame = false;
				}
				else {
					smoothProgress = prevProgress + (LibraryManager::progress - prevProgress) * deltaTime * 5.0f;
					smoothProgress = std::max(smoothProgress, 0.0f);
				}

				prevProgress = smoothProgress;


				ImU32 progressBarColor = IM_COL32(255, 165, 0, 255);  // Стандартный цвет (оранжевый)
				ImU32 fullProgressBarColor = IM_COL32(50, 205, 50, 255);  // Салатовый цвет для полностью загруженной полоски

				if (LibraryManager::progress == 1)
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, fullProgressBarColor);



				ImGui::ProgressBar(smoothProgress, ImVec2(-1.0f, 0.0f), LibraryManager::progress == 1 ? "Complited!" : "Loading");

				if (LibraryManager::progress == 1)
					ImGui::PopStyleColor();
			}

			buttonmask.i[1] = std::accumulate(dGame.data.begin(), dGame.data.end(), 0, [](int sum, const auto& pair) {
				return sum + std::count_if(pair.second.begin(), pair.second.end(), [](const auto& elem) {
					return elem.flag == install_types::installed && elem.pending;
					});
				});

			buttonmask.i[0] = std::accumulate(dGame.data.begin(), dGame.data.end(), 0, [](int sum, const auto& pair) {
				return sum + std::count_if(pair.second.begin(), pair.second.end(), [](const auto& elem) {
					return elem.flag != install_types::installed && elem.pending;
					});
				});



			if (buttonmask.i[0] == 0 && buttonmask.i[1] == 0)
				modifybuttontext = "Выберите";
			else
			{
				if (buttonmask.i[0] > 0)
				{
					modifybuttontext = "Скачать и установить";
					if (buttonmask.i[1] > 0)
					{
						if (!modifybuttontext.empty())
							modifybuttontext += " / ";
						modifybuttontext += "Переустановить или удалить";
					}

				}
				else if (buttonmask.i[1] > 0 && buttonmask.i[0] == 0)
					modifybuttontext = "Переустановить или удалить";
			}

			if (ImGui::Button(modifybuttontext.c_str(), ImVec2(500, 100)))
			{
				if (buttonmask.i[0] > 0)
				{
					std::thread downloadThread(&LibraryManager::DownloadInThreads, &dGame, 5);
					downloadThread.detach();
				}

				if (buttonmask.i[1] > 0)
					ImGui::OpenPopup("1233");

			}


			if (ImGui::Button(ICON_MS_3P "close", ImVec2(100, 20)))
				done = true;
			ImGui::Text("%s", (dGame.menu ? "true" : "false"));

			ImGui::End();

		}

		if (dGame.menu || true) {

			if (updategamewindowpos)
			{
				POINT cursorPos;
				GetCursorPos(&cursorPos);
				ImGui::SetNextWindowPos(ImVec2(cursorPos.x, cursorPos.y));
				updategamewindowpos = false;
			}
			static ImVec2 mainWindowPos(0, 0);
			static ImVec2 WindowSize(800, 500);
			static ImVec2 popupManagerWindowSize(WindowSize.x * 0.8f, WindowSize.y * 0.8f);
			static std::string popupManager{ "popupManager" };
			ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Once);
			ImGui::Begin("Выбор DXVK", nullptr, dwFlag);
			mainWindowPos = ImGui::GetWindowPos();
			{

				ImGui::SetNextWindowSize(ImVec2(WindowSize.x * 0.8f, WindowSize.y * 0.8f), ImGuiCond_Always);
				ImGui::SetNextWindowPos(ImVec2(mainWindowPos.x + WindowSize.x * 0.1f, mainWindowPos.y + WindowSize.y * 0.1f), ImGuiCond_Always);
				if (ImGui::BeginPopupModal("settings_game", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
				{
					if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
					{
						ImGui::CloseCurrentPopup();
					}

					size_t j = 0;
					ImGui::BeginChild("child_id", ImVec2(515, 335), true, ImGuiWindowFlags_AlwaysAutoResize);
					ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("HUD:").x) * 0.5f);
					ImGui::Text("HUD:");


					const std::string& full_name = dGame.hud.params["full"].name;
					bool full_active = dGame.hud.params["full"].active;

					if (!full_active)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.2f));
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
					}
					ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 80) * 0.5f);
					if (ImGui::Button(full_name.c_str(), ImVec2(80, 50)))
					{
						dGame.hud.SetParamActive(full_name, !full_active);
						for (auto i : dGame.hud)
						{
							if (i.second.name == full_name)
								continue;
							dGame.hud.SetParamActive(i.second.name, !full_active);
						}
					}

					ImGui::PopStyleColor(2);
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
					for (auto i : dGame.hud)
					{
						if (i.second.name == full_name)
							continue;

						if (!i.second.active)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.2f));
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
						}
						if (ImGui::Button(i.second.name.c_str(), ImVec2(80, 50)))
						{
							dGame.hud.SetParamActive(i.second.name, !i.second.active);
							if (full_active)
								dGame.hud.SetParamActive(full_name, !full_active);

						}

						ImGui::PopStyleColor(2);

						if (j++ == 0 || (j % 6 != 0 && j != dGame.hud.Count()))
							ImGui::SameLine();
					}
					ImGui::Spacing();
					ImGui::Separator();

					ImGui::SliderFloat("Размер (0 -> 1)", &dGame.hud.size_hud, 0.0f, 5.0f, "%.2f");
					{

					}
					ImGui::SliderFloat("Прозрачность (0 -> 1)", &dGame.hud.opacity_hud, 0.0f, 1.0f, "%.2f");
					{

					}
					ImGui::EndChild();
					ImGui::Button("HUD", ImVec2(50, 50));

					/*if (!sDll[i].include)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.2f));
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
					}

					if (ImGui::Button(sDll[i].dll.c_str(), ImVec2(100, 60)))
						sDll[i].include = !sDll[i].include;
					if (i < sDll.size() - 1)
						ImGui::SameLine();*/




					ImGui::EndPopup();
				}


			}

			{

				ImGui::SetNextWindowSize(popupManagerWindowSize, ImGuiCond_Always);
				ImGui::SetNextWindowPos(ImVec2(mainWindowPos.x + WindowSize.x * 0.1f, mainWindowPos.y + WindowSize.y * 0.1f), ImGuiCond_Always);
				if (ImGui::BeginPopupModal(popupManager.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
				{
					static bool modifytab{ false };
					if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
					{
						modifytab = false;
						ImGui::CloseCurrentPopup();
					}
					popupManager = "popupManager55";
					if (!modifytab)
					{
						static auto fileSize = getFolderSize("./installed");
						static std::string modifybuttontext;
						static bool isFirstFrame = true;
						static float prevProgress = 0.0f;
						static float lastUpdateTime = 0.0f;

						std::string drive = fs::current_path().root_name().string();

						uintmax_t totalBytes = 0;
						uintmax_t freeBytes = 0;

						getDiskSpaceInfo(drive, totalBytes, freeBytes);

						uintmax_t usedBytes = totalBytes - freeBytes;
						float usedPercentage = static_cast<float>(usedBytes) / totalBytes;

						ImGui::BeginChild("DiskChild", ImVec2(0, 63), true);
						ImGui::Text("Установлено версий на: %d МБ", fileSize);

						std::string freespace = "Свободно: " + std::to_string((size_t)(freeBytes / (1024.0 * 1024.0))) + " MB";
						ImGui::ProgressBar(usedPercentage, ImVec2(0.0f, 0.0f), freespace.c_str());
						ImGui::SameLine();
						ImGui::Text("%s\\ %.f MB", drive.c_str(), totalBytes / (1024.0 * 1024.0));
						ImGui::EndChild();


						{
							ImGui::BeginChild("DXVKChild", ImVec2(popupManagerWindowSize.x / 2 - 11, 210), true);
							static float windowWidth = ImGui::GetContentRegionAvail().x;
							static float textWidth = ImGui::CalcTextSize("DXVK").x;
							ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
							ImGui::Text("DXVK");

							ComboListVer(DXVK, async_theard, "DXVKInstalled", ImVec2(windowWidth, 170));

							ImGui::EndChild();
						}

						ImGui::SameLine();



						{
							ImGui::BeginChild("DXVK-ASYNCChild", ImVec2(popupManagerWindowSize.x / 2 - 10, 210), true);
							static float windowWidth = ImGui::GetContentRegionAvail().x;
							static float textWidth = ImGui::CalcTextSize("DXVK-ASYNC").x;
							ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
							ImGui::Text("DXVK-ASYNC");

							ComboListVer(DXVK_GPLASYNC, async_theard, "DXVKGPLASYNCInstalled", ImVec2(windowWidth, 170));

							ImGui::EndChild();
						}
						

						if (LibraryManager::progress > 0)
						{
							float currentTime = ImGui::GetTime();
							float deltaTime = currentTime - lastUpdateTime;
							lastUpdateTime = currentTime;

							float smoothProgress;

							if (isFirstFrame) {

								smoothProgress = LibraryManager::progress;
								isFirstFrame = false;
							}
							else {
								smoothProgress = prevProgress + (LibraryManager::progress - prevProgress) * deltaTime * 5.0f;
								smoothProgress = std::max(smoothProgress, 0.0f);
							}

							prevProgress = smoothProgress;


							//ImU32 progressBarColor = IM_COL32(255, 165, 0, 255);  // Стандартный цвет (оранжевый)
							ImU32 fullProgressBarColor = IM_COL32(50, 205, 50, 255);  // Салатовый цвет для полностью загруженной полоски

							if (LibraryManager::progress == 1)
								ImGui::PushStyleColor(ImGuiCol_PlotHistogram, fullProgressBarColor);


							std::ostringstream oss;
							oss << std::fixed << std::setprecision(1) << speedMbps;
							std::string speedStr = oss.str() + " МБ/с";

							ImGui::ProgressBar(smoothProgress, ImVec2(-1.0f, 0.0f), LibraryManager::progress == 1 ? "Complited!" : speedStr.c_str());

							if (LibraryManager::progress == 1)
								ImGui::PopStyleColor();
						}

						



						if (buttonmask.i[0] == 0 && buttonmask.i[1] == 0)
							modifybuttontext = "Выберите";
						else
						{
							if (buttonmask.i[0] > 0)
							{
								modifybuttontext = "Скачать и установить";
								if (buttonmask.i[1] > 0)
								{
									if (!modifybuttontext.empty())
										modifybuttontext += " / ";
									modifybuttontext += "Переустановить или удалить";
								}

							}
							else if (buttonmask.i[1] > 0 && buttonmask.i[0] == 0)
								modifybuttontext = "Переустановить или удалить";
						}
						static ImVec2 RegionAvail = ImGui::GetContentRegionAvail();
						ImGui::SetCursorPos(ImVec2((RegionAvail.x - 200) * 0.5f, popupManagerWindowSize.y-60));

						if (ImGui::Button(modifybuttontext.c_str(), ImVec2(200, 50)))
						{
							if (buttonmask.i[0] > 0)
							{
								std::thread downloadThread(&LibraryManager::DownloadInThreads, &dGame, 5);
								downloadThread.detach();
							}

							if (buttonmask.i[1] > 0)
								modifytab = true;

						}
					}
					else
					{

						if (buttonmask.i[1] == 0)
							modifytab = false;


						int countX = 0;
						int countY = 0;
						static ImVec2 buttonSize{ popupManagerWindowSize.x / 3 -  4 * 5, 50 };

						for (const auto& [key, vec] : dGame.data) {
							for (const auto& elem : vec) {
								if (elem.pending && elem.flag == install_types::installed) {
									countX++;
									if (elem.modify == modify_types::none) {
										countY++;
									}
								}
							}
						}



						ComboListVer(unknown, async_theard, "deinmodify", ImVec2(popupManagerWindowSize.x - 10, 307), Flag_ModifyWindow);

						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.17f, 0.17f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.22f, 0.22f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));


						if (ImGui::Button(countX == countY ? "Удалить всё" : "Удалить выбранное", buttonSize))
						{
							LibraryManager::progress = 0;
							for (auto& [key, vec] : dGame.data) {
								for (auto& i : vec) {
									if (i.modify == modify_types::del || (countX == countY && i.pending))
									{
										if (i.flag == install_types::installed)
										{
											std::string folder{ i.getfoldername() };
											try {
												fs::remove_all(folder);
												i.clear();
												DbgPrint("Папка удалена: %s\n", folder.c_str());
											}
											catch (const fs::filesystem_error& e) {
												i.flag = install_types::error;
												std::string errorMsg = "Не удалось удалить папку(" + folder + "): " + e.what();
												DbgPrint("%s", errorMsg.c_str());

											}
										}

									}
								}
							}
							LibraryManager::progress = 1;
						}

						ImGui::SameLine();
						ImGui::Dummy(ImVec2(14.0f, 0.0f));
						ImGui::SameLine();

						bool s = LibraryManager::count_download != 0;

						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.17f, 0.27f, 0.7f, s ? 0.4f : 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.32f, 0.7f, s ? 0.4f : 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.7f, s ? 0.4f : 1.0f));

						if (ImGui::Button(countX == countY ? "Переустановить всё" : "Переустановить выбранное", buttonSize))
						{
							if (!s)
							{
								for (auto& [key, vec] : dGame.data) {
									for (auto& i : vec) {
										if (countX == countY && i.pending)
											i.modify = modify_types::reinstall;
									}
								}
								std::thread downloadThread(&LibraryManager::DownloadInThreads, &dGame, 5);
								downloadThread.detach();
							}
						}
						ImGui::PopStyleColor(6);

						ImGui::SameLine();
						ImGui::Dummy(ImVec2(14.0f, 0.0f));
						ImGui::SameLine();

						if (ImGui::Button("Закрыть", buttonSize))
						{
							modifytab = false;
						}
					}




					buttonmask.i[1] = std::accumulate(dGame.data.begin(), dGame.data.end(), 0, [](int sum, const auto& pair) {
						return sum + std::count_if(pair.second.begin(), pair.second.end(), [](const auto& elem) {
							return elem.flag == install_types::installed && elem.pending;
							});
						});

					buttonmask.i[0] = std::accumulate(dGame.data.begin(), dGame.data.end(), 0, [](int sum, const auto& pair) {
						return sum + std::count_if(pair.second.begin(), pair.second.end(), [](const auto& elem) {
							return elem.flag != install_types::installed && elem.pending;
							});
						});

					ImGui::EndPopup();
				}


			}
			ComboListVer(DXVK, async_theard, "DXVKInstalled", ImVec2(150, 90), Flag_ShowOnlyInstalled);
			ImGui::SameLine();
			ImGui::PushFont(iconsLargeFont);
			if (ImGui::Button(ICON_MS_EDIT_DOCUMENT, ImVec2(50, 50)))
			{
				ImGui::OpenPopup(popupManager.c_str());
			}
			
			ImGui::PopFont();
			if (!dGame.dlls_FolderPath.empty()) {
				if (ChangeDllPath)
				{
					dGame.dllClear();
					for (const auto& entry : fs::directory_iterator(dGame.dlls_FolderPath + "/x64")) {
						if (entry.is_regular_file()) {
							dGame.dll.push_back(DllPath(entry.path().filename().string(), false));
						}
					}
					dGame.dllSort();

					ChangeDllPath = false;
				}

				for (int i = 0; i < dGame.dll.size(); i++)
				{
					ImGui::PushID(i);

					if (!dGame.dll[i].include)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.2f));
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
					}

					if (ImGui::Button(dGame.dll[i].dll.c_str(), ImVec2(100, 60)))
						dGame.dll[i].include = !dGame.dll[i].include;
					if (i < dGame.dll.size() - 1)
						ImGui::SameLine();

					ImGui::PopStyleColor(2);


					ImVec2 m = ImGui::GetIO().MousePos;
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(.0f, .0f)); // Устанавливаем прозрачный цвет фона
					ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Устанавливаем прозрачный цвет фона
					ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, .0f); // Устанавливаем прозрачный цвет обводки
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptNoPreviewTooltip)) //ImGuiDragDropFlags_AcceptNoDrawDefaultRect |

					{

						ImGui::PushID(123);
						ImGui::SetDragDropPayload("DND_DEMO_CELL", &i, sizeof(int));
						//ImGui::SetNextWindowPos(ImVec2(m.x - 50, m.y - 30));

						////ImGui::Begin("1", nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs);


						ImGui::Button(dGame.dll[i].dll.c_str(), ImVec2(100, 60));

						////ImGui::End();


						// Восстанавливаем стиль обводки кнопки
						ImGui::EndDragDropSource();
						ImGui::PopID();
					}
					ImGui::PopStyleColor();
					ImGui::PopStyleVar(2);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL"))
						{
							IM_ASSERT(payload->DataSize == sizeof(int));

							int payload_n = *(const int*)payload->Data;
							std::swap(dGame.dll[i].dll, dGame.dll[payload_n].dll);
							std::swap(dGame.dll[i].include, dGame.dll[payload_n].include);

						}
						ImGui::EndDragDropTarget();
					}

					ImGui::PopID();
				}
				if (ImGui::Button("Запустить", ImVec2(100, 60))) {

					lauchgame();

					dGame.menu = false;

				}
				if (ImGui::Button("Запустить\nв тестовом режиме", ImVec2(100, 80))) {

					lauchgame(true);

				}
				if (ImGui::Button("Настройки", ImVec2(100, 60)))
				{
					ImGui::OpenPopup("settings_game");
				}
			}

			ImGui::End();

		}


#pragma region ImGui DirectX Rendering
#pragma region ImGui Render Setup
		ImGui::Render();

		g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
#pragma endregion

#pragma region Scene Clear Begin
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(0, 0, 0, 255);
		g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
#pragma endregion

#pragma region ImGui PlatformWindows Render
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
#pragma endregion

#pragma region Scene Present

		HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
#pragma endregion
#pragma endregion
	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	if (r.joinable()) {
		r.detach();
	}
	if (future.joinable()) {
		future.detach();
	}


	CloseHandle(semaphore);
	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) return E_FAIL;

	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) return E_FAIL;

	return S_OK;
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}
static bool startCursorPosSet = false;
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
		InitNotifyIconData(hWnd);
		Shell_NotifyIcon(NIM_ADD, &notifyIconData);
		InitTrayMenu();
		break;
	}
	case WM_SYSICON: {
		if (lParam == WM_RBUTTONDOWN) {
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, curPoint.x, curPoint.y, 0, hWnd, NULL);
		}
		break;
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case ID_TRAY_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
			PostQuitMessage(0);
			break;
		case ID_TRAY_OPEN:
			MessageBox(NULL, TEXT("Открыть выбрано"), TEXT("Сообщение"), MB_OK);
			break;
		case ID_TRAY_DRIVER:
			winstatusdrive = !winstatusdrive;
			break;
		case ID_TRAY_DISABLE_TEST_MODE:
			settestsinging(false);
			testsigning = gettestsigning();
			break;
		}
		break;
	}
	case WM_DESTROY: {
		Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

