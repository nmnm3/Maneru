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
	CCPlan Next(CCMove& m, int incoming);
	void HardReset(CCPiece* hold, bool* board, int remainBagMask, int combo);
	void SoftReset(bool* board, int combo);
private:
	CCAsyncBot* bot;
	CCOptions options;
	CCWeights weights;
	CCPlanPlacement plans[MAX_NEXT_MOVE];
};

}