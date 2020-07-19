#pragma once

extern "C"
{
#include <coldclear.h>
}

namespace Maneru
{

const int MAX_NEXT_MOVE = 32;
class CCBot
{
public:
	CCBot();
	~CCBot();
	void Start(bool is20g);
	void Stop();
	void AddPiece(int p);
	int Next(CCMove& m, int incoming);
	void HardReset(CCPiece* hold, bool* board, int remainBagMask, int combo);
	void SoftReset(bool* board, int combo);
	const CCPlanPlacement* GetPlan(int index) const;
private:
	CCAsyncBot* bot;
	CCOptions options;
	CCWeights weights;
	CCPlanPlacement plans[MAX_NEXT_MOVE];
	int validPlans;
};

}