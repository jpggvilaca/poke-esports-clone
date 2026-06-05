#include "BattleBridge.h"

#include <godot_cpp/core/class_db.hpp>

namespace
{
    bool IsValidSpec(int value)
    {
        return value >= static_cast<int>(Spec::Top)
            && value <= static_cast<int>(Spec::Support);
    }

    bool IsValidStyle(int value)
    {
        return value >= static_cast<int>(Style::Aggressive)
            && value <= static_cast<int>(Style::Balanced);
    }

    godot::String ToGodotString(const std::string& value)
    {
        return godot::String(value.c_str());
    }

    std::string ToStandardString(const godot::String& value)
    {
        return std::string(value.utf8().get_data());
    }

    godot::String ToString(BattleActor actor)
    {
        switch (actor)
        {
        case BattleActor::Player: return "player";
        case BattleActor::Opponent: return "opponent";
        case BattleActor::None: return "none";
        }

        return "none";
    }

    godot::String ToString(BattleWinner winner)
    {
        switch (winner)
        {
        case BattleWinner::Player: return "player";
        case BattleWinner::Opponent: return "opponent";
        case BattleWinner::None: return "none";
        }

        return "none";
    }

    godot::String ToString(SkillEffectType type)
    {
        switch (type)
        {
        case SkillEffectType::None: return "none";
        case SkillEffectType::Heal: return "heal";
        case SkillEffectType::AttackModifier: return "attack_modifier";
        case SkillEffectType::DefenseModifier: return "defense_modifier";
        }

        return "none";
    }

    godot::String ToString(SkillEffectTarget target)
    {
        return target == SkillEffectTarget::Self ? "self" : "opponent";
    }

    godot::String ToString(SkillTone tone)
    {
        switch (tone)
        {
        case SkillTone::Basic: return "basic";
        case SkillTone::Aggressive: return "aggressive";
        case SkillTone::Defensive: return "defensive";
        case SkillTone::Balanced: return "balanced";
        case SkillTone::Risky: return "risky";
        case SkillTone::Utility: return "utility";
        }

        return "basic";
    }

    godot::String ToString(MatchContext context)
    {
        switch (context)
        {
        case MatchContext::Tutorial: return "tutorial";
        case MatchContext::Normal: return "normal";
        case MatchContext::Nemesis: return "nemesis";
        case MatchContext::Major: return "major";
        }

        return "normal";
    }
}

BattleBridge::BattleBridge()
    : playerProfiles_(data_),
      session_(data_),
      profile_(TrainerProfile::CreateNew("Trainer", GameType::LeagueOfLegends, Spec::Top, playerProfiles_))
{
}

godot::Array BattleBridge::get_specs() const
{
    godot::Array specs;
    for (const SpecData& spec : data_.GetSpecs())
    {
        godot::Dictionary row;
        row["id"] = static_cast<int>(spec.spec);
        row["name"] = ToGodotString(spec.name);
        specs.push_back(row);
    }

    return specs;
}

godot::Array BattleBridge::get_styles() const
{
    godot::Array styles;
    for (int id = static_cast<int>(Style::Aggressive); id <= static_cast<int>(Style::Balanced); ++id)
    {
        godot::Dictionary row;
        row["id"] = id;
        row["name"] = ToGodotString(::ToString(static_cast<Style>(id)));
        styles.push_back(row);
    }

    return styles;
}

godot::Array BattleBridge::start_battle(
    int player_style,
    int opponent_spec,
    int opponent_style)
{
    if (!IsValidSpec(opponent_spec))
    {
        return Rejected("Unknown spec.");
    }

    if (!IsValidStyle(player_style) || !IsValidStyle(opponent_style))
    {
        return Rejected("Unknown style.");
    }

    const TrainerProfileState& trainer = profile_.GetState();
    if (trainer.roster.empty())
    {
        return Rejected("No active player profile.");
    }

    BattleSetup setup;
    setup.activePlayerIndex = trainer.activePlayerIndex;
    for (int index = 0; index < static_cast<int>(trainer.roster.size()); ++index)
    {
        const PlayerProfileState& playerProfile = trainer.roster[index];
        BattleSetup::PlayerSlot slot;
        slot.profileIndex = index;
        slot.name = playerProfile.name;
        slot.spec = playerProfile.spec;
        slot.style = static_cast<Style>(player_style);
        slot.passiveBonuses = playerProfile.passiveBonuses;
        for (const std::string& skillId : playerProfile.activeSkillIds)
        {
            slot.skills.push_back({ skillId });
        }
        setup.playerTeam.push_back(slot);
    }
    setup.opponentSpec = static_cast<Spec>(opponent_spec);
    setup.opponentStyle = static_cast<Style>(opponent_style);
    return ToArrayAndApplyRewards(session_.StartBattle(setup));
}

