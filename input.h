#pragma once
#include <windows.h>
#include <functional>
#include <map>
#include <string>
namespace Maneru
{

class ControllerInterface
{
public:
	virtual void Refresh() = 0;
	virtual void SetButtonFunction(const char* name, std::function<void(void)>& f, float repeatDelay, float repeatRate) = 0;
};

ControllerInterface* OpenController(int index);

typedef std::map<std::string, int> ControllerList;

ControllerList GetControllerList();

}