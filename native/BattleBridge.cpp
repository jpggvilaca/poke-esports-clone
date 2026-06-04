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
    : session_(data_),
      profile_(PlayerProfile::CreateNew("Player", GameType::LeagueOfLegends, Spec::Top, data_))
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
    int player_spec,
    int player_style,
    int opponent_spec,
    int opponent_style)
{
    if (!IsValidSpec(player_spec) || !IsValidSpec(opponent_spec))
    {
        return Rejected("Unknown spec.");
    }

    if (!IsValidStyle(player_style) || !IsValidStyle(opponent_style))
    {
        return Rejected("Unknown style.");
    }

    BattleSetup setup;
    setup.playerSpec = static_cast<Spec>(player_spec);
    setup.playerStyle = static_cast<Style>(player_style);
    if (profile_.GetState().spec == setup.playerSpec)
    {
        setup.playerPassiveBonuses = profile_.GetPassiveBonuses();
    }
    setup.opponentSpec = static_cast<Spec>(opponent_spec);
    setup.opponentStyle = static_cast<Style>(opponent_style);
    return ToArray(session_.StartBattle(setup));
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

    return ToArray(session_.UsePlayerSkill(id));
}

godot::Array BattleBridge::change_style(int style)
{
    if (!IsValidStyle(style))
    {
        return Rejected("Unknown style.");
    }

    return ToArray(session_.ChangePlayerStyle(static_cast<Style>(style)));
}

godot::Dictionary BattleBridge::get_battle_state() const
{
    const BattleState state = session_.GetState();
    godot::Dictionary row;
    row["started"] = state.started;
    row["finished"] = state.finished;
    row["winner"] = ToString(state.winner);
    row["player"] = ToDictionary(state.player);
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

godot::Dictionary BattleBridge::create_profile(const godot::String& player_name, int spec)
{
    if (!IsValidSpec(spec))
    {
        ProfileCommandResult result;
        result.error = "Unknown spec.";
        return ToDictionary(result);
    }

    profile_ = PlayerProfile::CreateNew(
        ToStandardString(player_name),
        GameType::LeagueOfLegends,
        static_cast<Spec>(spec),
        data_);

    ProfileCommandResult result;
    result.accepted = true;
    return ToDictionary(result);
}

godot::Dictionary BattleBridge::get_profile_state() const
{
    return ToDictionary(profile_.GetState());
}

godot::Dictionary BattleBridge::profile_award_xp(int amount)
{
    return ToDictionary(profile_.AwardPlayerXp(amount));
}

godot::Dictionary BattleBridge::profile_award_rating(int amount)
{
    return ToDictionary(profile_.AwardRating(amount));
}

godot::Dictionary BattleBridge::profile_award_money(int amount)
{
    return ToDictionary(profile_.AwardMoney(amount));
}

godot::Dictionary BattleBridge::profile_learn_skill(const godot::String& skill_id)
{
    return ToDictionary(profile_.LearnSkill(ToStandardString(skill_id)));
}

godot::Dictionary BattleBridge::profile_equip_skill(const godot::String& skill_id)
{
    return ToDictionary(profile_.EquipSkill(ToStandardString(skill_id)));
}

godot::Dictionary BattleBridge::profile_unequip_skill(const godot::String& skill_id)
{
    return ToDictionary(profile_.UnequipSkill(ToStandardString(skill_id)));
}

godot::Dictionary BattleBridge::profile_add_trophy(const godot::String& trophy_id)
{
    return ToDictionary(profile_.AddTrophy(ToStandardString(trophy_id)));
}

godot::Dictionary BattleBridge::profile_apply_match_result(int opponent_level, int context, bool won)
{
    if (!IsValidMatchContext(context))
    {
        RatingResult result;
        result.error = "Unknown match context.";
        return ToDictionary(result);
    }

    const PlayerProfileState& profile = profile_.GetState();
    RatingResult result = rating_.CalculateChange(
        profile.rating,
        profile.level,
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
        godot::D_METHOD("start_battle", "player_spec", "player_style", "opponent_spec", "opponent_style"),
        &BattleBridge::start_battle);
    godot::ClassDB::bind_method(godot::D_METHOD("use_skill", "skill_id"), &BattleBridge::use_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("change_style", "style"), &BattleBridge::change_style);
    godot::ClassDB::bind_method(godot::D_METHOD("get_battle_state"), &BattleBridge::get_battle_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_available_skills"), &BattleBridge::get_available_skills);
    godot::ClassDB::bind_method(godot::D_METHOD("create_profile", "player_name", "spec"), &BattleBridge::create_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("get_profile_state"), &BattleBridge::get_profile_state);
    godot::ClassDB::bind_method(godot::D_METHOD("profile_award_xp", "amount"), &BattleBridge::profile_award_xp);
    godot::ClassDB::bind_method(godot::D_METHOD("profile_award_rating", "amount"), &BattleBridge::profile_award_rating);
    godot::ClassDB::bind_method(godot::D_METHOD("profile_award_money", "amount"), &BattleBridge::profile_award_money);
    godot::ClassDB::bind_method(godot::D_METHOD("profile_learn_skill", "skill_id"), &BattleBridge::profile_learn_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("profile_equip_skill", "skill_id"), &BattleBridge::profile_equip_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("profile_unequip_skill", "skill_id"), &BattleBridge::profile_unequip_skill);
    godot::ClassDB::bind_method(godot::D_METHOD("profile_add_trophy", "trophy_id"), &BattleBridge::profile_add_trophy);
    godot::ClassDB::bind_method(
        godot::D_METHOD("profile_apply_match_result", "opponent_level", "context", "won"),
        &BattleBridge::profile_apply_match_result);
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

godot::Dictionary BattleBridge::ToDictionary(const PlayerProfileState& profile) const
{
    godot::Dictionary row;
    row["player_name"] = ToGodotString(profile.playerName);
    row["game_type"] = static_cast<int>(profile.gameType);
    row["game_type_name"] = ToGodotString(::ToString(profile.gameType));
    row["spec"] = static_cast<int>(profile.spec);
    row["spec_name"] = ToGodotString(::ToString(profile.spec));
    row["rank"] = static_cast<int>(profile.rank);
    row["rank_name"] = ToGodotString(::ToString(profile.rank));
    row["level"] = profile.level;
    row["xp"] = profile.xp;
    row["xp_required"] = profile.xpRequiredForNextLevel;
    row["rating"] = profile.rating;
    row["money"] = profile.money;
    row["bonus_max_hp"] = profile.passiveBonuses.maxHpBonus;
    row["bonus_base_power"] = profile.passiveBonuses.basePowerBonus;
    row["bonus_counter_damage_percent"] = profile.passiveBonuses.counterDamageBonusPercent;
    row["learned_skills"] = ToSkillArray(profile.learnedSkillIds);
    row["active_skills"] = ToSkillArray(profile.activeSkillIds);
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