godot::Array BattleBridge::use_skill(const godot::String& skill_id)
{
    const std::string id = ToStandardString(skill_id);
    bool isAvailable = false;
    for (const SkillView& skill : session_.GetAvailablePlayerSkills())
    {
        if (skill.id == id)
        {
            isAvailable = true;
            break;
        }
    }

    if (!isAvailable)
    {
        return Rejected("Unknown or unavailable skill.");
    }

    return ToArrayAndApplyRewards(session_.UsePlayerSkill(id));
}

godot::Array BattleBridge::switch_player(int player_index)
{
    return ToArrayAndApplyRewards(session_.SwitchPlayer(player_index));
}

godot::Array BattleBridge::change_style(int style)
{
    if (!IsValidStyle(style))
    {
        return Rejected("Unknown style.");
    }

    return ToArrayAndApplyRewards(session_.ChangePlayerStyle(static_cast<Style>(style)));
}

godot::Dictionary BattleBridge::get_battle_state() const
{
    const BattleState state = session_.GetState();
    godot::Dictionary row;
    row["started"] = state.started;
    row["finished"] = state.finished;
    row["winner"] = ToString(state.winner);
    row["active_player_index"] = state.activePlayerIndex;
    row["player"] = ToDictionary(state.player);
    godot::Array playerTeam;
    for (const CompetitorView& player : state.playerTeam)
    {
        playerTeam.push_back(ToDictionary(player));
    }
    row["player_team"] = playerTeam;
    row["opponent"] = ToDictionary(state.opponent);
    return row;
}

godot::Array BattleBridge::get_available_skills() const
{
    const BattleState state = session_.GetState();
    godot::Array skills;
    for (const SkillView& skill : session_.GetAvailablePlayerSkills())
    {
        skills.push_back(ToDictionary(skill, state.player.focus));
    }

    return skills;
}

godot::Dictionary BattleBridge::create_trainer(const godot::String& trainer_name, int starter_spec)
{
    if (!IsValidSpec(starter_spec))
    {
        ProfileCommandResult result;
        result.error = "Unknown spec.";
        return ToDictionary(result);
    }

    profile_ = TrainerProfile::CreateNew(
        ToStandardString(trainer_name),
        GameType::LeagueOfLegends,
        static_cast<Spec>(starter_spec),
        playerProfiles_);

    ProfileCommandResult result;
    result.accepted = true;
    return ToDictionary(result);
}

godot::Dictionary BattleBridge::get_trainer_state() const
{
    return ToDictionary(profile_.GetState());
}

godot::Dictionary BattleBridge::active_player_award_xp(int amount)
{
    PlayerProfileState* activePlayer = profile_.GetMutableActivePlayerProfile();
    if (activePlayer == nullptr)
    {
        ProfileCommandResult result;
        result.error = "No active player profile.";
        return ToDictionary(result);
    }

    return ToDictionary(playerProfiles_.AwardXp(*activePlayer, amount));
}

godot::Dictionary BattleBridge::active_player_learn_skill(const godot::String& skill_id)
{
    PlayerProfileState* activePlayer = profile_.GetMutableActivePlayerProfile();
    if (activePlayer == nullptr)
    {
        ProfileCommandResult result;
        result.error = "No active player profile.";
        return ToDictionary(result);
    }

    return ToDictionary(playerProfiles_.LearnSkill(*activePlayer, ToStandardString(skill_id)));
}

