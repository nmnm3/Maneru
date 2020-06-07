#include "config.h"

using namespace Maneru;
#include <fstream>
class ConfigCollection : public ConfigNode
{
public:
	bool LoadConfig(const char* filename)
	{
		configs.clear();
		std::ifstream input(filename);
		if (input.bad())
			return false;
		while (!input.eof())
		{
			std::string line;
			input >> line;
			size_t index = line.find('=');
			if (index == line.npos)
				return false;

			std::string key = line.substr(0, index);
			std::string value = line.substr(index + 1);
			configs[key] = value;
		}
		return true;
	}
};

static ConfigCollection globalConfig;

ConfigNode* Maneru::GetGlobalConfig()
{
	return &globalConfig;
}

bool Maneru::LoadConfig(const char *filename)
{
	return globalConfig.LoadConfig(filename);
}

bool HasPrefix(const std::string& s, const std::string& prefix)
{
	if (prefix.length() > s.length()) return false;
	for (int i = 0; i < prefix.length(); i++)
	{
		if (s[i] != prefix[i]) return false;
	}
	return true;
}

ConfigNode ConfigNode::GetNode(const std::string& prefix) const
{
	ConfigNode n;
	auto start = configs.lower_bound(prefix);
	for (auto it = start; it != configs.end(); it++)
	{
		if (HasPrefix(it->first, prefix))
		{
			if (it->first.length() > prefix.length())
				n.configs[it->first.substr(prefix.length() + 1)] = it->second;
		}
		else
			break;
	}
	return n;
}