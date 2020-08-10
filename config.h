#pragma once
#include <map>
#include <string>
namespace Maneru
{

enum ConfigValueKind
{
	String,
	Int,
	Float,
	Bool,
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
	if (str == "yes")
	{
		value = true;
		return true;
	}
	if (str == "no")
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