godot::Dictionary BattleBridge::active_player_equip_skill(const godot::String& skill_id)
{
    PlayerProfileState* activePlayer = profile_.GetMutableActivePlayerProfile();
    if (activePlayer == nullptr)
    {
        ProfileCommandResult result;
        result.error = "No active player profile.";
        return ToDictionary(result);
    }

    return ToDictionary(playerProfiles_.EquipSkill(*activePlayer, ToStandardString(skill_id)));
}

godot::Dictionary BattleBridge::active_player_unequip_skill(const godot::String& skill_id)
{
    PlayerProfileState* activePlayer = profile_.GetMutableActivePlayerProfile();
    if (activePlayer == nullptr)
    {
        ProfileCommandResult result;
        result.error = "No active player profile.";
        return ToDictionary(result);
    }

    return ToDictionary(playerProfiles_.UnequipSkill(*activePlayer, ToStandardString(skill_id)));
}

godot::Dictionary BattleBridge::trainer_award_rating(int amount)
{
    return ToDictionary(profile_.AwardRating(amount));
}

godot::Dictionary BattleBridge::trainer_award_money(int amount)
{
    return ToDictionary(profile_.AwardMoney(amount));
}

godot::Dictionary BattleBridge::trainer_add_trophy(const godot::String& trophy_id)
{
    return ToDictionary(profile_.AddTrophy(ToStandardString(trophy_id)));
}

godot::Dictionary BattleBridge::trainer_add_player(const godot::String& player_name, int spec)
{
    if (!IsValidSpec(spec))
    {
        ProfileCommandResult result;
        result.error = "Unknown spec.";
        return ToDictionary(result);
    }

    PlayerProfileState playerProfile = playerProfiles_.CreateStarter(
        ToStandardString(player_name),
        static_cast<Spec>(spec));
    return ToDictionary(profile_.AddPlayerProfile(playerProfile));
}

godot::Dictionary BattleBridge::trainer_apply_match_result(int opponent_level, int context, bool won)
{
    if (!IsValidMatchContext(context))
    {
        RatingResult result;
        result.error = "Unknown match context.";
        return ToDictionary(result);
    }

    const TrainerProfileState& profile = profile_.GetState();
    RatingResult result = rating_.CalculateChange(
        profile.rating,
        profile_.GetActivePlayerLevel(),
        opponent_level,
        static_cast<MatchContext>(context),
        won);
    if (!result.accepted)
    {
        return ToDictionary(result);
    }

    profile_.AwardRating(result.ratingChange);
    return ToDictionary(result);
}

void BattleBridge::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_specs"), &BattleBridge::get_specs);
    godot::ClassDB::bind_method(godot::D_METHOD("get_styles"), &BattleBridge::get_styles);
    godot::ClassDB::bind_method(
        godot::D_METHOD("start_battle", "player_style", "opponent_spec", "opponent_style"),
        &BattleBridge::start_battle);
    godot::ClassDB::bind_method(godot::D_METHOD("use_skill", "skill_id"), &BattleBridge::use_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("switch_player", "player_index"), &BattleBridge::switch_player);
    godot::ClassDB::bind_method(godot::D_METHOD("change_style", "style"), &BattleBridge::change_style);
    godot::ClassDB::bind_method(godot::D_METHOD("get_battle_state"), &BattleBridge::get_battle_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_available_skills"), &BattleBridge::get_available_skills);
    godot::ClassDB::bind_method(godot::D_METHOD("create_trainer", "trainer_name", "starter_spec"), &BattleBridge::create_trainer);
    godot::ClassDB::bind_method(godot::D_METHOD("get_trainer_state"), &BattleBridge::get_trainer_state);
    godot::ClassDB::bind_method(godot::D_METHOD("active_player_award_xp", "amount"), &BattleBridge::active_player_award_xp);
    godot::ClassDB::bind_method(godot::D_METHOD("active_player_learn_skill", "skill_id"), &BattleBridge::active_player_learn_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("active_player_equip_skill", "skill_id"), &BattleBridge::active_player_equip_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("active_player_unequip_skill", "skill_id"), &BattleBridge::active_player_unequip_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("trainer_award_rating", "amount"), &BattleBridge::trainer_award_rating);
    godot::ClassDB::bind_method(godot::D_METHOD("trainer_award_money", "amount"), &BattleBridge::trainer_award_money);
    godot::ClassDB::bind_method(godot::D_METHOD("trainer_add_trophy", "trophy_id"), &BattleBridge::trainer_add_trophy);
    godot::ClassDB::bind_method(godot::D_METHOD("trainer_add_player", "player_name", "spec"), &BattleBridge::trainer_add_player);
    godot::ClassDB::bind_method(
        godot::D_METHOD("trainer_apply_match_result", "opponent_level", "context", "won"),
        &BattleBridge::trainer_apply_match_result);
}

