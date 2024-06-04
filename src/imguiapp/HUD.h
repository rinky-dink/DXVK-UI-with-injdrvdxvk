#pragma once
#include "pch.h"

class HUD {
public:
	float size_hud = 1.f;
	float opacity_hud = 1.f;

	struct ParamInfo {
		std::string name;
		bool active;
	};

	std::map<std::string, ParamInfo> params;

	HUD() {
		InitParams();
	}

	void AddParam(const std::string& paramName) {
		params[paramName] = ParamInfo{ paramName, false };
	}

	// ������������� ���������� ���������
	void SetParamActive(const std::string& paramName, bool isActive) {
		if (params.find(paramName) != params.end()) {
			params[paramName].active = isActive;
		}
	}

	// �������� ������ ���������� ���������
	bool IsParamActive(const std::string& paramName) const {
		auto it = params.find(paramName);
		return it != params.end() ? it->second.active : false;
	}

	// Implementing begin() and end() methods for range-based for loop
	auto begin() { return params.begin(); }
	auto end() { return params.end(); }
	auto begin() const { return params.cbegin(); }
	auto end() const { return params.cend(); }

	std::size_t Count() const {
		return params.size();
	}


	void update(const std::string& str) {
		std::istringstream iss(str);
		std::string token;
		while (std::getline(iss, token, ',')) {
			if (token.find("scale=") != std::string::npos) {
				std::string scale_str = token.substr(6); // �������� ��������� ����� "scale="
				std::replace(scale_str.begin(), scale_str.end(), '.', ','); // �������� ����� �� �������
				try {
					size_hud = std::stof(scale_str);
				}
				catch (...) {
					DbgPrint("Failed to parse scale value from token: %s\n", scale_str.c_str());
				}
			}
			else if (token.find("opacity=") != std::string::npos) {
				std::string opacity_str = token.substr(8); // �������� ��������� ����� "opacity="
				std::replace(opacity_str.begin(), opacity_str.end(), '.', ','); // �������� ����� �� �������
				try {
					opacity_hud = std::stof(opacity_str);
				}
				catch (...) {
					DbgPrint("Failed to parse opacity value from token: %s\n", opacity_str.c_str());

				}
			}
			else {

				SetParamActive(token, true);
			}
		}
	}

	std::string get() {
		std::string hud_params{};
		for (auto path : params) {
			if (path.second.active) {
				hud_params += path.second.name + ",";
			}
		}

		std::ostringstream ss;
		ss << std::fixed << std::setprecision(2) << size_hud;
		std::string size_hud_str = ss.str();

		// �������� ������� �� �����
		for (auto& c : size_hud_str) {
			if (c == ',') {
				c = '.';
			}
		}

		if (hud_params.empty())
			return "0";

		// ��������� ������ � hud_params
		hud_params += "scale=" + size_hud_str + ",";

		// ����������� ����� � ������ � ����� ������� ����� �����
		ss.str(""); // ������� stringstream
		ss << std::fixed << std::setprecision(2) << opacity_hud;
		std::string opacity_hud_str = ss.str();

		// �������� ������� �� �����
		for (auto& c : opacity_hud_str) {
			if (c == ',') {
				c = '.';
			}
		}

		// ��������� ������ � hud_params
		hud_params += "opacity=" + opacity_hud_str;

		return hud_params;
	}

private:


	void InitParams() {
		static const std::vector<std::string> paramNames = {
			"full", "fps", "frametimes", "submissions", "drawcalls", "pipelines",
			"descriptors", "memory", "gpuload", "version", "api", "cs", "compiler", "samplers"
		};

		for (const auto& name : paramNames) {
			AddParam(name);
		}
	}
};
