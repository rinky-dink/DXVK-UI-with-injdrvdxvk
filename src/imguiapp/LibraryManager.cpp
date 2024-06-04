#include "pch.h"
#include "LibraryManager.h"

std::mutex LibraryManager::mtx;
std::vector<float> LibraryManager::taskProgress;
int LibraryManager::count_download = 0;
float LibraryManager::progress = 0.0f;

type GetType(std::string& str)
{
    if (str == "dxvk")
        return DXVK;
    return unknown;
}


void LibraryManager::sData::clear() {
    modify = modify_types::none;
    flag = install_types::none;
    pending = false;
}

std::string LibraryManager::sData::getfoldername() const {
    std::string bin;
    bool gplasync{ false };

    if (downloadUrl.find("doitsujin/dxvk") != std::string::npos)
        bin = "dxvk-";
    else if (downloadUrl.find("HansKristian-Work/vkd3d-proton") != std::string::npos)
        bin = "vkd3d-proton-";
    else if (downloadUrl.find("Ph42oN/dxvk-gplasync") != std::string::npos)
    {
        gplasync = true;
        bin = "dxvk-gplasync-";
    }
    else
        return "error";

    return fs::absolute("installed\\" + bin + (gplasync ? "v" : "" ) + getversion()).string();
}

std::string LibraryManager::sData::getversion() const {
    std::regex versionRegex(R"((?:[vV]ersion\s*)?([\d.]+(?:-\d+)?))");
    std::smatch match;
    if (!std::regex_search(version, match, versionRegex))
        return "";

    return match[1].str();
}

std::string LibraryManager::sData::getversionFormated(){
    std::regex re("^v");
    return std::regex_replace(version, re, "Version ");
}

// Определения методов LibraryManager
LibraryManager::LibraryManager() : menu(false) {}

void LibraryManager::parseVersion() {
    std::regex versionRegex("(.+)-(.+)");
    std::smatch match;

    std::string path = std::filesystem::path(dlls_FolderPath).filename().string();

    if (std::regex_search(path, match, versionRegex)) {
        std::string name = match[1].str();
        std::string version = match[2].str();

        for (auto& i : data[GetType(name)]) {
            if (i.getversion() == version) {
                for (const auto& entry : std::filesystem::directory_iterator(dlls_FolderPath + "\\x64")) {
                    if (entry.is_regular_file()) {
                        std::string filename = entry.path().filename().string();
                        auto it = std::find(included_dlls.begin(), included_dlls.end(), filename);

                        dll.push_back(DllPath(filename, it != included_dlls.end()));
                    }
                }

                //std::sort(dll.begin(), dll.end(), customSort);
                i.pending = true;
            }
        }
    }
}

void LibraryManager::getInstalledVersions() {
    std::regex versionRegex("^([^-]+(?:-[A-Za-z]+)*)-v?([0-9]+(?:\\.[0-9]+)*(-.+)?)$");
    std::smatch match;
    std::string install_folder{ "installed" };
    std::filesystem::create_directories(std::filesystem::path(install_folder));
    DbgPrint("Found: ");
    for (const auto& entry : std::filesystem::directory_iterator(install_folder)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();

            if (std::regex_search(folderName, match, versionRegex)) {
                if (match.size() > 2) {
                    type index;
                    
                    if (match[1].str() == "dxvk")
                        index = DXVK;
                    else if(match[1].str() == "dxvk-gplasync")
                        index = DXVK_GPLASYNC;

                    
                    for (auto& str : data[index]) {
                        
                        if ( str.getversionFormated() == "Version " + match[2].str()) {
                            DbgPrint("\"%s\";\t", match[0].str().c_str());
                            str.flag = installed;
                            break;
                        }
                       /* else
                            DbgPrint("%s %s %s", str.getversionFormated(), match[1].str().c_str(), match[3].str().c_str() );*/
                    }
                }
            }
        }
    }
    DbgPrint("\n");
}