godot::Array BattleBridge::ToArray(const BattleActionResult& result) const
{
    if (!result.accepted)
    {
        return Rejected(ToGodotString(result.error));
    }

    godot::Array events;
    if (result.battleStarted)
    {
        events.push_back(CreateEvent("battle_started", BattleActor::None, BattleActor::None));
    }

    if (result.styleChanged)
    {
        events.push_back(CreateEvent(
            "style_changed",
            BattleActor::Player,
            BattleActor::Player,
            "",
            "",
            static_cast<int>(result.newStyle)));
    }

    if (result.playerSwitched)
    {
        events.push_back(CreateEvent(
            "player_switched",
            BattleActor::Player,
            BattleActor::Player,
            "",
            ToGodotString(result.newPlayerName),
            result.newPlayerIndex));
    }

    for (const SkillUseResult& skillUse : result.skillUses)
    {
        AppendSkillUse(events, skillUse);
    }

    if (result.battleFinished)
    {
        const BattleActor winner = result.winner == BattleWinner::Player
            ? BattleActor::Player
            : BattleActor::Opponent;
        events.push_back(CreateEvent("battle_finished", winner, BattleActor::None));
    }

    return events;
}

godot::Array BattleBridge::ToArrayAndApplyRewards(const BattleActionResult& result)
{
    godot::Array events = ToArray(result);
    if (result.accepted && result.reward.awarded)
    {
        AppendBattleReward(events, result.reward);
    }

    return events;
}

godot::Array BattleBridge::Rejected(const godot::String& message) const
{
    godot::Dictionary event;
    event["type"] = "action_rejected";
    event["actor"] = "player";
    event["target"] = "player";
    event["message"] = message;

    godot::Array events;
    events.push_back(event);
    return events;
}

void BattleBridge::AppendSkillUse(godot::Array& events, const SkillUseResult& skillUse) const
{
    if (!skillUse.used)
    {
        return;
    }

    events.push_back(CreateEvent(
        "skill_used",
        skillUse.actor,
        skillUse.target,
        skillUse.skillId));

    if (!skillUse.hit)
    {
        events.push_back(CreateEvent(
            "missed",
            skillUse.actor,
            skillUse.target,
            skillUse.skillId));
        AppendSkillXp(events, skillUse);
        return;
    }

    if (skillUse.damage.applied)
    {
        if (skillUse.damage.effectiveness == Effectiveness::SuperEffective)
        {
            events.push_back(CreateEvent(
                "super_effective",
                skillUse.actor,
                skillUse.target,
                skillUse.skillId));
        }
        else if (skillUse.damage.effectiveness == Effectiveness::NotVeryEffective)
        {
            events.push_back(CreateEvent(
                "not_very_effective",
                skillUse.actor,
                skillUse.target,
                skillUse.skillId));
        }

        events.push_back(CreateEvent(
            "damage_dealt",
            skillUse.actor,
            skillUse.target,
            skillUse.skillId,
            "",
            skillUse.damage.amount));
    }

    if (skillUse.effect.applied)
    {
        if (skillUse.effect.type == SkillEffectType::Heal)
        {
            events.push_back(CreateEvent(
                "healed",
                skillUse.actor,
                skillUse.effect.target,
                skillUse.skillId,
                "",
                skillUse.effect.healingAmount));
        }
        else
        {
            events.push_back(CreateEvent(
                skillUse.effect.type == SkillEffectType::AttackModifier
                    ? "attack_modified"
                    : "defense_modified",
                skillUse.actor,
                skillUse.effect.target,
                skillUse.skillId,
                "",
                skillUse.effect.value,
                skillUse.effect.duration));
        }
    }

    AppendSkillXp(events, skillUse);
}

