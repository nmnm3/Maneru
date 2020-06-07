#pragma once

extern "C"
{
#include <coldclear.h>
}

namespace Maneru
{

struct CCPlan
{
	CCPlanPlacement* plans;
	unsigned int n;
};

const int MAX_NEXT_MOVE = 32;
class CCBot
{
public:
	CCBot();
	~CCBot();
	void AddPiece(int p);
	CCPlan Next(CCMove& m);
	void ResetBot(bool* board);
private:
	CCAsyncBot* bot;
	CCPlanPlacement plans[MAX_NEXT_MOVE];
};

}