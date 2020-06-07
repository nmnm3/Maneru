#pragma once
#include <map>
#include <string>
namespace Maneru
{
struct ControlConfig
{
	float repeatDelay;
	float repeatSpeed;

	// Button mapping
	const char *a, *b, *x, *y;
	const char *left, *right, *up, *down;
	const char *lb, *rb;
	const char *menu, *start;
};

struct ColdClearConfig
{
	bool use_hold;
	bool speculate;
	bool pcloop;
	uint32_t min_nodes;
	uint32_t max_nodes;
	uint32_t threads;
};

struct GraphicConfig
{
	int fps;
};

struct GameConfig
{
	int test;
};

enum ConfigValueKind
{
	String,
	Int,
	Float,
	Bool,
};

struct Config
{
	GraphicConfig graphic;
	ColdClearConfig cc;
	ControlConfig ctrl;
};

template <typename T>
inline bool ParseValue(const std::string& str, T& value)
{
	__debugbreak();
	return false;
}

template <>
inline bool ParseValue(const std::string& str, int& value)
{
	try
	{
		value = std::stoi(str);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

template <>
inline bool ParseValue(const std::string& str, float& value)
{
	try
	{
		value = std::stof(str);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

template <>
inline bool ParseValue(const std::string& str, unsigned int& value)
{
	try
	{
		value = std::stoi(str);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

template <>
inline bool ParseValue(const std::string& str, bool& value)
{
	if (str == "true")
	{
		value = true;
		return true;
	}
	if (str == "false")
	{
		value = false;
		return true;
	}
	return false;
}

template <>
inline bool ParseValue(const std::string& str, std::string& value)
{
	value = str;
	return true;
}

class ConfigNode
{
public:
	ConfigNode GetNode(const std::string& prefix) const;
	template<typename T>
	bool GetValue(const std::string& name, T& value) const
	{
		auto it = configs.find(name);
		if (it == configs.end())
			return false;
		return ParseValue(it->second, value);
	}
protected:
	std::map<std::string, std::string> configs;
};

ConfigNode* GetGlobalConfig();
bool LoadConfig(const char* filename);

}