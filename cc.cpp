#include "cc.h"
#include "config.h"
using namespace Maneru;
#pragma comment (lib, "cold_clear.dll.lib")

CCBot::CCBot() : plans(), validPlans(0)
{
	ConfigNode n = GetGlobalConfig()->GetNode("cc");

	cc_default_options(&options);
	cc_default_weights(&weights);
	n.GetValue("min_nodes", options.min_nodes);
	n.GetValue("max_nodes", options.max_nodes);
	n.GetValue("threads", options.threads);
	n.GetValue("use_hold", options.use_hold);
	n.GetValue("speculate", options.speculate);
	n.GetValue("pcloop", options.pcloop);

	n.GetValue("b2b_clear", weights.b2b_clear);
	n.GetValue("clear1", weights.clear1);
	n.GetValue("clear2", weights.clear2);
	n.GetValue("clear3", weights.clear3);
	n.GetValue("clear4", weights.clear4);
	n.GetValue("tspin1", weights.tspin1);
	n.GetValue("tspin2", weights.tspin2);
	n.GetValue("tspin3", weights.tspin3);
	n.GetValue("mini_tspin1", weights.mini_tspin1);
	n.GetValue("mini_tspin2", weights.mini_tspin2);
	n.GetValue("perfect_clear", weights.perfect_clear);
	n.GetValue("combo_garbage", weights.combo_garbage);
	n.GetValue("move_time", weights.move_time);
	n.GetValue("wasted_t", weights.wasted_t);

	bot = nullptr;
}

CCBot::~CCBot()
{
	cc_destroy_async(bot);
}

void Maneru::CCBot::Start(bool is20g)
{
	options.mode = is20g ? CC_20G : CC_0G;
	bot = cc_launch_async(&options, &weights);
}

void CCBot::Stop()
{
	cc_destroy_async(bot);
	bot = nullptr;
}

void CCBot::AddPiece(int p)
{
	if (bot) cc_add_next_piece_async(bot, (CCPiece)p);
}

int CCBot::Next(CCMove & m, int incoming)
{
	cc_request_next_move(bot, incoming);
	uint32_t size = MAX_NEXT_MOVE;
	CCBotPollStatus status = cc_block_next_move(bot, &m, plans, &size);
	if (status != CC_MOVE_PROVIDED)
	{
		return 0;
	}

	int ymap[40];
	for (int i = 0; i < 40; i++)
	{
		ymap[i] = i;
	}

	for (int i = 0; i < size; i++)
	{
		CCPlanPlacement& p = plans[i];
		for (int j = 0; j < 4; j++)
		{
			p.expected_y[j] = ymap[p.expected_y[j]];
		}
		for (int j = 3; j >= 0; j--)
		{
			int c = p.cleared_lines[j];
			if (c == -1) continue;
			if (c < 0 || c > 40)
			{
				return 0;
			}

			for (int k = c; k < 39; k++)
			{
				ymap[k] = ymap[k + 1];
			}
		}
	}
	validPlans = size;
	return size;
}

void CCBot::HardReset(CCPiece * hold, bool* board, int remainBagMask, int combo)
{
	if (bot) cc_destroy_async(bot);
	bot = cc_launch_with_board_async(&options, &weights, board, remainBagMask, hold, false, combo);
}

void CCBot::SoftReset(bool * board, int combo)
{
	if (bot) cc_reset_async(bot, board, false, combo);
}

const CCPlanPlacement* CCBot::GetPlan(int index) const
{
	if (index < validPlans)
	{
		return plans + index;
	}
	return nullptr;
}
