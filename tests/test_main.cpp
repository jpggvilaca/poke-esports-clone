#include "BattleSession.h"
#include "PlayerProfileSystem.h"
#include "RatingSystem.h"
#include "SimulationData.h"
#include "TrainerProfile.h"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string ToText(bool value)
    {
        return value ? "true" : "false";
    }

    std::string ToText(const std::string& value)
    {
        return "\"" + value + "\"";
    }

    std::string ToText(const char* value)
    {
        return "\"" + std::string(value) + "\"";
    }

    std::string ToText(Spec value)
    {
        return ToString(value);
    }

    std::string ToText(Style value)
    {
        return ToString(value);
    }

    std::string ToText(CareerRank value)
    {
        return ToString(value);
    }

    std::string ToText(SimulationError value)
    {
        switch (value)
        {
        case SimulationError::None: return "None";
        case SimulationError::UnknownGameType: return "UnknownGameType";
        case SimulationError::BattleNeedsPlayerProfile: return "BattleNeedsPlayerProfile";
        case SimulationError::UnknownSpec: return "UnknownSpec";
        case SimulationError::UnknownPlayerProfileSpec: return "UnknownPlayerProfileSpec";
        case SimulationError::UnknownStyle: return "UnknownStyle";
        case SimulationError::UnknownPlayerProfileStyle: return "UnknownPlayerProfileStyle";
        case SimulationError::UnknownActivePlayerProfile: return "UnknownActivePlayerProfile";
        case SimulationError::BattleNotStarted: return "BattleNotStarted";
        case SimulationError::BattleAlreadyFinished: return "BattleAlreadyFinished";
        case SimulationError::UnknownSkill: return "UnknownSkill";
        case SimulationError::SkillUnavailableForStyle: return "SkillUnavailableForStyle";
        case SimulationError::InsufficientFocus: return "InsufficientFocus";
        case SimulationError::StyleAlreadyActive: return "StyleAlreadyActive";
        case SimulationError::UnknownPlayerProfile: return "UnknownPlayerProfile";
        case SimulationError::PlayerProfileAlreadyActive: return "PlayerProfileAlreadyActive";
        case SimulationError::PlayerProfileCannotPlay: return "PlayerProfileCannotPlay";
        case SimulationError::LevelsMustBePositive: return "LevelsMustBePositive";
        case SimulationError::SkillAlreadyLearned: return "SkillAlreadyLearned";
        case SimulationError::SkillNotLearned: return "SkillNotLearned";
        case SimulationError::SkillAlreadyActive: return "SkillAlreadyActive";
        case SimulationError::ActiveSkillSlotsFull: return "ActiveSkillSlotsFull";
        case SimulationError::SkillNotActive: return "SkillNotActive";
        case SimulationError::NegativeXpAward: return "NegativeXpAward";
        case SimulationError::TrophyAlreadyEarned: return "TrophyAlreadyEarned";
        case SimulationError::RosterFull: return "RosterFull";
        }

        return "Unknown";
    }

    std::string ToText(BattleEventType value)
    {
        switch (value)
        {
        case BattleEventType::None: return "None";
        case BattleEventType::BattleStarted: return "BattleStarted";
        case BattleEventType::StyleChanged: return "StyleChanged";
        case BattleEventType::PlayerSwitched: return "PlayerSwitched";
        case BattleEventType::SkillStarted: return "SkillStarted";
        case BattleEventType::FocusChanged: return "FocusChanged";
        case BattleEventType::AttackMissed: return "AttackMissed";
        case BattleEventType::DamageApplied: return "DamageApplied";
        case BattleEventType::HealingApplied: return "HealingApplied";
        case BattleEventType::StatusApplied: return "StatusApplied";
        case BattleEventType::SkillXpGained: return "SkillXpGained";
        case BattleEventType::SkillLeveledUp: return "SkillLeveledUp";
        case BattleEventType::BattleFinished: return "BattleFinished";
        case BattleEventType::RewardGranted: return "RewardGranted";
        }

        return "Unknown";
    }

    std::string ToText(BattleWinner value)
    {
        switch (value)
        {
        case BattleWinner::None:
            return "None";
        case BattleWinner::Player:
            return "Player";
        case BattleWinner::Opponent:
            return "Opponent";
        }

        return "Unknown";
    }

    std::string ToText(BattleActor value)
    {
        switch (value)
        {
        case BattleActor::None:
            return "None";
        case BattleActor::Player:
            return "Player";
        case BattleActor::Opponent:
            return "Opponent";
        }

        return "Unknown";
    }

    template <typename Value>
    std::string ToText(const Value& value)
    {
        std::ostringstream output;
        output << value;
        return output.str();
    }

    struct TestContext
    {
        int failures = 0;

        void Expect(bool condition, const std::string& message)
        {
            if (!condition)
            {
                ++failures;
                std::cout << "  FAIL: " << message << "\n";
            }
        }

        template <typename Actual, typename Expected>
        void ExpectEqual(
            const Actual& actual,
            const Expected& expected,
            const std::string& message)
        {
            if (!(actual == expected))
            {
                ++failures;
                std::cout << "  FAIL: " << message
                    << " (expected " << ToText(expected)
                    << ", got " << ToText(actual) << ")\n";
            }
        }
    };

    bool ContainsInt(const std::vector<int>& values, int expected)
    {
        for (int value : values)
        {
            if (value == expected)
            {
                return true;
            }
        }

        return false;
    }

    bool ContainsEventType(const std::vector<BattleEvent>& events, BattleEventType expected)
    {
        for (const BattleEvent& event : events)
        {
            if (event.type == expected)
            {
                return true;
            }
        }

        return false;
    }

    BattleSetup::PlayerSlot MakeBattleSlot(
        int profileIndex,
        const std::string& name,
        Spec spec,
        const std::string& basicSkillId)
    {
        BattleSetup::PlayerSlot slot;
        slot.profileIndex = profileIndex;
        slot.name = name;
        slot.spec = spec;
        slot.style = Style::Balanced;
        slot.passiveBonuses.maxHpBonus = 10000;
        slot.passiveBonuses.basePowerBonus = 500;
        slot.skills.push_back({ basicSkillId, 80, 0 });
        return slot;
    }

    void TestPlayerProfileSystem(TestContext& test)
    {
        SimulationData data;
        PlayerProfileSystem profiles(data);

        PlayerProfileState player = profiles.CreateStarter("Rookie Top", Spec::Top);
        test.ExpectEqual(player.name, std::string("Rookie Top"), "starter keeps the requested name");
        test.ExpectEqual(player.spec, Spec::Top, "starter keeps the requested spec");
        test.ExpectEqual(player.level, 1, "starter begins at level 1");
        test.ExpectEqual(player.xp, 0, "starter begins with no XP");
        test.ExpectEqual(player.xpRequiredForNextLevel, 100, "level 1 needs 100 XP");
        test.ExpectEqual(player.rank, CareerRank::Rookie, "starter begins as Rookie");
        test.ExpectEqual(player.learnedSkillIds.size(), static_cast<std::size_t>(4), "starter learns four skills");
        test.ExpectEqual(player.activeSkillIds.size(), static_cast<std::size_t>(4), "starter equips four skills");
        test.ExpectEqual(player.learnedSkillIds[0], std::string("top-basic"), "starter loadout begins with the spec basic");

        ProfileCommandResult equipUnknown = profiles.EquipSkill(player, "support-basic");
        test.Expect(!equipUnknown.accepted, "cannot equip a skill before learning it");
        test.ExpectEqual(equipUnknown.errorCode, SimulationError::SkillNotLearned, "equip reject has a stable error code");

        ProfileCommandResult learnRecover = profiles.LearnSkill(player, "top-recover");
        test.Expect(learnRecover.accepted, "can learn a new skill");
        test.Expect(profiles.HasLearnedSkill(player, "top-recover"), "learned skill is tracked");

        ProfileCommandResult duplicateLearn = profiles.LearnSkill(player, "top-recover");
        test.Expect(!duplicateLearn.accepted, "cannot learn the same skill twice");

        ProfileCommandResult equipWhenFull = profiles.EquipSkill(player, "top-recover");
        test.Expect(!equipWhenFull.accepted, "cannot equip when all active slots are full");

        ProfileCommandResult unequipBasic = profiles.UnequipSkill(player, "top-basic");
        test.Expect(unequipBasic.accepted, "can unequip an active skill");
        test.Expect(!profiles.HasActiveSkill(player, "top-basic"), "unequipped skill is no longer active");

        ProfileCommandResult equipRecover = profiles.EquipSkill(player, "top-recover");
        test.Expect(equipRecover.accepted, "can equip a learned skill after freeing a slot");
        test.Expect(profiles.HasActiveSkill(player, "top-recover"), "equipped skill is active");

        ProfileCommandResult negativeXp = profiles.AwardXp(player, -1);
        test.Expect(!negativeXp.accepted, "negative XP awards are rejected");
        test.ExpectEqual(negativeXp.errorCode, SimulationError::NegativeXpAward, "negative XP reject has a stable error code");

        ProfileCommandResult rankUp = profiles.AwardXp(player, 700);
        test.Expect(rankUp.accepted, "positive XP awards are accepted");
        test.Expect(rankUp.leveledUp, "large XP award reports a level up");
        test.ExpectEqual(player.level, 5, "700 XP carries level 1 exactly to level 5");
        test.ExpectEqual(player.xp, 0, "exact level-up XP leaves no remainder");
        test.ExpectEqual(player.xpRequiredForNextLevel, 300, "level 5 next XP requirement is refreshed");
        test.ExpectEqual(player.rank, CareerRank::Ladder, "level 5 promotes to Ladder");
        test.ExpectEqual(player.passiveBonuses.maxHpBonus, 10, "Ladder grants the HP passive bonus");
    }

    void TestTrainerProfile(TestContext& test)
    {
        SimulationData data;
        PlayerProfileSystem profiles(data);
        TrainerProfile trainer = TrainerProfile::CreateNew(
            "Coach",
            GameType::LeagueOfLegends,
            Spec::Mid,
            profiles);

        const TrainerProfileState& state = trainer.GetState();
        test.ExpectEqual(state.trainerName, std::string("Coach"), "trainer name is stored");
        test.ExpectEqual(state.rating, TrainerBalance::StartingRating, "trainer starts at the default rating");
        test.ExpectEqual(state.money, TrainerBalance::StartingMoney, "trainer starts with no money");
        test.ExpectEqual(state.roster.size(), static_cast<std::size_t>(1), "trainer starts with one player profile");
        test.Expect(trainer.GetActivePlayerProfile() != nullptr, "active player profile is available");
        test.ExpectEqual(trainer.GetActivePlayerLevel(), 1, "active player level starts at 1");
        test.ExpectEqual(trainer.GetActivePlayerProfile()->spec, Spec::Mid, "starter spec is added to the roster");
        test.Expect(trainer.GetMutablePlayerProfile(99) == nullptr, "invalid roster index returns nullptr");

        ProfileCommandResult ratingLoss = trainer.AwardRating(-1200);
        test.Expect(ratingLoss.accepted, "rating awards can be negative");
        test.ExpectEqual(ratingLoss.newValue, 0, "rating cannot drop below zero");

        ProfileCommandResult moneyGain = trainer.AwardMoney(50);
        test.Expect(moneyGain.accepted, "money awards are accepted");
        test.ExpectEqual(moneyGain.newValue, 50, "money gain is added");

        ProfileCommandResult moneyLoss = trainer.AwardMoney(-100);
        test.Expect(moneyLoss.accepted, "money loss is accepted");
        test.ExpectEqual(moneyLoss.newValue, 0, "money cannot drop below zero");

        ProfileCommandResult firstTrophy = trainer.AddTrophy("first-win");
        test.Expect(firstTrophy.accepted, "first trophy insert is accepted");
        ProfileCommandResult duplicateTrophy = trainer.AddTrophy("first-win");
        test.Expect(!duplicateTrophy.accepted, "duplicate trophy insert is rejected");
        test.ExpectEqual(duplicateTrophy.errorCode, SimulationError::TrophyAlreadyEarned, "duplicate trophy reject has a stable error code");

        for (int index = 1; index < TrainerBalance::MaxPlayerProfiles; ++index)
        {
            PlayerProfileState extra = profiles.CreateStarter("Bench " + std::to_string(index), Spec::Support);
            ProfileCommandResult added = trainer.AddPlayerProfile(extra);
            test.Expect(added.accepted, "can add player profile until the roster is full");
        }

        PlayerProfileState overflow = profiles.CreateStarter("Overflow", Spec::Top);
        ProfileCommandResult rejected = trainer.AddPlayerProfile(overflow);
        test.Expect(!rejected.accepted, "cannot add player profile past the roster limit");
        test.ExpectEqual(rejected.errorCode, SimulationError::RosterFull, "roster overflow reject has a stable error code");
        test.ExpectEqual(
            trainer.GetState().roster.size(),
            static_cast<std::size_t>(TrainerBalance::MaxPlayerProfiles),
            "roster stays at the max size");
    }

    void TestRatingSystem(TestContext& test)
    {
        RatingSystem ratings;

        RatingResult normalWin = ratings.CalculateChange(1000, 3, 5, MatchContext::Normal, true);
        test.Expect(normalWin.accepted, "valid win rating calculation is accepted");
        test.ExpectEqual(normalWin.ratingChange, 30, "beating a higher-level opponent grants bonus rating");
        test.ExpectEqual(normalWin.newRating, 1030, "win rating change is applied");

        RatingResult tutorialWin = ratings.CalculateChange(1000, 3, 5, MatchContext::Tutorial, true);
        test.Expect(tutorialWin.accepted, "tutorial rating calculation is accepted");
        test.ExpectEqual(tutorialWin.ratingChange, 15, "tutorial matches halve rating gains");

        RatingResult nemesisLoss = ratings.CalculateChange(1000, 5, 3, MatchContext::Nemesis, false);
        test.Expect(nemesisLoss.accepted, "valid loss rating calculation is accepted");
        test.ExpectEqual(nemesisLoss.ratingChange, -30, "nemesis matches amplify rating losses");
        test.ExpectEqual(nemesisLoss.newRating, 970, "loss rating change is applied");

        RatingResult majorMinimumWin = ratings.CalculateChange(1000, 10, 1, MatchContext::Major, true);
        test.Expect(majorMinimumWin.accepted, "major rating calculation is accepted");
        test.ExpectEqual(majorMinimumWin.ratingChange, 4, "minimum win gain is applied before the major multiplier");

        RatingResult invalidLevels = ratings.CalculateChange(1000, 0, 1, MatchContext::Normal, true);
        test.Expect(!invalidLevels.accepted, "zero player level is rejected");
        test.ExpectEqual(invalidLevels.errorCode, SimulationError::LevelsMustBePositive, "invalid rating inputs have a stable error code");
        test.ExpectEqual(invalidLevels.error, std::string("Levels must be positive."), "invalid level error explains the problem");
    }

    void TestBattleSessionSwitchingAndXpShare(TestContext& test)
    {
        SimulationData data;
        BattleSession session(data, 1234);

        BattleSetup setup;
        setup.gameType = GameType::LeagueOfLegends;
        setup.playerTeam.push_back(MakeBattleSlot(101, "Starter", Spec::Top, "top-basic"));
        setup.playerTeam.push_back(MakeBattleSlot(202, "Closer", Spec::Jungle, "jungle-basic"));
        setup.activePlayerIndex = 0;
        setup.opponentSpec = Spec::Mid;
        setup.opponentStyle = Style::Balanced;

        BattleActionResult start = session.StartBattle(setup);
        test.Expect(start.accepted, "battle starts with a two-player team");
        test.Expect(start.battleStarted, "start result reports battleStarted");
        test.ExpectEqual(start.events.size(), static_cast<std::size_t>(1), "start result has one timeline event");
        test.ExpectEqual(start.events[0].type, BattleEventType::BattleStarted, "start timeline begins with BattleStarted");
        test.Expect(start.finalState.started, "start result carries the final state snapshot");
        test.ExpectEqual(session.GetState().activePlayerIndex, 0, "first player starts active");

        BattleActionResult sameSwitch = session.SwitchPlayer(0);
        test.Expect(!sameSwitch.accepted, "cannot switch to the already-active player");
        test.ExpectEqual(sameSwitch.errorCode, SimulationError::PlayerProfileAlreadyActive, "same-player switch reject has a stable error code");

        BattleActionResult switchResult = session.SwitchPlayer(1);
        test.Expect(switchResult.accepted, "can switch to another available player");
        test.Expect(switchResult.playerSwitched, "switch result reports playerSwitched");
        test.ExpectEqual(switchResult.events[0].type, BattleEventType::PlayerSwitched, "switch timeline starts with PlayerSwitched");
        test.Expect(ContainsEventType(switchResult.events, BattleEventType::SkillStarted), "switch timeline includes opponent response skill");
        test.ExpectEqual(switchResult.oldPlayerIndex, 0, "switch result tracks old active index");
        test.ExpectEqual(switchResult.newPlayerIndex, 1, "switch result tracks new active index");
        test.ExpectEqual(switchResult.newPlayerName, std::string("Closer"), "switch result tracks new player name");
        test.ExpectEqual(session.GetState().activePlayerIndex, 1, "second player is active after switching");

        BattleActionResult finalAction;
        for (int attempt = 0; attempt < 20 && !session.GetState().finished; ++attempt)
        {
            finalAction = session.UsePlayerSkill("jungle-basic");
            test.Expect(finalAction.accepted, "active player can use their basic skill");
            if (!finalAction.accepted)
            {
                break;
            }
        }

        BattleState finalState = session.GetState();
        test.Expect(finalState.finished, "battle finishes after the boosted switched player attacks");
        test.ExpectEqual(finalState.winner, BattleWinner::Player, "player team wins the battle");
        test.ExpectEqual(finalAction.events[0].type, BattleEventType::SkillStarted, "skill timeline starts with the selected skill");
        test.ExpectEqual(finalAction.events[0].actor, BattleActor::Player, "first skill event belongs to the player");
        test.Expect(ContainsEventType(finalAction.events, BattleEventType::DamageApplied), "winning timeline includes damage");
        test.Expect(ContainsEventType(finalAction.events, BattleEventType::SkillXpGained), "winning timeline includes skill XP");
        test.Expect(ContainsEventType(finalAction.events, BattleEventType::BattleFinished), "winning timeline includes battle finish");
        test.Expect(ContainsEventType(finalAction.events, BattleEventType::RewardGranted), "winning timeline includes reward grant");
        test.Expect(finalAction.finalState.finished, "winning result carries the final state snapshot");
        test.Expect(finalAction.reward.awarded, "winning action attaches an XP reward");
        test.ExpectEqual(finalAction.reward.totalXp, 120, "battle win grants the configured total XP");
        test.ExpectEqual(finalAction.reward.xpPerParticipant, 60, "XP is split across both participants");
        test.ExpectEqual(finalAction.reward.participantPlayerIndices.size(), static_cast<std::size_t>(2), "two participants receive XP");
        test.Expect(ContainsInt(finalAction.reward.participantPlayerIndices, 101), "starting player receives shared XP");
        test.Expect(ContainsInt(finalAction.reward.participantPlayerIndices, 202), "switched-in player receives shared XP");
    }

    void TestBattleSessionSeededReplay(TestContext& test)
    {
        SimulationData data;
        BattleSession first(data, 777);
        BattleSession second(data, 777);

        BattleSetup setup;
        setup.gameType = GameType::LeagueOfLegends;
        setup.playerTeam.push_back(MakeBattleSlot(101, "Starter", Spec::Top, "top-basic"));
        setup.activePlayerIndex = 0;
        setup.opponentSpec = Spec::Mid;
        setup.opponentStyle = Style::Balanced;

        first.StartBattle(setup);
        second.StartBattle(setup);

        BattleActionResult firstAction = first.UsePlayerSkill("top-basic");
        BattleActionResult secondAction = second.UsePlayerSkill("top-basic");
        test.Expect(firstAction.accepted && secondAction.accepted, "seeded replay actions are accepted");
        test.ExpectEqual(firstAction.skillUses.size(), secondAction.skillUses.size(), "seeded replay produces the same number of skill uses");
        test.ExpectEqual(firstAction.events.size(), secondAction.events.size(), "seeded replay produces the same number of events");

        for (std::size_t index = 0; index < firstAction.skillUses.size(); ++index)
        {
            test.ExpectEqual(firstAction.skillUses[index].hit, secondAction.skillUses[index].hit, "seeded replay hit results match");
            test.ExpectEqual(firstAction.skillUses[index].damage.amount, secondAction.skillUses[index].damage.amount, "seeded replay damage matches");
        }
    }

    struct TestCase
    {
        const char* name;
        void (*run)(TestContext&);
    };
}

int main()
{
    const std::vector<TestCase> tests = {
        { "PlayerProfileSystem", TestPlayerProfileSystem },
        { "TrainerProfile", TestTrainerProfile },
        { "RatingSystem", TestRatingSystem },
        { "BattleSession switching and XP sharing", TestBattleSessionSwitchingAndXpShare },
        { "BattleSession seeded replay", TestBattleSessionSeededReplay },
    };

    int totalFailures = 0;
    for (const TestCase& testCase : tests)
    {
        std::cout << "Running " << testCase.name << "...\n";
        TestContext context;
        testCase.run(context);

        if (context.failures == 0)
        {
            std::cout << "  PASS\n";
        }

        totalFailures += context.failures;
    }

    if (totalFailures == 0)
    {
        std::cout << "\nAll backend C++ tests passed.\n";
        return 0;
    }

    std::cout << "\n" << totalFailures << " backend C++ test expectation(s) failed.\n";
    return 1;
}
