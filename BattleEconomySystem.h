#pragma once

#include "BattleEventFactory.h"

#include <algorithm>
#include <vector>

class BattleEconomySystem
{
public:
    static inline constexpr int PlayerWinXpReward = 120;
    static inline constexpr int FarmingActionInterval = 3;
    static inline constexpr int FarmingManaGain = 20;

    void ApplyTimedFarming(
        int playerActionCount,
        std::vector<Competitor>& playerTeam,
        std::vector<BattleEvent>& events) const
    {
        if (playerActionCount <= 0 || playerActionCount % FarmingActionInterval != 0)
        {
            return;
        }

        BattleEvent started = MakeBattleEvent(BattleEventType::FarmingTriggered, BattleActor::Player);
        started.amount = FarmingManaGain;
        started.reason = "Farm secured";
        events.push_back(started);

        for (int index = 0; index < static_cast<int>(playerTeam.size()); ++index)
        {
            Competitor& player = playerTeam[index];
            if (player.hp <= 0)
            {
                continue;
            }

            const int oldMana = player.mana;
            player.mana = std::clamp(player.mana + FarmingManaGain, 0, player.maxMana);
            if (player.mana == oldMana)
            {
                continue;
            }

            BattleEvent manaChanged = MakeActorEvent(
                BattleEventType::ManaChanged,
                MakeBattleEventParticipant(BattleActor::Player, index, player));
            manaChanged.oldValue = oldMana;
            manaChanged.newValue = player.mana;
            manaChanged.amount = player.mana - oldMana;
            manaChanged.reason = "farming";
            events.push_back(manaChanged);
        }
    }

    BattleRewardResult CreateBattleReward(
        BattleWinner winner,
        const std::vector<int>& participatingPlayerIndices,
        const std::vector<Competitor>& playerTeam) const
    {
        BattleRewardResult reward;
        if (winner != BattleWinner::Player || participatingPlayerIndices.empty())
        {
            return reward;
        }

        reward.awarded = true;
        reward.totalXp = PlayerWinXpReward;
        reward.xpPerParticipant = PlayerWinXpReward
            / static_cast<int>(participatingPlayerIndices.size());

        for (int playerIndex : participatingPlayerIndices)
        {
            reward.participantPlayerIndices.push_back(playerTeam[playerIndex].profileIndex);
        }

        return reward;
    }
};
