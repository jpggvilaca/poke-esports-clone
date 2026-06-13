#include "BattleSession.h"
#include "PlayerProfileSystem.h"
#include "RatingSystem.h"
#include "ScoutSystem.h"
#include "SimulationData.h"
#include "TrainerProfile.h"

#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <random>
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
        case SimulationError::UnknownActivePlayerProfile: return "UnknownActivePlayerProfile";
        case SimulationError::BattleNotStarted: return "BattleNotStarted";
        case SimulationError::BattleAlreadyFinished: return "BattleAlreadyFinished";
        case SimulationError::UnknownSkill: return "UnknownSkill";
        case SimulationError::UnknownDrill: return "UnknownDrill";
        case SimulationError::InsufficientMana: return "InsufficientMana";
        case SimulationError::SkillOnCooldown: return "SkillOnCooldown";
        case SimulationError::ActorStunned: return "ActorStunned";
        case SimulationError::ActorSilenced: return "ActorSilenced";
        case SimulationError::ActorRooted: return "ActorRooted";
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
        case SimulationError::InvalidSkillTarget: return "InvalidSkillTarget";
        }

        return "Unknown";
    }

    std::string ToText(BattleEventType value)
    {
        switch (value)
        {
        case BattleEventType::None: return "None";
        case BattleEventType::BattleStarted: return "BattleStarted";
        case BattleEventType::PlayerSwitched: return "PlayerSwitched";
        case BattleEventType::SkillStarted: return "SkillStarted";
        case BattleEventType::DrillStarted: return "DrillStarted";
        case BattleEventType::DrillCompleted: return "DrillCompleted";
        case BattleEventType::ManaChanged: return "ManaChanged";
        case BattleEventType::CooldownStarted: return "CooldownStarted";
        case BattleEventType::CooldownTicked: return "CooldownTicked";
        case BattleEventType::CooldownReady: return "CooldownReady";
        case BattleEventType::ActionBlocked: return "ActionBlocked";
        case BattleEventType::AttackMissed: return "AttackMissed";
        case BattleEventType::DamageApplied: return "DamageApplied";
        case BattleEventType::HealingApplied: return "HealingApplied";
        case BattleEventType::StatusApplied: return "StatusApplied";
        case BattleEventType::StatusExpired: return "StatusExpired";
        case BattleEventType::MarkApplied: return "MarkApplied";
        case BattleEventType::MarkTriggered: return "MarkTriggered";
        case BattleEventType::MarkExpired: return "MarkExpired";
        case BattleEventType::FarmingTriggered: return "FarmingTriggered";
        case BattleEventType::ObjectiveDestroyed: return "ObjectiveDestroyed";
        case BattleEventType::ReinforcementStarted: return "ReinforcementStarted";
        case BattleEventType::NeutralObjectiveSpawned: return "NeutralObjectiveSpawned";
        case BattleEventType::NeutralObjectiveSlain: return "NeutralObjectiveSlain";
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

    Competitor MakeCompetitor(Spec spec, const std::string& traitId)
    {
        Competitor competitor;
        competitor.spec = spec;
        competitor.traitId = traitId;
        competitor.hp = 100;
        competitor.maxHp = 100;
        competitor.mana = 0;
        competitor.maxMana = Balance::StartingMaxMana;
        competitor.basePower = Balance::StartingBasePower;
        return competitor;
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
        slot.passiveBonuses.maxHpBonus = 10000;
        slot.passiveBonuses.basePowerBonus = 500;
        slot.skills.push_back({ basicSkillId, 80, 0 });
        return slot;
    }

    BattleSetup::PlayerSlot MakeAbilityBattleSlot(
        int profileIndex,
        const std::string& name,
        Spec spec,
        std::initializer_list<const char*> skillIds,
        int currentMana = -1)
    {
        BattleSetup::PlayerSlot slot;
        slot.profileIndex = profileIndex;
        slot.name = name;
        slot.spec = spec;
        slot.passiveBonuses.maxHpBonus = 1000;
        slot.currentMana = currentMana;
        slot.maxMana = Balance::StartingMaxMana;
        for (const char* skillId : skillIds)
        {
            slot.skills.push_back({ skillId, 1, 0 });
        }
        return slot;
    }

    BattleSetup::OpponentSlot MakeOpponentSlot(
        const std::string& name,
        Spec spec,
        int hp,
        int basePowerBonus = 0)
    {
        BattleSetup::OpponentSlot slot;
        slot.name = name;
        slot.spec = spec;
        slot.maxHp = hp;
        slot.currentHp = hp;
        slot.maxMana = Balance::StartingMaxMana;
        slot.basePowerBonus = basePowerBonus;
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
        test.ExpectEqual(player.learnedSkillIds.size(), static_cast<std::size_t>(4), "starter learns the full spec kit");
        test.ExpectEqual(player.activeSkillIds.size(), static_cast<std::size_t>(4), "starter equips the full spec kit");
        test.ExpectEqual(player.learnedSkillIds[0], std::string("top-basic"), "starter loadout begins with the spec basic");
        test.ExpectEqual(player.activeSkillIds[0], std::string("top-basic"), "starter active skill is the spec basic");
        test.ExpectEqual(player.activeSkillIds[1], std::string("top-sunder"), "starter second skill is the mana damage skill");
        test.ExpectEqual(player.activeSkillIds[2], std::string("top-hold-line"), "starter third skill is the effect skill");
        test.ExpectEqual(player.activeSkillIds[3], std::string("top-gamebreaker"), "starter fourth skill is the ultimate");
        test.ExpectEqual(player.traitId, std::string("clutch-player"), "top starter receives the top default trait");

        test.ExpectEqual(
            profiles.CreateStarter("Rookie Jungle", Spec::Jungle).traitId,
            std::string("shotcaller"),
            "jungle starter receives the jungle default trait");
        test.ExpectEqual(
            profiles.CreateStarter("Rookie Mid", Spec::Mid).traitId,
            std::string("lane-bully"),
            "mid starter receives the mid default trait");
        test.ExpectEqual(
            profiles.CreateStarter("Rookie ADC", Spec::Adc).traitId,
            std::string("precision-carry"),
            "ADC starter receives the ADC default trait");
        test.ExpectEqual(
            profiles.CreateStarter("Rookie Support", Spec::Support).traitId,
            std::string("stabilizer"),
            "support starter receives the support default trait");

        ProfileCommandResult equipUnknown = profiles.EquipSkill(player, "support-basic");
        test.Expect(!equipUnknown.accepted, "cannot equip a skill before learning it");
        test.ExpectEqual(equipUnknown.errorCode, SimulationError::SkillNotLearned, "equip reject has a stable error code");

        ProfileCommandResult learnMissing = profiles.LearnSkill(player, "missing-skill");
        test.Expect(!learnMissing.accepted, "cannot learn an unknown skill");
        test.ExpectEqual(learnMissing.errorCode, SimulationError::UnknownSkill, "unknown learn reject has a stable error code");

        ProfileCommandResult equipMissing = profiles.EquipSkill(player, "missing-skill");
        test.Expect(!equipMissing.accepted, "cannot equip an unknown skill");
        test.ExpectEqual(equipMissing.errorCode, SimulationError::UnknownSkill, "unknown equip reject has a stable error code");

        ProfileCommandResult learnRecover = profiles.LearnSkill(player, "jungle-gank");
        test.Expect(learnRecover.accepted, "can learn a new skill");
        test.Expect(profiles.HasLearnedSkill(player, "jungle-gank"), "learned skill is tracked");

        ProfileCommandResult duplicateLearn = profiles.LearnSkill(player, "jungle-gank");
        test.Expect(!duplicateLearn.accepted, "cannot learn the same skill twice");

        ProfileCommandResult equipWhenFull = profiles.EquipSkill(player, "jungle-gank");
        test.Expect(!equipWhenFull.accepted, "cannot equip when all active slots are full");

        ProfileCommandResult unequipBasic = profiles.UnequipSkill(player, "top-basic");
        test.Expect(unequipBasic.accepted, "can unequip an active skill");
        test.Expect(!profiles.HasActiveSkill(player, "top-basic"), "unequipped skill is no longer active");

        ProfileCommandResult equipRecover = profiles.EquipSkill(player, "jungle-gank");
        test.Expect(equipRecover.accepted, "can equip a learned skill after freeing a slot");
        test.Expect(profiles.HasActiveSkill(player, "jungle-gank"), "equipped skill is active");

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
        test.ExpectEqual(player.passiveBonuses.maxHpBonus, 26, "level 5 combines top HP growth with Ladder HP bonus");
        test.ExpectEqual(player.passiveBonuses.basePowerBonus, 0, "level 5 applies top base power growth");
    }

    void TestSimulationDataReferences(TestContext& test)
    {
        SimulationData data;
        test.Expect(data.LoadedExternalSkills(), "external skills CSV is loaded from the project data folder");
        test.Expect(data.LoadedExternalTraits(), "external traits CSV is loaded from the project data folder");
        test.Expect(data.LoadedExternalSpecs(), "external specs CSV is loaded from the project data folder");
        test.Expect(data.LoadedExternalDrills(), "external drills CSV is loaded from the project data folder");

        for (const SpecData& spec : data.GetSpecs())
        {
            test.ExpectEqual(
                spec.skillIds.size(),
                static_cast<std::size_t>(Balance::StarterSkillsPerSpec),
                ToString(spec.spec) + " has the expected starter skill count");
            test.Expect(
                data.FindTrait(spec.defaultTraitId) != nullptr,
                ToString(spec.spec) + " default trait exists");

            for (const std::string& skillId : spec.skillIds)
            {
                test.Expect(
                    data.FindSkill(skillId) != nullptr,
                ToString(spec.spec) + " starter skill exists: " + skillId);
            }
        }

        for (const SpecData& spec : data.GetSpecs())
        {
            for (const std::string& skillId : spec.skillIds)
            {
                const Skill* skill = data.FindSkill(skillId);
                if (skill == nullptr || skill->effectType == SkillEffectType::None)
                {
                    continue;
                }

                test.Expect(!skill->effects.empty(), skillId + " exposes composable effects");
                if (!skill->effects.empty())
                {
                    test.Expect(skill->effects[0].type == skill->effectType, skillId + " primary effect type is mirrored");
                    test.Expect(skill->effects[0].target == skill->effectTarget, skillId + " primary effect target is mirrored");
                    test.ExpectEqual(skill->effects[0].value, skill->effectValue, skillId + " primary effect value is mirrored");
                    test.ExpectEqual(skill->effects[0].durationTurns, skill->durationTurns, skillId + " primary effect duration is mirrored");
                    test.ExpectEqual(skill->effects[0].markBonusDamage, skill->markBonusDamage, skillId + " primary mark bonus is mirrored");
                }
            }
        }

        for (const SpecData& spec : data.GetSpecs())
        {
            for (const std::string& skillId : spec.skillIds)
            {
                const Skill* skill = data.FindSkill(skillId);
                if (skill == nullptr)
                {
                    continue;
                }

                test.Expect(!skill->sourceSpec.empty(), skillId + " exposes source spec metadata");
                test.Expect(!skill->colorId.empty(), skillId + " exposes skill color metadata");
                if (skillId.find("-basic") != std::string::npos)
                {
                    test.ExpectEqual(skill->colorId, std::string("neutral"), skillId + " basic skill uses neutral color");
                }
                else
                {
                    test.Expect(skill->colorId != "neutral", skillId + " non-basic skill uses spec color");
                }
            }
        }
    }

    void TestTraitBattleRules(TestContext& test)
    {
        SimulationData data;
        BattleRules rules(data);

        const Skill* topPressure = data.FindSkill("top-sunder");
        const Skill* midBasic = data.FindSkill("mid-basic");
        const Skill* adcAllIn = data.FindSkill("adc-bullet-time");
        const Skill* jungleDisrupt = data.FindSkill("jungle-gank");
        const Skill* supportRecover = data.FindSkill("support-peel");
        test.Expect(topPressure != nullptr, "top pressure skill exists");
        test.Expect(midBasic != nullptr, "mid basic skill exists");
        test.Expect(adcAllIn != nullptr, "ADC all-in skill exists");
        test.Expect(jungleDisrupt != nullptr, "jungle disrupt skill exists");
        test.Expect(supportRecover != nullptr, "support recover skill exists");

        if (topPressure == nullptr
            || midBasic == nullptr
            || adcAllIn == nullptr
            || jungleDisrupt == nullptr
            || supportRecover == nullptr)
        {
            return;
        }

        SkillProgress progress{ topPressure->id, 1, 0 };
        Competitor clutch = MakeCompetitor(Spec::Top, "clutch-player");
        clutch.hp = 35;
        test.ExpectEqual(rules.GetManaCost(*topPressure, progress, clutch), 35, "clutch does not trigger at 35 percent HP");
        clutch.hp = 34;
        test.ExpectEqual(rules.GetManaCost(*topPressure, progress, clutch), 26, "clutch discounts paid abilities below 35 percent HP");

        Competitor mid = MakeCompetitor(Spec::Mid, "lane-bully");
        Competitor adc = MakeCompetitor(Spec::Adc, "precision-carry");
        BattleStatus midStatus;
        BattleStatus adcStatus;
        DamageResult bullyDamage = rules.CalculateDamage(*midBasic, { midBasic->id, 1, 0 }, mid, midStatus, adc, adcStatus);
        mid.traitId = "";
        DamageResult normalDamage = rules.CalculateDamage(*midBasic, { midBasic->id, 1, 0 }, mid, midStatus, adc, adcStatus);
        test.Expect(bullyDamage.amount > normalDamage.amount, "lane bully increases super-effective damage");

        const double precisionAccuracy = rules.GetAccuracy(*adcAllIn, { adcAllIn->id, 1, 0 }, adc);
        test.Expect(
            precisionAccuracy > 0.92 && precisionAccuracy < 0.94,
            "precision carry improves damaging accuracy");

        Competitor jungle = MakeCompetitor(Spec::Jungle, "shotcaller");
        test.ExpectEqual(
            rules.GetEffectValue(*jungleDisrupt, { jungleDisrupt->id, 1, 0 }, jungle),
            0,
            "jungle effect skill applies mark setup");

        Competitor support = MakeCompetitor(Spec::Support, "stabilizer");
        test.ExpectEqual(
            rules.GetEffectValue(*supportRecover, { supportRecover->id, 1, 0 }, support),
            48,
            "stabilizer strengthens healing effects");
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
        test.ExpectEqual(trainer.GetActivePlayerProfile()->traitId, std::string("lane-bully"), "trainer starter carries its spec trait");
        test.Expect(trainer.GetMutablePlayerProfile(99) == nullptr, "invalid roster index returns nullptr");

        PlayerProfileState bench = profiles.CreateStarter("Bench", Spec::Support);
        ProfileCommandResult addedBench = trainer.AddPlayerProfile(bench);
        test.Expect(addedBench.accepted, "can add a bench player before switching active profile");
        ProfileCommandResult activeSwitch = trainer.SetActivePlayerIndex(1);
        test.Expect(activeSwitch.accepted, "can set the active player profile");
        test.ExpectEqual(activeSwitch.oldValue, 0, "active switch reports old index");
        test.ExpectEqual(activeSwitch.newValue, 1, "active switch reports new index");
        test.ExpectEqual(trainer.GetActivePlayerProfile()->spec, Spec::Support, "active profile follows the selected index");
        ProfileCommandResult invalidSwitch = trainer.SetActivePlayerIndex(99);
        test.Expect(!invalidSwitch.accepted, "cannot set active profile to an invalid index");
        test.ExpectEqual(invalidSwitch.errorCode, SimulationError::UnknownPlayerProfile, "invalid active profile switch has a stable error code");
        test.ExpectEqual(trainer.GetState().activePlayerIndex, 1, "invalid active profile switch leaves active index unchanged");

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

        for (int index = 2; index < TrainerBalance::MaxPlayerProfiles; ++index)
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

    void TestScoutSystem(TestContext& test)
    {
        SimulationData data;
        ScoutSystem scouts(data);

        ScoutOfferView locked = scouts.GetNextOffer(1049, {}, {});
        test.Expect(!locked.available, "no scout offer appears below the first rating threshold");

        ScoutOfferView first = scouts.GetNextOffer(1050, {}, {});
        test.Expect(first.available, "first scout offer appears at the first rating threshold");
        test.ExpectEqual(first.id, std::string("first-scout"), "first scout offer id is stable");
        test.ExpectEqual(first.requiredRating, 1050, "first scout offer keeps its rating threshold");
        test.ExpectEqual(first.candidates.size(), static_cast<std::size_t>(3), "first scout offer has three candidates");
        test.ExpectEqual(first.candidates[0].name, std::string("Mira"), "first scout candidate name is hydrated");
        test.ExpectEqual(first.candidates[0].spec, Spec::Support, "first scout candidate spec is hydrated");
        test.ExpectEqual(first.candidates[0].activeSkillIds.size(), static_cast<std::size_t>(4), "scout candidate receives starter skills");

        ScoutOfferView third = scouts.GetNextOffer(1200, { "first-scout" }, { "second-scout" });
        test.Expect(third.available, "completed and declined offers are skipped");
        test.ExpectEqual(third.id, std::string("third-scout"), "third offer is next after skipping earlier offers");

        ProfileCommandResult validRecruit = scouts.CanRecruitCandidate(5, TrainerBalance::MaxPlayerProfiles, 2, first);
        test.Expect(validRecruit.accepted, "can recruit a valid candidate when roster has room");

        ProfileCommandResult rosterFull = scouts.CanRecruitCandidate(6, TrainerBalance::MaxPlayerProfiles, 0, first);
        test.Expect(!rosterFull.accepted, "cannot recruit when roster is full");
        test.ExpectEqual(rosterFull.errorCode, SimulationError::RosterFull, "full roster scout reject has a stable error code");

        ProfileCommandResult badCandidate = scouts.CanRecruitCandidate(5, TrainerBalance::MaxPlayerProfiles, 99, first);
        test.Expect(!badCandidate.accepted, "cannot recruit an unknown scout candidate");
        test.ExpectEqual(badCandidate.errorCode, SimulationError::UnknownPlayerProfile, "bad scout candidate reject has a stable error code");

        ProfileCommandResult noOffer = scouts.CanRecruitCandidate(5, TrainerBalance::MaxPlayerProfiles, 0, {});
        test.Expect(!noOffer.accepted, "cannot recruit without a pending scout offer");
        test.ExpectEqual(noOffer.errorCode, SimulationError::UnknownPlayerProfile, "missing scout offer reject has a stable error code");
    }

    void TestBattleSessionSwitchingAndXpShare(TestContext& test)
    {
        SimulationData data;
        BattleSession session(data, 1234);

        BattleSetup setup;
        setup.gameType = GameType::LeagueOfLegends;
        BattleSetup::PlayerSlot starter = MakeBattleSlot(101, "Starter", Spec::Top, "top-basic");
        starter.passiveBonuses.basePowerBonus = 0;
        starter.skills[0].level = 1;
        BattleSetup::PlayerSlot closer = MakeBattleSlot(202, "Closer", Spec::Jungle, "jungle-basic");
        setup.playerTeam.push_back(starter);
        setup.playerTeam.push_back(closer);
        setup.activePlayerIndex = 0;
        setup.opponentSpec = Spec::Mid;
        setup.opponentMaxHp = 350;

        BattleActionResult start = session.StartBattle(setup);
        test.Expect(start.accepted, "battle starts with a two-player team");
        test.Expect(start.battleStarted, "start result reports battleStarted");
        test.ExpectEqual(start.events.size(), static_cast<std::size_t>(1), "start result has one timeline event");
        test.ExpectEqual(start.events[0].type, BattleEventType::BattleStarted, "start timeline begins with BattleStarted");
        test.Expect(start.finalState.started, "start result carries the final state snapshot");
        test.ExpectEqual(session.GetState().activePlayerIndex, 0, "first player starts active");

        BattleActionResult manualSwitch = session.SwitchPlayer(1);
        test.Expect(!manualSwitch.accepted, "manual switching is rejected while party order is fixed");
        test.ExpectEqual(manualSwitch.errorCode, SimulationError::PlayerProfileCannotPlay, "manual switch reject has a stable error code");

        BattleActionResult firstAction = session.UsePlayerSkill("top-basic");
        test.Expect(firstAction.accepted, "starting player can use their basic skill");
        test.Expect(firstAction.playerSwitched, "turn flow advances to the next living player");
        test.Expect(ContainsEventType(firstAction.events, BattleEventType::PlayerSwitched), "turn flow emits PlayerSwitched");
        test.ExpectEqual(firstAction.oldPlayerIndex, 0, "turn advance tracks old active index");
        test.ExpectEqual(firstAction.newPlayerIndex, 1, "turn advance tracks new active index");
        test.ExpectEqual(firstAction.newPlayerName, std::string("Closer"), "turn advance tracks new player name");
        test.ExpectEqual(session.GetState().activePlayerIndex, 1, "second player is active after automatic turn advance");

        auto activeBasicSkill = [&session]() {
            return session.GetState().player.name == "Closer"
                ? std::string("jungle-basic")
                : std::string("top-basic");
        };
        auto hasLivingOpponent = [&session]() {
            for (const CompetitorView& opponent : session.GetState().opponentTeam)
            {
                if (opponent.hp > 0)
                {
                    return true;
                }
            }

            return false;
        };

        BattleActionResult finalAction;
        for (int attempt = 0; attempt < 20 && hasLivingOpponent(); ++attempt)
        {
            finalAction = session.UsePlayerSkill(activeBasicSkill());
            test.Expect(finalAction.accepted, "active player can use their basic skill");
            if (!finalAction.accepted)
            {
                break;
            }
        }

        test.Expect(!session.GetState().finished, "defeating the opponent opens objectives instead of ending the battle");
        test.Expect(session.GetState().opponentObjectiveVulnerable, "enemy objective becomes vulnerable after the opponent lineup is down");
        BattleActionResult blockedChampionHit = session.UsePlayerSkill(activeBasicSkill());
        test.Expect(!blockedChampionHit.accepted, "champion attacks stop once no living opponent remains");
        test.ExpectEqual(blockedChampionHit.errorCode, SimulationError::InvalidSkillTarget, "dead champion targeting rejects with invalid target");

        for (int attempt = 0; attempt < 120 && !session.GetState().finished; ++attempt)
        {
            if (hasLivingOpponent())
            {
                finalAction = session.UsePlayerSkill(activeBasicSkill());
                test.Expect(finalAction.accepted, "active player can fight a respawned opponent");
            }
            else
            {
                finalAction = session.UsePlayerPushObjective();
                test.Expect(finalAction.accepted, "active player can push while the enemy team is down");
            }
            if (!finalAction.accepted)
            {
                break;
            }
        }

        BattleState finalState = session.GetState();
        test.Expect(finalState.finished, "battle finishes after the player destroys the enemy Nexus");
        test.ExpectEqual(finalState.winner, BattleWinner::Player, "player team wins the battle");
        test.ExpectEqual(finalState.opponentObjectives[2].hp, 0, "enemy Nexus is destroyed");
        test.Expect(!finalAction.events.empty(), "winning action includes timeline events");
        if (!finalAction.events.empty())
        {
            test.ExpectEqual(finalAction.events[0].type, BattleEventType::SkillStarted, "skill timeline starts with the selected skill");
            test.ExpectEqual(finalAction.events[0].actor, BattleActor::Player, "first skill event belongs to the player");
        }
        test.Expect(ContainsEventType(finalAction.events, BattleEventType::DamageApplied), "winning timeline includes damage");
        test.Expect(!ContainsEventType(finalAction.events, BattleEventType::SkillXpGained), "winning timeline respects the per-skill XP cap");
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

    void TestBattleSessionOpponentTeam3v3(TestContext& test)
    {
        SimulationData data;
        BattleSession session(data, 2026);

        BattleSetup setup;
        setup.gameType = GameType::LeagueOfLegends;
        for (int index = 0; index < 3; ++index)
        {
            BattleSetup::PlayerSlot slot = MakeBattleSlot(300 + index, "Player " + std::to_string(index + 1), Spec::Top, "adc-basic");
            slot.currentMana = 100;
            slot.skills[0].level = 80;
            setup.playerTeam.push_back(slot);
        }
        setup.activePlayerIndex = 0;
        setup.opponentTeam.push_back(MakeOpponentSlot("Rival Top", Spec::Top, 80));
        setup.opponentTeam.push_back(MakeOpponentSlot("Rival Jungle", Spec::Jungle, 80));
        setup.opponentTeam.push_back(MakeOpponentSlot("Rival Support", Spec::Support, 80));

        BattleActionResult start = session.StartBattle(setup);
        test.Expect(start.accepted, "3v3 battle starts");
        BattleState startState = session.GetState();
        test.ExpectEqual(startState.playerTeam.size(), static_cast<std::size_t>(3), "3v3 state exposes three player slots");
        test.ExpectEqual(startState.opponentTeam.size(), static_cast<std::size_t>(3), "3v3 state exposes three opponent slots");
        test.ExpectEqual(startState.activeOpponentIndex, 0, "first rival member starts active");

        BattleActionResult firstKo = session.UsePlayerSkill("adc-basic");
        test.Expect(firstKo.accepted, "player can attack the first rival member");
        test.Expect(!firstKo.battleFinished, "defeating one rival member does not finish a 3v3 battle");
        BattleState afterFirstKo = session.GetState();
        test.ExpectEqual(afterFirstKo.activeOpponentIndex, 1, "battle advances to the second rival member");
        test.ExpectEqual(afterFirstKo.opponent.name, std::string("Rival Jungle"), "active opponent snapshot follows the second rival member");

        BattleActionResult finalAction = firstKo;
        auto hasLivingOpponent = [&session]() {
            for (const CompetitorView& opponent : session.GetState().opponentTeam)
            {
                if (opponent.hp > 0)
                {
                    return true;
                }
            }

            return false;
        };
        for (int attempt = 0; attempt < 20 && hasLivingOpponent(); ++attempt)
        {
            finalAction = session.UsePlayerSkill("adc-basic");
            test.Expect(finalAction.accepted, "next player can keep fighting the rival lineup");
            if (!finalAction.accepted)
            {
                break;
            }
        }

        test.Expect(!session.GetState().finished, "3v3 battle continues into the fight-push loop");
        for (int attempt = 0; attempt < 160 && !session.GetState().finished; ++attempt)
        {
            if (hasLivingOpponent())
            {
                finalAction = session.UsePlayerSkill("adc-basic");
                test.Expect(finalAction.accepted, "next player can fight a respawned rival");
            }
            else
            {
                finalAction = session.UsePlayerPushObjective();
                test.Expect(finalAction.accepted, "next player can push while rivals are down");
            }
            if (!finalAction.accepted)
            {
                break;
            }
        }

        BattleState finalState = session.GetState();
        test.Expect(finalState.finished, "3v3 battle finishes after the enemy Nexus falls");
        test.ExpectEqual(finalState.winner, BattleWinner::Player, "player team wins the 3v3 test battle");
        test.ExpectEqual(finalState.opponentTeam.size(), static_cast<std::size_t>(3), "final state keeps all rival members for UI display");
        test.ExpectEqual(finalState.opponentObjectives[2].hp, 0, "3v3 enemy Nexus is destroyed");
        test.Expect(finalAction.reward.awarded, "3v3 win grants the battle reward");
    }

    void TestBattleSessionPlayerKoRequiresObjectiveFinish(TestContext& test)
    {
        SimulationData data;
        BattleSession session(data, 3030);

        BattleSetup setup;
        setup.gameType = GameType::LeagueOfLegends;
        BattleSetup::PlayerSlot player;
        player.profileIndex = 404;
        player.name = "Fragile Top";
        player.spec = Spec::Top;
        player.currentHp = 1;
        player.skills.push_back({ "top-basic", 1, 0 });
        setup.playerTeam.push_back(player);

        BattleSetup::OpponentSlot opponent = MakeOpponentSlot("Fed ADC", Spec::Adc, 1000);
        opponent.skills.push_back({ "adc-basic", 80, 0 });
        setup.opponentTeam.push_back(opponent);

        test.Expect(session.StartBattle(setup).accepted, "KO objective battle starts");
        BattleActionResult result = session.PassPlayerTurn();
        test.Expect(result.accepted, "player can pass into the opponent turn");
        test.Expect(ContainsEventType(result.events, BattleEventType::DamageApplied), "opponent first KOs the player");
        test.Expect(ContainsEventType(result.events, BattleEventType::ReinforcementStarted), "KO starts the player respawn timer");
        test.Expect(!ContainsEventType(result.events, BattleEventType::ObjectiveDestroyed), "opponent does not immediately push after the KO");
        test.Expect(!session.GetState().finished, "KO does not finish the battle");
        test.ExpectEqual(session.GetState().player.hp, 0, "player champion is KO'd");
        test.ExpectEqual(session.GetState().player.reinforcementTimer, 2, "early KO starts a two-turn respawn timer");

        BattleActionResult firstWait = session.PassPlayerTurn();
        test.Expect(firstWait.accepted, "KO'd player can wait one turn");
        test.ExpectEqual(session.GetState().player.reinforcementTimer, 1, "waiting ticks the respawn timer");
        test.Expect(!session.GetState().finished, "one wait turn does not finish the battle");

        BattleActionResult secondWait = session.PassPlayerTurn();
        test.Expect(secondWait.accepted, "KO'd player can wait a second turn");
        test.Expect(ContainsEventType(secondWait.events, BattleEventType::HealingApplied), "player respawns after the timer expires");

        BattleState finalState = session.GetState();
        test.Expect(!finalState.finished, "respawn happens before the Nexus falls");
        test.ExpectEqual(finalState.player.hp, finalState.player.maxHp, "player respawns at full HP");
        test.ExpectEqual(finalState.player.reinforcementTimer, 0, "respawn clears the timer");
        test.Expect(finalState.playerObjectives[2].hp > 0, "player Nexus survives the death timer window");
    }

    void TestBattleSessionOpponentKoRespawns(TestContext& test)
    {
        SimulationData data;
        BattleSession session(data, 5050);

        BattleSetup setup;
        setup.gameType = GameType::LeagueOfLegends;
        BattleSetup::PlayerSlot player = MakeAbilityBattleSlot(
            0,
            "Enemy Respawn Tester",
            Spec::Top,
            { "top-basic" });
        player.passiveBonuses.basePowerBonus = 1000;
        player.passiveBonuses.maxHpBonus = 10000;
        player.skills[0].level = 80;
        setup.playerTeam.push_back(player);
        setup.opponentSpec = Spec::Support;
        setup.opponentMaxHp = 120;

        test.Expect(session.StartBattle(setup).accepted, "enemy respawn battle starts");
        BattleActionResult ko = session.UsePlayerSkill("top-basic");
        test.Expect(ko.accepted, "player can KO the enemy");
        test.Expect(ContainsEventType(ko.events, BattleEventType::ReinforcementStarted), "enemy KO starts a respawn timer");
        test.ExpectEqual(session.GetState().opponent.hp, 0, "enemy is KO'd");
        test.ExpectEqual(session.GetState().opponent.reinforcementTimer, 2, "enemy gets the same two-turn respawn timer");

        BattleActionResult firstWait = session.UsePlayerFarm();
        test.Expect(firstWait.accepted, "player can act while enemy is down");
        test.ExpectEqual(session.GetState().opponent.reinforcementTimer, 1, "enemy respawn timer ticks");

        BattleActionResult secondWait = session.UsePlayerFarm();
        test.Expect(secondWait.accepted, "player can take the second wait action");
        test.Expect(ContainsEventType(secondWait.events, BattleEventType::HealingApplied), "enemy respawns after the timer expires");
        test.ExpectEqual(session.GetState().opponent.hp, session.GetState().opponent.maxHp, "enemy respawns at full HP");
        test.Expect(!session.GetState().opponentObjectiveVulnerable, "enemy respawn restores the objective shield");
    }

    void TestBattleSessionDragonObjective(TestContext& test)
    {
        SimulationData data;
        BattleSession session(data, 6060);

        BattleSetup setup;
        setup.gameType = GameType::LeagueOfLegends;
        BattleSetup::PlayerSlot player = MakeAbilityBattleSlot(
            0,
            "Dragon Tester",
            Spec::Top,
            { "top-basic" });
        player.passiveBonuses.basePowerBonus = 1000;
        player.passiveBonuses.maxHpBonus = 10000;
        player.skills[0].level = 80;
        setup.playerTeam.push_back(player);
        setup.opponentSpec = Spec::Support;
        setup.opponentMaxHp = 1000;

        test.Expect(session.StartBattle(setup).accepted, "dragon battle starts");
        test.Expect(!session.GetState().neutralObjective.active, "dragon starts off the map");
        test.ExpectEqual(session.GetState().neutralObjective.respawnTimer, 6, "dragon starts with a six-turn spawn timer");

        BattleActionResult spawnAction;
        for (int turn = 1; turn <= 6; ++turn)
        {
            spawnAction = session.UsePlayerFarm();
            test.Expect(spawnAction.accepted, "dragon setup action is accepted");
            if (turn < 6)
            {
                test.Expect(!session.GetState().neutralObjective.active, "dragon stays hidden before turn six");
            }
        }
        test.Expect(ContainsEventType(spawnAction.events, BattleEventType::NeutralObjectiveSpawned), "sixth action spawns dragon");
        test.Expect(session.GetState().neutralObjective.active, "dragon is active after spawn");
        test.ExpectEqual(session.GetState().neutralObjective.hp, session.GetState().neutralObjective.maxHp, "dragon spawns at full HP");

        BattleActionResult dragonAction = session.UsePlayerAttackDragon();
        test.Expect(dragonAction.accepted, "player can attack dragon");
        test.Expect(ContainsEventType(dragonAction.events, BattleEventType::DamageApplied), "dragon attack deals neutral damage");
        test.Expect(ContainsEventType(dragonAction.events, BattleEventType::NeutralObjectiveSlain), "dragon can be slain");

        BattleState finalState = session.GetState();
        test.Expect(!finalState.neutralObjective.active, "dragon leaves the map after being slain");
        test.ExpectEqual(finalState.neutralObjective.respawnTimer, 4, "dragon starts a four-turn respawn timer after being slain");
        test.ExpectEqual(finalState.playerPowerBuffPercent, 15, "slaying dragon grants the player team power buff");
        test.ExpectEqual(finalState.player.powerBuffPercent, 15, "active player view exposes the dragon buff");
    }

    void TestManaCooldownAndControlMechanics(TestContext& test)
    {
        SimulationData data;

        {
            BattleSession session(data, 222);
            BattleSetup setup;
            setup.gameType = GameType::LeagueOfLegends;
            setup.playerTeam.push_back(MakeAbilityBattleSlot(
                0,
                "Mana Tester",
                Spec::Top,
                { "top-basic", "top-sunder" }));
            setup.opponentSpec = Spec::Support;
            setup.opponentMaxHp = 1000;

            BattleActionResult start = session.StartBattle(setup);
            test.Expect(start.accepted, "mana battle starts");
            test.ExpectEqual(session.GetState().player.mana, Balance::StartingMana, "battle mana starts with the configured opening mana");

            BattleActionResult rejected = session.UsePlayerSkill("top-sunder");
            test.Expect(!rejected.accepted, "paid ability rejects without mana");
            test.ExpectEqual(rejected.errorCode, SimulationError::InsufficientMana, "paid reject reports insufficient mana");
            test.Expect(ContainsEventType(rejected.events, BattleEventType::ActionBlocked), "paid reject emits action blocked");

            BattleActionResult basic = session.UsePlayerSkill("top-basic");
            test.Expect(basic.accepted, "basic is legal at zero mana");
            test.Expect(ContainsEventType(basic.events, BattleEventType::ManaChanged), "basic generates mana");
            test.ExpectEqual(session.GetState().player.mana, Balance::StartingMana + Balance::BasicManaGain, "basic grants configured mana");
        }

        {
            BattleSession session(data, 223);
            BattleSetup setup;
            setup.gameType = GameType::LeagueOfLegends;
            setup.playerTeam.push_back(MakeAbilityBattleSlot(
                0,
                "Farm Tester",
                Spec::Top,
                { "top-basic" }));
            setup.opponentSpec = Spec::Support;
            setup.opponentMaxHp = 1000;

            test.Expect(session.StartBattle(setup).accepted, "farm battle starts");
            BattleActionResult farm = session.UsePlayerFarm();
            test.Expect(farm.accepted, "farm action is accepted");
            test.Expect(ContainsEventType(farm.events, BattleEventType::FarmingTriggered), "farm emits a farming event");
            test.Expect(ContainsEventType(farm.events, BattleEventType::ManaChanged), "farm grants mana");
            test.Expect(!ContainsEventType(farm.events, BattleEventType::StatusApplied), "farm does not apply defense");
            test.ExpectEqual(
                session.GetState().player.mana,
                Balance::StartingMana + BattleEconomySystem::FarmingManaGain,
                "farm grants the configured mana");
            test.ExpectEqual(
                session.GetState().player.status.defenseModifierPercent,
                0,
                "farm leaves defense unchanged");
            test.ExpectEqual(
                session.GetState().player.status.defenseModifierTurns,
                0,
                "farm does not create a defense duration");
        }

        {
            BattleSession session(data, 333);
            BattleSetup setup;
            setup.gameType = GameType::LeagueOfLegends;
            setup.playerTeam.push_back(MakeAbilityBattleSlot(
                0,
                "Cooldown Tester",
                Spec::Top,
                { "top-basic", "top-sunder" },
                100));
            setup.opponentSpec = Spec::Support;
            setup.opponentMaxHp = 1000;

            test.Expect(session.StartBattle(setup).accepted, "cooldown battle starts");
            BattleActionResult sunder = session.UsePlayerSkill("top-sunder");
            test.Expect(sunder.accepted, "paid ability can be used with mana");
            test.Expect(ContainsEventType(sunder.events, BattleEventType::CooldownStarted), "paid ability starts cooldown");

            BattleActionResult blocked = session.UsePlayerSkill("top-sunder");
            test.Expect(!blocked.accepted, "cooldown blocks immediate reuse");
            test.ExpectEqual(blocked.errorCode, SimulationError::SkillOnCooldown, "cooldown reject reports stable error");

            test.Expect(session.UsePlayerSkill("top-basic").accepted, "first basic ticks cooldown");
            test.ExpectEqual(session.GetAvailablePlayerSkills()[1].cooldownRemaining, 1, "cooldown ticks after owner action");
            test.Expect(session.UsePlayerSkill("top-basic").accepted, "second basic ticks cooldown");
            test.ExpectEqual(session.GetAvailablePlayerSkills()[1].cooldownRemaining, 0, "cooldown reaches ready");
        }

        {
            BattleSession session(data, 444);
            BattleSetup setup;
            setup.gameType = GameType::LeagueOfLegends;
            setup.playerTeam.push_back(MakeAbilityBattleSlot(
                0,
                "ADC",
                Spec::Adc,
                { "adc-basic", "adc-trap" },
                100));
            setup.opponentSpec = Spec::Support;
            setup.opponentMaxHp = 1000;

            test.Expect(session.StartBattle(setup).accepted, "control battle starts");
            BattleActionResult trap = session.UsePlayerSkill("adc-trap");
            test.Expect(trap.accepted, "trap ability is accepted");
            test.Expect(ContainsEventType(trap.events, BattleEventType::StatusApplied), "trap applies stun status");
            test.Expect(ContainsEventType(trap.events, BattleEventType::ActionBlocked), "stunned opponent skips action");
        }
    }

    void TestMarksBuffsDebuffsAndOpponentAI(TestContext& test)
    {
        SimulationData data;

        {
            BattleSession session(data, 555);
            BattleSetup setup;
            setup.gameType = GameType::LeagueOfLegends;
            setup.playerTeam.push_back(MakeAbilityBattleSlot(
                0,
                "Jungle",
                Spec::Jungle,
                { "jungle-basic", "jungle-gank" },
                100));
            setup.opponentSpec = Spec::Support;
            setup.opponentMaxHp = 1000;

            test.Expect(session.StartBattle(setup).accepted, "mark battle starts");
            BattleActionResult mark = session.UsePlayerSkill("jungle-gank");
            test.Expect(mark.accepted, "mark ability is accepted");
            test.Expect(ContainsEventType(mark.events, BattleEventType::MarkApplied), "mark is applied");

            BattleActionResult trigger = session.UsePlayerSkill("jungle-basic");
            test.Expect(trigger.accepted, "follow-up basic is accepted");
            test.Expect(ContainsEventType(trigger.events, BattleEventType::MarkTriggered), "damaging follow-up triggers mark");
        }

        {
            BattleRules rules(data);
            const Skill* adcBasic = data.FindSkill("adc-basic");
            test.Expect(adcBasic != nullptr, "ADC basic exists for modifier tests");
            if (adcBasic != nullptr)
            {
                Competitor attacker = MakeCompetitor(Spec::Adc, "");
                Competitor defender = MakeCompetitor(Spec::Support, "");
                BattleStatus plainAttacker;
                BattleStatus plainDefender;
                const DamageResult plain = rules.CalculateDamage(
                    *adcBasic,
                    { adcBasic->id, 1, 0 },
                    attacker,
                    plainAttacker,
                    defender,
                    plainDefender);

                BattleStatus buffedAttacker;
                buffedAttacker.attackModifierPercent = 50;
                buffedAttacker.attackModifierTurns = 1;
                buffedAttacker.attackPenetrationPercent = 50;
                buffedAttacker.attackPenetrationTurns = 1;
                BattleStatus defendedTarget;
                defendedTarget.defenseModifierPercent = 40;
                defendedTarget.defenseModifierTurns = 1;
                const DamageResult modified = rules.CalculateDamage(
                    *adcBasic,
                    { adcBasic->id, 1, 0 },
                    attacker,
                    buffedAttacker,
                    defender,
                    defendedTarget);
                test.Expect(modified.amount > plain.amount, "attack buff and penetration increase damage through defense");

                BattleStatus rootedTarget;
                rootedTarget.rootedTurns = 1;
                const DamageResult rooted = rules.CalculateDamage(
                    *adcBasic,
                    { adcBasic->id, 1, 0 },
                    attacker,
                    plainAttacker,
                    defender,
                    rootedTarget);
                test.Expect(rooted.amount > plain.amount, "rooted targets take setup follow-up damage");
            }

            const Skill* topSunder = data.FindSkill("top-sunder");
            test.Expect(topSunder != nullptr, "top spender exists for cooldown modifier tests");
            if (topSunder != nullptr)
            {
                BattleStatus reduced;
                reduced.cooldownModifierPercent = -50;
                reduced.cooldownModifierTurns = 1;
                BattleStatus increased;
                increased.cooldownModifierPercent = 50;
                increased.cooldownModifierTurns = 1;
                test.ExpectEqual(rules.GetCooldownTurns(*topSunder, reduced), 1, "cooldown reduction lowers cooldown");
                test.ExpectEqual(rules.GetCooldownTurns(*topSunder, increased), 3, "cooldown increase raises cooldown");
            }
        }

        {
            BattleRules rules(data);
            ProgressionSystem progression;
            SkillSystem skillSystem(data, rules, progression);
            OpponentAI ai(data, rules, skillSystem);
            Competitor opponent = MakeCompetitor(Spec::Adc, "");
            opponent.skills.push_back({ "adc-basic", 1, 0 });
            opponent.skills.push_back({ "adc-multi-strike", 1, 0 });
            opponent.abilityStates.push_back({ "adc-basic", 0 });
            opponent.abilityStates.push_back({ "adc-multi-strike", 0 });
            opponent.mana = 0;
            Competitor player = MakeCompetitor(Spec::Support, "");
            BattleStatus opponentStatus;
            BattleStatus playerStatus;
            std::mt19937 randomEngine(42);
            SkillProgress* selected = ai.SelectSkill(opponent, opponentStatus, player, playerStatus, randomEngine);
            test.Expect(selected != nullptr, "AI finds a legal fallback ability");
            if (selected != nullptr)
            {
                test.ExpectEqual(selected->skillId, std::string("adc-basic"), "AI falls back to basic when spender is illegal");
            }
        }
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
        { "SimulationData references", TestSimulationDataReferences },
        { "TraitBattleRules", TestTraitBattleRules },
        { "TrainerProfile", TestTrainerProfile },
        { "RatingSystem", TestRatingSystem },
        { "ScoutSystem", TestScoutSystem },
        { "BattleSession switching and XP sharing", TestBattleSessionSwitchingAndXpShare },
        { "BattleSession opponent team 3v3", TestBattleSessionOpponentTeam3v3 },
        { "BattleSession player KO requires objective finish", TestBattleSessionPlayerKoRequiresObjectiveFinish },
        { "BattleSession opponent KO respawns", TestBattleSessionOpponentKoRespawns },
        { "BattleSession dragon objective", TestBattleSessionDragonObjective },
        { "Mana, cooldown, and control mechanics", TestManaCooldownAndControlMechanics },
        { "Marks, modifiers, and opponent AI", TestMarksBuffsDebuffsAndOpponentAI },
        { "BattleSession seeded replay", TestBattleSessionSeededReplay },
    };

    int totalFailures = 0;
    for (const TestCase& testCase : tests)
    {
        std::cout << "Running " << testCase.name << "..." << std::endl;
        TestContext context;
        try
        {
            testCase.run(context);
        }
        catch (const std::exception& exception)
        {
            context.Expect(false, std::string("unhandled exception: ") + exception.what());
        }

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