void LibraryManager::update() {
    clear();
    if (!path.empty()) {
        std::string dxvk_conf = std::filesystem::path(path).parent_path().string() + "\\dxvk.conf";
        std::ifstream inFile(dxvk_conf);

        std::string line;

        while (std::getline(inFile, line)) {
            if (line.find("dxvk.dlls.FolderPath") != std::string::npos) {
                size_t pos = line.find("\"");
                if (pos != std::string::npos) {
                    dlls_FolderPath = line.substr(pos + 1, line.find("\"", pos + 1) - pos - 1);
                }
            }
            else if (line.find("dxvk.dlls") != std::string::npos) {
                size_t pos = line.find("= ");
                if (pos != std::string::npos) {
                    std::istringstream iss(line.substr(pos + 2));
                    std::string dll;
                    while (std::getline(iss, dll, ',')) {
                        included_dlls.push_back(dll);
                    }
                }
            }
            else if (line.find("dxvk.hud") != std::string::npos) {
                size_t pos = line.find("= ");
                if (pos != std::string::npos) {
                    hud.update(line.substr(pos + 2));
                }
            }
        }
        DbgPrint("dlls_FolderPath = %s\n", dlls_FolderPath.c_str());
        DbgPrint("hud.get() = %s\n", hud.get().c_str());
    }
}



bool LibraryManager::CompareReleases(const sData& a, const sData& b) {
    return a.version == b.version && a.downloadUrl == b.downloadUrl;
}