void BattleBridge::AppendSkillXp(godot::Array& events, const SkillUseResult& skillUse) const
{
    if (!skillUse.xp.leveledUp)
    {
        return;
    }

    events.push_back(CreateEvent(
        "skill_leveled_up",
        skillUse.actor,
        skillUse.actor,
        skillUse.skillId,
        "",
        skillUse.xp.newLevel));
}

void BattleBridge::AppendBattleReward(godot::Array& events, const BattleRewardResult& reward)
{
    events.push_back(CreateEvent(
        "battle_xp_awarded",
        BattleActor::Player,
        BattleActor::Player,
        "",
        "",
        reward.totalXp,
        reward.xpPerParticipant));

    for (int playerIndex : reward.participantPlayerIndices)
    {
        PlayerProfileState* playerProfile = profile_.GetMutablePlayerProfile(playerIndex);
        if (playerProfile == nullptr)
        {
            continue;
        }

        ProfileCommandResult xp = playerProfiles_.AwardXp(*playerProfile, reward.xpPerParticipant);
        godot::Dictionary event = CreateEvent(
            xp.leveledUp ? "player_leveled_up" : "player_xp_gained",
            BattleActor::Player,
            BattleActor::Player,
            "",
            ToGodotString(playerProfile->name),
            reward.xpPerParticipant,
            playerIndex);
        event["old_level"] = xp.oldLevel;
        event["new_level"] = xp.newLevel;
        event["old_xp"] = xp.oldValue;
        event["new_xp"] = xp.newValue;
        events.push_back(event);
    }
}

