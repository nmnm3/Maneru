#pragma once
#include <windows.h>
#include <functional>
#include <vector>
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

std::vector<std::string> GetControllerList();

}