void LibraryManager::GetRelease(type typebin, const std::string token) {
    std::string jsonFilePath;
    std::string repository;
    bool isGitLab{ false };

    // Определяем параметры репозитория и путь к JSON файлу
    switch (typebin) {
    case DXVK:
        repository = "doitsujin/dxvk";
        jsonFilePath = "json\\dxvk.json";
        break;
    case DXVK_GPLASYNC:
        repository = "43488626";
        jsonFilePath = "json\\dxvk_gplasync.json";
        isGitLab = true;
        break;
    case VKD3D:
        repository = "HansKristian-Work/vkd3d-proton";
        jsonFilePath = "json\\vkd3d.json";
        break;
    default:
        repository = "unknown";
        jsonFilePath = "json\\unknown.json";
        break;
    }

    std::string apiUrl = (isGitLab ? "https://gitlab.com/api/v4/projects/" : "https://api.github.com/repos/") + repository + "/releases";

    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        struct curl_slist* headers = nullptr;
        std::string authHeader = "Authorization: Bearer " + token;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, isGitLab ? "Accept: application/json" : "Accept: application/vnd.github.v3+json");
        if (!isGitLab)
            headers = curl_slist_append(headers, "User-Agent: Your-User-Agent-Name");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string responseString;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &LibraryManager::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            DbgPrint("Error get:\n");
        }
        else {
            std::filesystem::create_directories(std::filesystem::path(jsonFilePath).parent_path());
            json jsonResponse;
            json jsonFile;
            bool needUpdate = false;

            if (!ParseJson(responseString, jsonResponse)) {
                DbgPrint("Error parsing JSON\n");
                needUpdate = true;
                return;
            }
            
            if (!LoadJsonFromFile(jsonFilePath, jsonFile)) {
                SaveJsonToFile(jsonFilePath, responseString);

                DbgPrint("Updating JSON file\n");
                if (!LoadJsonFromFile(jsonFilePath, jsonFile))
                {
                    DbgPrint("Error LoadJsonFromFile JSON\n");
                    return;
                }
            }

            for (const auto& release : jsonResponse) {
                sData releaseInfo;
                if (!ExtractReleaseInfo(release, releaseInfo, isGitLab)) {
                    DbgPrint("Error extracting release info\n");
                    continue;
                }

                if (!IsReleaseInFile(jsonFile, releaseInfo, isGitLab)) {
                    DbgPrint("Release not found in the second JSON file: %s \t %s\n", releaseInfo.version.c_str(), releaseInfo.downloadUrl.c_str());
                    needUpdate = true;
                }
                else {
                    DbgPrint("%s \t %s\n", releaseInfo.version.c_str(), releaseInfo.downloadUrl.c_str());
                }

                data[typebin].push_back(releaseInfo);
            }

            if (needUpdate) {
                DbgPrint("Updating JSON file\n");
                SaveJsonToFile(jsonFilePath, responseString);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
}

bool LibraryManager::ParseJson(const std::string& content, json& jsonObj) {
    try {
        jsonObj = json::parse(content);
        return true;
    }
    catch (const json::parse_error& e) {
        DbgPrint("Error parsing JSON: %s\n", e.what());
        return false;
    }
}

bool LibraryManager::LoadJsonFromFile(const std::string& filePath, json& jsonObj) {
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    inputFile.close();

    return ParseJson(buffer.str(), jsonObj);
}

bool LibraryManager::ExtractReleaseInfo(const json& release, sData& releaseInfo, bool isGitLab) {
    if (release.find("name") == release.end() || !release["name"].is_string()) {
        return false;
    }
    releaseInfo.version = release["name"].get<std::string>();
    
    if (release.find("assets") == release.end() || !(isGitLab ? release["assets"].is_object() : release["assets"].is_array()) ) {
        return false;
    }

    if (isGitLab) {
        if (!release["assets"]["links"][0].find("url")->is_string()) {
            DbgPrint("GitLab assets links do not contain valid URL: %s\n", release.dump().c_str());
            return false;
        }
        releaseInfo.downloadUrl = release["assets"]["links"][0]["url"].get<std::string>();
    }
    else {
        if (release["assets"].empty() || !release["assets"][0].find("browser_download_url")->is_string()) {
            return false;
        }
        releaseInfo.downloadUrl = release["assets"][0]["browser_download_url"].get<std::string>();
    }

    return true;
}

bool LibraryManager::IsReleaseInFile(const json& jsonFile, const sData& releaseInfo, bool isGitLab) {
    return std::any_of(jsonFile.begin(), jsonFile.end(), [&](const auto& fileRelease) {
        sData fileReleaseInfo;
        if (!ExtractReleaseInfo(fileRelease, fileReleaseInfo, isGitLab)) {
            return false;
        }
        return CompareReleases(releaseInfo, fileReleaseInfo);
        });
}

void LibraryManager::SaveJsonToFile(const std::string& filePath, const std::string& content) {
    std::ofstream outputFile(filePath);
    if (outputFile.is_open()) {
        outputFile << content;
        outputFile.close();
    }
}


void LibraryManager::Download(const size_t& taskId, sData& arr) {
    if (arr.flag == installed && arr.modify == modify_types::reinstall) {
        try {
            std::filesystem::remove_all(arr.getfoldername());
            arr.flag = install_types::none;
            DbgPrint("Файл(%s) успешно удален.\n", arr.getfoldername().c_str());
        }
        catch (const std::filesystem::filesystem_error& e) {
            arr.flag = install_types::error;
            DbgPrint("Не удалось удалить файл(%s): %s\n", arr.getfoldername().c_str(), e.what());
        }
    }

    CURL* curl;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    if (curl) {
        std::filesystem::path tempPath = std::filesystem::temp_directory_path();
        std::filesystem::path filePath = tempPath / extractFileName(arr.downloadUrl);
        
        std::ofstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            
            ProgressData progressData;
            progressData.lastPercentage = 0.0;
            progressData.taskId = taskId;
            progressData.startTime = std::chrono::steady_clock::now();
          
            curl_easy_setopt(curl, CURLOPT_URL, arr.downloadUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &LibraryManager::WriteCallbackd);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &LibraryManager::progressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressData);

            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);

            CURLcode res = curl_easy_perform(curl);

            file.close();

            if (res != CURLE_OK) {
                DbgPrint("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }
            else {
                std::string command = "C:\\Windows\\System32\\tar.exe -xkzf " + filePath.string() + " -C " + ".\\installed";

                try {
                    std::string output;
                    boost::process::child process(command, boost::process::std_out > output, boost::process::windows::hide);
                    DbgPrint("output: %s", command.c_str());
                    process.wait();

                    int exitCode = process.exit_code();

                    if (exitCode == 0) {
                        arr.flag = install_types::installed;
                        arr.modify = modify_types::none;
                        arr.pending = false;

                        DbgPrint("Successfully installed(%s)\n", filePath.string().c_str());
                    }
                    else {
                        arr.flag = install_types::error;
                        DbgPrint("Extraction failed with exit code: %d\n", exitCode);
                    }
                }
                catch (const boost::process::process_error& e) {
                    DbgPrint("Error during extraction: %s\n", e.what());
                }
            }
        }
        else {
            DbgPrint("The file(%s) could not be opened for writing.\n", filePath.string().c_str());
        }

        try {
            std::filesystem::remove(filePath);
            DbgPrint("The file(%s) was successfully deleted.\n", filePath.string().c_str());
        }
        catch (const std::filesystem::filesystem_error& e) {
            DbgPrint("The file(%s) could not be deleted: %s\n", filePath.string().c_str(), e.what());
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void LibraryManager::DownloadInThreads(int numThreads) {
    progress = 0;


    for (auto& [key, vec] : dGame.data) {
        for (auto& i : vec) {
            if ((i.pending && i.flag != install_types::installed) || (i.pending && i.modify == modify_types::reinstall))
                count_download++;
        }
    }

    taskProgress.resize(count_download);

    std::vector<std::thread> threads;
    size_t j = 0;
    for (auto& [key, vec] : dGame.data) {
        for (auto& i : vec) {
            if (i.pending && i.flag != install_types::installed || (i.pending && i.modify == modify_types::reinstall)) {
                threads.emplace_back([this, &vec, taskId = j++](sData& arr) { Download(taskId, arr); }, std::ref(i));

            }
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }


    count_download = 0;
    progress = 1;
    taskProgress.clear();
    taskProgress.resize(0);
}

std::string LibraryManager::extractFileName(const std::string& url) {
    std::regex pattern(R"(/([^/]+)$)");
    std::smatch match;

    if (std::regex_search(url, match, pattern)) {
        std::string str{ match[1] };
        std::size_t pos = str.find('?');
        if (pos != std::string::npos) {
            return str.substr(0, pos);
        }
        return str;
    }
    else {
        return url;
    }
}

size_t LibraryManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

size_t LibraryManager::WriteCallbackd(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    file->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}
double speedMbps;
int LibraryManager::progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
    ProgressData* progressData = static_cast<ProgressData*>(clientp);

    if (dltotal > 0) {
        float percentage = (dlnow / (float)dltotal) * 100.0;

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - progressData->startTime);

       
        if (duration.count() > 0) { // избегаем деления на ноль
            curl_off_t downloadedBytes = dlnow;

            // Скорость загрузки в байтах на микросекунду
            double speedBytesPerMicrosecond = downloadedBytes / ((double)duration.count() / 1e6);
            DbgPrint("sda %.2f \n", ((double)duration.count() / 1e6));

            // Перевод скорости в мегабайты в секунду
            speedMbps = speedBytesPerMicrosecond / (1024 * 1024);

            // Вывод скорости загрузки
            DbgPrint("out %.2f \n", speedMbps);
        }

        DbgPrint("Downloaded: %lld of %lld bytes\n", dlnow, dltotal);

        DbgPrint("%lld \n", duration.count());
        if (percentage - progressData->lastPercentage > 1.0) {
            std::lock_guard<std::mutex> lock(mtx);

            taskProgress[progressData->taskId] = percentage;
            float totalProgress = 0;

            for (auto& progress : taskProgress)
                totalProgress += progress;

            progress = totalProgress / 100 / count_download;

            progressData->lastPercentage = percentage;
        }
    }

    return 0;
}

void LibraryManager::clear() {
    for (auto& i : data[DXVK]) {
        i.pending = false;
    }
    dll.clear();
    included_dlls.clear();

    for (auto& i : hud) {
        i.second.active = false;
    }

    hud.opacity_hud = 1.f;
    hud.size_hud = 1.f;
    dlls_FolderPath = "";
}

std::string LibraryManager::getIncludedDlls()
{
    std::string dlls{};
    for (auto path : dGame.dll)
        if (path.include)
            dlls += path.dll + ",";

    if (!dlls.empty()) {
        dlls.resize(dlls.size() - 1);
    }
    return dlls;
}

void LibraryManager::dllSort()
{
    auto customSort = [](const DllPath& a, const DllPath& b) {
        auto extractNumber = [](const std::string& s) {
            std::smatch match;
            std::regex_search(s, match, std::regex(R"(d3d([\d]+))"));
            if (!match.empty()) {
                return std::stoi(match[1]);
            }
            return -1;
            };

        int number_a = extractNumber(a.dll);
        int number_b = extractNumber(b.dll);

        if (number_a != number_b) {
            return number_a < number_b;
        }

        else {
            return a.dll < b.dll;
        }
        };

    std::sort(dll.begin(), dll.end(), customSort);
}

void LibraryManager::dllClear()
{
   dll.clear();
}
