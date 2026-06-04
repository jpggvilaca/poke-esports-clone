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
}

BattleBridge::BattleBridge()
    : session_(data_)
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
    row["status"] = status;
    return row;
}

godot::Dictionary BattleBridge::ToDictionary(const SkillView& skill, int available_focus) const
{
    godot::Dictionary row;
    row["id"] = ToGodotString(skill.id);
    row["name"] = ToGodotString(skill.name);
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
