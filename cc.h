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
	int Query(CCMove& m, unsigned char* x, unsigned char* y, CCPlanPlacement* plans, int numPlans);
	void Advance(unsigned char* x, unsigned char* y);
	int Next(CCMove& m, int incoming, CCPlanPlacement* plans, int numPlans);
	void HardReset(CCPiece* hold, bool* board, int remainBagMask, int combo);
	void SoftReset(bool* board, int combo);
private:
	CCAsyncBot* bot;
	CCOptions options;
	CCWeights weights;
};

}