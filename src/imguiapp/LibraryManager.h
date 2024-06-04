#pragma once

#include "pch.h"

using json = nlohmann::json;

struct ProgressData {
    float lastPercentage;
    size_t taskId;
    double totalBytes;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastTime;
};

class LibraryManager {
private:
    struct sData {
        modify_types modify{ modify_types::none };
        install_types flag{ install_types::none };
        std::string version;
        std::string downloadUrl;
        bool pending{ false };

        void clear();
        std::string getfoldername() const;
        std::string getversion() const;
        std::string getversionFormated();
    };
    
public:
    LibraryManager();
    void parseVersion();
    void getInstalledVersions();
    void update();
    bool CompareReleases(const sData& a, const sData& b);

    void GetRelease(type typebin, const std::string token);
    bool ParseJson(const std::string& content, json& jsonObj);
    bool LoadJsonFromFile(const std::string& filePath, json& jsonObj);
    bool ExtractReleaseInfo(const json& release, sData& releaseInfo, bool isGitLab);
    bool IsReleaseInFile(const json& jsonFile, const sData& releaseInfo, bool isGitLab);
    void SaveJsonToFile(const std::string& filePath, const std::string& content);
    void Download(const size_t& taskId, sData& arr);
    void DownloadInThreads(int numThreads);
    std::string extractFileName(const std::string& url);
    void clear();
    std::string getIncludedDlls();
    void dllSort();
    void dllClear();



    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t WriteCallbackd(void* contents, size_t size, size_t nmemb, void* userp);
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

public:
    std::unordered_map<type, std::vector<sData>> data{};
    std::string path{};
    std::string dlls_FolderPath{};
    HUD hud{};
    std::vector<DllPath> dll{};
    std::vector<std::string> included_dlls{};
    bool menu{ false };

    static std::mutex mtx;
    static std::vector<float> taskProgress;
    static int count_download;
    static float progress;
};

