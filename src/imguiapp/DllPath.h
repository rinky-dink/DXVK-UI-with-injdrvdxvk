#pragma once
struct DllPath {
	std::string dll{};
	bool include{ false };

};
bool customSort(const DllPath& a, const DllPath& b);
