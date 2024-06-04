#pragma once
enum type {
	DXVK,
	DXVK_GPLASYNC,
	VKD3D,
	unknown
};

enum install_types
{
	error = -1,
	none,
	installed,
};

enum class modify_types
{
	none,
	reinstall,
	del,
	maxvalue
};

enum FlagsState {
	Flag_None = 0,
	Flag_ShowOnlyInstalled = 1 << 0,
	Flag_ModifyWindow = 1 << 1
};