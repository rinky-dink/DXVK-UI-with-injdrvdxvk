#include "pch.h"

HRGN CreateRoundRectRgn(int x, int y, int width, int height, int radius) {
    return CreateRoundRectRgn(x + 1, y + 1, x + width - 1, y + height - 1, radius, radius);
}

void SetWindowRoundRect(HWND hwnd, int width, int height, int radius) {
    HRGN hRgn = CreateRoundRectRgn(0, 0, width, height, radius);
    SetWindowRgn(hwnd, hRgn, TRUE);
    DeleteObject(hRgn);
}

bool HasFlag(FlagsState flag, FlagsState& mask) {
    return (flag & mask) != 0;
}

modify_types operator++(modify_types& value) {
    value = static_cast<modify_types>(static_cast<int>(value) + 1);
    if (value == modify_types::maxvalue) {
        value = modify_types::none;
    }
    return value;
}

std::mutex logMutex;
bool isNewLine = true;

void logToFile(const char* format, ...) {
    std::lock_guard<std::mutex> lock(logMutex);

    fopen_s(&logFile, "log.txt", "a");
    if (logFile != nullptr) {
        if (isNewLine) {
            // Получаем текущее время
            auto now = std::chrono::system_clock::now();
            auto now_time = std::chrono::system_clock::to_time_t(now);

            char time_str[100];
            std::strftime(time_str, sizeof(time_str), "%d.%m.%Y %H:%M:%S", std::localtime(&now_time));

            fprintf(logFile, "[%s] ", time_str);
        }

        va_list args;
        va_start(args, format);
        vfprintf(logFile, format, args);
        va_end(args);

        // Проверка, закончилась ли строка символом новой строки
        if (strchr(format, '\n') != nullptr) {
            isNewLine = true;
        }
        else {
            isNewLine = false;
        }

        fclose(logFile);
    }
}

bool IsElevated() {
    BOOL isElevated = FALSE;
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD dwSize;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
            isElevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return isElevated == TRUE;
}

bool gettestsigning() {
    const char* cmd = "cmd.exe /C bcdedit /enum {current}";

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hStdOutRead, hStdOutWrite;
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        std::cerr << "Ошибка при создании pipe." << std::endl;
        return 0;
    }

    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    if (!CreateProcessA(NULL, (LPSTR)cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "Ошибка при создании процесса." << std::endl;
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        return 0;
    }

    CloseHandle(hStdOutWrite);

    char buffer[128];
    DWORD bytesRead;
    bool status{ false };
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        if (strstr(buffer, "testsigning") && strstr(buffer, "Yes")) {
            status = true;
            break;
        }
    }

    CloseHandle(hStdOutRead);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    
    return status;
}