godot::Dictionary BattleBridge::CreateEvent(
    const char* type,
    BattleActor actor,
    BattleActor target,
    const std::string& skillId,
    const godot::String& message,
    int value,
    int duration) const
{
    godot::Dictionary row;
    row["type"] = type;
    row["actor"] = ToString(actor);
    row["target"] = ToString(target);
    row["skill_id"] = ToGodotString(skillId);
    const Skill* skill = data_.FindSkill(skillId);
    row["skill_name"] = skill == nullptr ? "" : ToGodotString(skill->name);
    row["message"] = message;
    row["value"] = value;
    row["duration"] = duration;
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const CompetitorView& competitor) const
{
    godot::Dictionary status;
    status["attack_modifier_percent"] = competitor.status.attackModifierPercent;
    status["attack_modifier_hits"] = competitor.status.attackModifierHits;
    status["defense_modifier_percent"] = competitor.status.defenseModifierPercent;
    status["defense_modifier_hits"] = competitor.status.defenseModifierHits;

    godot::Dictionary row;
    row["profile_index"] = competitor.profileIndex;
    row["name"] = ToGodotString(competitor.name);
    row["spec"] = static_cast<int>(competitor.spec);
    row["spec_name"] = ToGodotString(::ToString(competitor.spec));
    row["style"] = static_cast<int>(competitor.style);
    row["style_name"] = ToGodotString(::ToString(competitor.style));
    row["hp"] = competitor.hp;
    row["max_hp"] = competitor.maxHp;
    row["focus"] = competitor.focus;
    row["max_focus"] = competitor.maxFocus;
    row["base_power"] = competitor.basePower;
    row["counter_damage_bonus_percent"] = competitor.counterDamageBonusPercent;
    row["status"] = status;
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const SkillView& skill, int available_focus) const
{
    godot::Dictionary row;
    row["id"] = ToGodotString(skill.id);
    row["name"] = ToGodotString(skill.name);
    row["description"] = ToGodotString(skill.description);
    row["tone"] = ToGodotString(::ToString(skill.tone));
    row["power"] = skill.power;
    row["focus_cost"] = skill.focusCost;
    row["accuracy"] = skill.accuracy;
    row["level"] = skill.level;
    row["xp"] = skill.xp;
    row["effect_type"] = ToString(skill.effectType);
    row["effect_target"] = ToString(skill.effectTarget);
    row["effect_value"] = skill.effectValue;
    row["effect_uses"] = skill.effectUses;
    row["affordable"] = skill.focusCost <= available_focus;
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const PlayerProfileState& playerProfile) const
{
    godot::Dictionary row;
    row["name"] = ToGodotString(playerProfile.name);
    row["spec"] = static_cast<int>(playerProfile.spec);
    row["spec_name"] = ToGodotString(::ToString(playerProfile.spec));
    row["rank"] = static_cast<int>(playerProfile.rank);
    row["rank_name"] = ToGodotString(::ToString(playerProfile.rank));
    row["level"] = playerProfile.level;
    row["xp"] = playerProfile.xp;
    row["xp_required"] = playerProfile.xpRequiredForNextLevel;
    row["bonus_max_hp"] = playerProfile.passiveBonuses.maxHpBonus;
    row["bonus_base_power"] = playerProfile.passiveBonuses.basePowerBonus;
    row["bonus_counter_damage_percent"] = playerProfile.passiveBonuses.counterDamageBonusPercent;
    row["learned_skills"] = ToSkillArray(playerProfile.learnedSkillIds);
    row["active_skills"] = ToSkillArray(playerProfile.activeSkillIds);
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const TrainerProfileState& profile) const
{
    godot::Dictionary row;
    row["trainer_name"] = ToGodotString(profile.trainerName);
    row["game_type"] = static_cast<int>(profile.gameType);
    row["game_type_name"] = ToGodotString(::ToString(profile.gameType));
    row["rating"] = profile.rating;
    row["money"] = profile.money;
    row["active_player_index"] = profile.activePlayerIndex;

    godot::Array roster;
    for (const PlayerProfileState& playerProfile : profile.roster)
    {
        roster.push_back(ToDictionary(playerProfile));
    }

    row["roster"] = roster;
    row["active_player_profile"] = profile.activePlayerIndex >= 0
        && profile.activePlayerIndex < static_cast<int>(profile.roster.size())
        ? ToDictionary(profile.roster[profile.activePlayerIndex])
        : godot::Dictionary();
    row["trophies"] = ToStringArray(profile.trophyIds);
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const ProfileCommandResult& result) const
{
    godot::Dictionary row;
    row["accepted"] = result.accepted;
    row["error"] = ToGodotString(result.error);
    row["old_value"] = result.oldValue;
    row["new_value"] = result.newValue;
    row["old_level"] = result.oldLevel;
    row["new_level"] = result.newLevel;
    row["leveled_up"] = result.leveledUp;
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const RatingResult& result) const
{
    godot::Dictionary row;
    row["accepted"] = result.accepted;
    row["error"] = ToGodotString(result.error);
    row["won"] = result.won;
    row["context"] = static_cast<int>(result.context);
    row["context_name"] = ToString(result.context);
    row["player_level"] = result.playerLevel;
    row["opponent_level"] = result.opponentLevel;
    row["old_rating"] = result.oldRating;
    row["rating_change"] = result.ratingChange;
    row["new_rating"] = result.newRating;
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const BattleRewardResult& result) const
{
    godot::Dictionary row;
    row["awarded"] = result.awarded;
    row["total_xp"] = result.totalXp;
    row["xp_per_participant"] = result.xpPerParticipant;

    godot::Array participants;
    for (int playerIndex : result.participantPlayerIndices)
    {
        participants.push_back(playerIndex);
    }
    row["participant_player_indices"] = participants;
    return row;
}

godot::Array BattleBridge::ToSkillArray(const std::vector<std::string>& skillIds) const
{
    godot::Array rows;
    for (const std::string& skillId : skillIds)
    {
        godot::Dictionary row;
        row["id"] = ToGodotString(skillId);
        const Skill* skill = data_.FindSkill(skillId);
        row["name"] = skill == nullptr ? ToGodotString(skillId) : ToGodotString(skill->name);
        rows.push_back(row);
    }

    return rows;
}

bool BattleBridge::IsValidMatchContext(int value) const
{
    return value >= static_cast<int>(MatchContext::Tutorial)
        && value <= static_cast<int>(MatchContext::Major);
}

godot::Array BattleBridge::ToStringArray(const std::vector<std::string>& values) const
{
    godot::Array rows;
    for (const std::string& value : values)
    {
        rows.push_back(ToGodotString(value));
    }

    return rows;
}