void settestsinging(bool status)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "cmd.exe /C bcdedit /set testsigning " + std::string(status ? "on" : "off");

    if (!CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "Ошибка при создании процесса." << std::endl;
        return ;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void reboot()
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "cmd.exe /C shutdown /r /t 0";

    if (!CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "Ошибка при создании процесса." << std::endl;
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}


bool lauchgame(bool testSetup) {

    std::string folderPath{ dGame.dlls_FolderPath };
    std::string dlls{ dGame.getIncludedDlls() };
    std::string hud_params = dGame.hud.get();

    if (testSetup)
    {
        sMSG sendData;
        HANDLE hDevice = CreateFile("\\\\.\\BlueStreetDriver",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

        if (hDevice == INVALID_HANDLE_VALUE) {
            printf("[!] Failed to open device\n");
            return false;
        }

        int wpathLength = MultiByteToWideChar(CP_UTF8, 0, folderPath.c_str(), -1, NULL, 0);

        WCHAR* wpath = new WCHAR[wpathLength];

        MultiByteToWideChar(CP_UTF8, 0, folderPath.c_str(), -1, wpath, wpathLength);

        int wdllsLength = MultiByteToWideChar(CP_UTF8, 0, dlls.c_str(), -1, NULL, 0);

        WCHAR* wdlls = new WCHAR[wdllsLength];

        MultiByteToWideChar(CP_UTF8, 0, dlls.c_str(), -1, wdlls, wdllsLength);

        wcscpy_s(sendData.Path, wpath);
        wcscpy_s(sendData.Dlls, wdlls);

        DWORD returned;
        BOOL success = DeviceIoControl(hDevice,
            IOCTL_BLUESTREET_SEND_DATA,
            &sendData,
            sizeof(sendData),
            nullptr,
            0,
            &returned,
            nullptr);


        CloseHandle(hDevice);

        delete[] wpath;
        delete[] wdlls;
    }

    std::string dxvk_conf = fs::path(dGame.path).parent_path().string() + "\\dxvk.conf";

    if (fs::exists(dxvk_conf)) {

        std::ifstream inFile(dxvk_conf);
        if (inFile.is_open()) {
            std::string line;
            std::string output_t;
            std::string output;

            bool dxvk_dlls_FolderPath{ false };
            bool dxvk_dlls{ false };
            bool dxvk_hud{ false };

            while (std::getline(inFile, line)) {

                if (line.find("dxvk.dlls.FolderPath") != std::string::npos && !testSetup) {
                    dxvk_dlls_FolderPath = true;
                    line = "dxvk.dlls.FolderPath = \"" + dGame.dlls_FolderPath + "\"";
                }
                else if (line.find("dxvk.dlls") != std::string::npos && !testSetup) {
                    dxvk_dlls = true;
                    line = "dxvk.dlls = " + dlls;
                }
                else if (line.find("dxvk.hud") != std::string::npos) {
                    dxvk_hud = true;
                    line = "dxvk.hud = " + hud_params;
                }

                output_t += line + "\n";
            }

            inFile.close();

            if (!dxvk_dlls_FolderPath && !testSetup)
                output += "dxvk.dlls.FolderPath = \"" + dGame.dlls_FolderPath + "\"\n\n";

            if (!dxvk_dlls && !testSetup)
                output += "dxvk.dlls = " + dlls + "\n\n";

            if (!dxvk_hud)
                output += "dxvk.hud = " + hud_params + "\n\n";

            output += output_t;

            std::ofstream outFile(dxvk_conf);
            if (outFile.is_open()) {
                outFile << output;
                outFile.close();
                std::cout << "Параметры в файле " << dxvk_conf << " успешно изменены." << std::endl;
            }
        }
    }
    else {

        std::ofstream outFile(dxvk_conf);
        if (!outFile) {
            std::cerr << "Ошибка создания файла " << std::endl;
        }

        if(!testSetup)
            outFile /*  << "[" << fs::path(dGame.path).filename().string() << "]\n" */ 
                        << "dxvk.dlls.FolderPath = \"" << dGame.dlls_FolderPath << "\"\n\n"
                        << "dxvk.dlls = " << dlls << "\n";



        CURL* curl;
        CURLcode res;
        const char* url = "https://raw.githubusercontent.com/doitsujin/dxvk/master/dxvk.conf";
        std::string output;

        curl = curl_easy_init();
        if (curl) {
            // Установка URL для запроса
            curl_easy_setopt(curl, CURLOPT_URL, url);

            // Установка функции обратного вызова для записи данных в строку
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LibraryManager::WriteCallback);

            // Установка указателя на строку для записи
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

            // Выполнение запроса
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "Ошибка выполнения запроса: " << curl_easy_strerror(res) << std::endl;
                curl_easy_cleanup(curl);
                return 1;
            }

            // Закрытие curl
            curl_easy_cleanup(curl);
        }
        else {
            std::cerr << "Ошибка инициализации libcurl" << std::endl;
            return 1;
        }

        // Изменение строки в полученных данных
        std::string searchString = "# dxvk.hud = ";
        std::string replaceString = "dxvk.hud = " + hud_params;
        size_t pos = output.find(searchString);
        if (pos != std::string::npos) {
            output.replace(pos, searchString.length(), replaceString);
        }
        else {
            std::cerr << "Строка для замены не найдена" << std::endl;
            return 1;
        }

        if (!outFile) {
            std::cerr << "Ошибка открытия файла для записи" << std::endl;
            return 1;
        }
        outFile << output;
        outFile.close();
    }

    HINSTANCE result = ShellExecute(NULL, NULL, dGame.path.c_str(), testSetup ? "-dxvk" : "", fs::path(dGame.path).parent_path().string().c_str(), SW_SHOWNORMAL);
    if ((int)result <= 32) {
        std::wostringstream woss;
        woss << L"Failed to Create process. Error code: " << GetLastError();
        MessageBoxW(NULL, woss.str().c_str(), L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    return true;
}

uintmax_t getFolderSize(const fs::path& folderPath) {
    uintmax_t folderSize = 0;

    if (fs::exists(folderPath) && fs::is_directory(folderPath))
    {
        for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
            if (fs::is_regular_file(entry.status())) {
                folderSize += fs::file_size(entry);
            }
        }
        return folderSize / (1024 * 1024);
    }

    return folderSize;
}

void getDiskSpaceInfo(const std::string& drive, uintmax_t& totalBytes, uintmax_t& freeBytes) {
    ULARGE_INTEGER freeBytesAvailable, totalBytesTemp, totalFreeBytes;
    if (GetDiskFreeSpaceEx(drive.c_str(), &freeBytesAvailable, &totalBytesTemp, &totalFreeBytes)) {
        totalBytes = totalBytesTemp.QuadPart;
        freeBytes = totalFreeBytes.QuadPart;
    }
    else {
        std::cerr << "Error getting disk space information." << std::endl;
    }
}


//void LoadFontFromResource(ImFont*& font)
//{
//    HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(101), RT_FONT);
//    if (hResource)
//    {
//        HGLOBAL hGlobal = LoadResource(nullptr, hResource);
//        if (hGlobal)
//        {
//            const void* fontData = LockResource(hGlobal);
//            int fontSize = 18;
//
//            ImGuiIO& io = ImGui::GetIO();
//            ImFontAtlas* atlas = io.Fonts;
//
//            font = atlas->AddFontFromMemoryTTF(const_cast<void*>(fontData), hGlobal ? SizeofResource(nullptr, hResource) : 0, fontSize, nullptr, io.Fonts->GetGlyphRangesCyrillic());
//            io.Fonts->Build();
//            FreeResource(hGlobal);
//
//        }
//    }
//}

//std::vector<ReleaseInfo> GetReleases(const std::string& repository, const std::string& token, const std::string& jsonFilePath) {
//    std::string apiUrl = "https://api.github.com/repos/" + repository + "/releases";
//    std::vector<ReleaseInfo> releases;
//
//    /*if (std::filesystem::exists(jsonFilePath) && std::filesystem::file_size(jsonFilePath) > 0) {
//
//    }*/
//
//    CURL* curl;
//    CURLcode res;
//
//    curl_global_init(CURL_GLOBAL_DEFAULT);
//    curl = curl_easy_init();
//
//    if (curl) {
//        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
//        struct curl_slist* headers = nullptr;
//        headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
//        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
//        headers = curl_slist_append(headers, "User-Agent: Your-User-Agent-Name");  // �������� "Your-User-Agent-Name" �� ���� ��� ������������ ��� �������� ����������
//        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//
//        std::string responseString;
//        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
//
//        res = curl_easy_perform(curl);
//
//        
//        bool badAnswer { false };
//        bool apiError { false };
//        bool needUpdate { false };
//        if (res != CURLE_OK) {
//            std::cout << "Error get : " << std::endl;
//        }
//        else {
//            std::string content;
//            std::filesystem::create_directories(std::filesystem::path(jsonFilePath).parent_path());
//            std::fstream inputFile(jsonFilePath, std::ios::in);
//
//            if (inputFile.is_open()) {
//                std::stringstream buffer;
//                buffer << inputFile.rdbuf();
//                content = buffer.str();
//                inputFile.close();
//            }
//
//            json jsonResponse;
//            json jsonFile;
//           
//            try {
//                jsonResponse = json::parse(responseString);
//            }
//            catch (const json::parse_error& e) {
//
//                std::cout << "Error parsing response JSON : " << e.what() << std::endl;
//                badAnswer = true;
//            }
//
//            try {
//                jsonFile = json::parse(content);
//            }
//            catch (const json::parse_error& e) {
//                std::cout << "Error parsing file JSON: " << e.what() << std::endl;
//                if (badAnswer)
//                    goto EXIT;
//
//                needUpdate = true;
//            }
//
//            APIERROR:
//            for (const auto& release : badAnswer || apiError ? jsonFile : jsonResponse) {
//                ReleaseInfo releaseInfo;
//
//                if (release.find("name") != release.end() && release["name"].is_string()) {
//                    releaseInfo.version = release["name"].get<std::string>();
//                }
//                else {
//                    std::cout << responseString << std::endl;
//                    apiError = true;
//                    if (needUpdate)
//                        goto EXIT;
//                    goto APIERROR;
//                }
//
//                if (release.find("assets") != release.end() && release["assets"].is_array() &&
//                    !release["assets"].empty() && release["assets"][0].find("browser_download_url") != release["assets"][0].end() &&
//                    release["assets"][0]["browser_download_url"].is_string()) {
//                    releaseInfo.downloadUrl = release["assets"][0]["browser_download_url"].get<std::string>();
//                }
//                else {
//                    std::cout << responseString << std::endl;
//                    apiError = true;
//                    if (needUpdate)
//                        goto EXIT;
//                    goto APIERROR;
//                }
//
//                releaseInfo.downloadUrl = release["assets"][0]["browser_download_url"].get<std::string>();
//
//                if (!needUpdate && !badAnswer && !apiError)
//                {
//                    auto it = std::find_if(jsonFile.begin(), jsonFile.end(), [&](const auto& fileRelease) {
//                        ReleaseInfo fileReleaseInfo;
//                        fileReleaseInfo.version = fileRelease["name"].get<std::string>();
//                        fileReleaseInfo.downloadUrl = fileRelease["assets"][0]["browser_download_url"].get<std::string>();
//                        return CompareReleases(releaseInfo, fileReleaseInfo);
//                        });
//
//                    if (it != jsonFile.end()) {
//                        std::cout << releaseInfo.version << "\t" << releaseInfo.downloadUrl << "\n";
//                    }
//                    else {
//                        std::cout << "WRITE JSON FILE ->:\n\t\tRelease not found in the second JSON file: " << releaseInfo.version << "\t" << releaseInfo.downloadUrl << "\n";
//                        needUpdate = true;
//                    }
//                }
//
//                releases.push_back(releaseInfo);
//            }
//            if (needUpdate)
//            {
//                std::cout << "UPDATE FILE\n";
//                inputFile.open(jsonFilePath, std::ios::out | std::ios::trunc);
//                if (inputFile.is_open()) {
//                    inputFile << responseString;
//                    inputFile.close();
//                }
//            }
//        }
//        
//    EXIT:
//    std::cout << "badAnswer : " << badAnswer << std::endl;
//    std::cout << "apiError : " << apiError << std::endl;
//    std::cout << "needUpdate : " << needUpdate << std::endl;
//    
//        if(apiError && needUpdate)
//            MessageBoxA(NULL, "NETWORK && FILE", "ERROR", 0);
//        curl_easy_cleanup(curl);
//        curl_global_cleanup();
//    }
//
//    return releases;
//}
//
