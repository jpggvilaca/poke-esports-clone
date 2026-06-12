#include "SimulationData.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
    Skill MakeSkill(
        const std::string& id,
        const std::string& name,
        int manaCost,
        int manaGain,
        int cooldownTurns,
        int power,
        double accuracy,
        SkillEffectType effectType = SkillEffectType::None,
        SkillEffectTarget effectTarget = SkillEffectTarget::Self,
        int effectValue = 0,
        int durationTurns = 0,
        int markBonusDamage = 0)
    {
        Skill skill;
        skill.id = id;
        skill.name = name;
        skill.manaCost = manaCost;
        skill.manaGain = manaGain;
        skill.cooldownTurns = cooldownTurns;
        skill.power = power;
        skill.accuracy = accuracy;
        skill.effectType = effectType;
        skill.effectTarget = effectTarget;
        skill.effectValue = effectValue;
        skill.durationTurns = durationTurns;
        skill.markBonusDamage = markBonusDamage;
        if (effectType != SkillEffectType::None)
        {
            skill.effects.push_back({ effectType, effectTarget, effectValue, durationTurns, markBonusDamage });
        }
        return skill;
    }

    std::string InferSkillSourceSpec(const std::string& skillId)
    {
        const std::size_t delimiter = skillId.find('-');
        if (delimiter == std::string::npos)
        {
            return "";
        }

        return skillId.substr(0, delimiter);
    }

    TraitDefinition MakeTrait(
        const std::string& id,
        const std::string& name,
        const std::string& description,
        TraitEffectType effectType,
        int effectValue)
    {
        TraitDefinition trait;
        trait.id = id;
        trait.name = name;
        trait.description = description;
        trait.effectType = effectType;
        trait.effectValue = effectValue;
        return trait;
    }

    DrillDefinition MakeDrill(
        GameType gameType,
        const std::string& id,
        const std::string& displayName,
        const std::string& description,
        int missManaGain,
        int goodManaGain,
        int perfectManaGain)
    {
        DrillDefinition drill;
        drill.gameType = gameType;
        drill.id = id;
        drill.displayName = displayName;
        drill.description = description;
        drill.missManaGain = missManaGain;
        drill.goodManaGain = goodManaGain;
        drill.perfectManaGain = perfectManaGain;
        return drill;
    }

    std::string Trim(const std::string& value)
    {
        const std::size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return "";
        }

        const std::size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::vector<std::string> Split(const std::string& value, char delimiter)
    {
        std::vector<std::string> parts;
        std::stringstream input(value);
        std::string part;
        while (std::getline(input, part, delimiter))
        {
            parts.push_back(Trim(part));
        }
        return parts;
    }

    std::vector<std::string> ParseCsvRow(const std::string& line)
    {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;

        for (char character : line)
        {
            if (character == '"')
            {
                in_quotes = !in_quotes;
                continue;
            }

            if (character == ',' && !in_quotes)
            {
                fields.push_back(Trim(field));
                field.clear();
                continue;
            }

            field.push_back(character);
        }

        fields.push_back(Trim(field));
        return fields;
    }

    bool IsHeaderRow(const std::vector<std::string>& fields, const std::string& firstField)
    {
        return !fields.empty() && fields[0] == firstField;
    }

    SkillEffectType SkillEffectTypeFromString(const std::string& value)
    {
        if (value == "heal") return SkillEffectType::Heal;
        if (value == "attack_modifier") return SkillEffectType::AttackModifier;
        if (value == "defense_modifier") return SkillEffectType::DefenseModifier;
        if (value == "attack_penetration_modifier") return SkillEffectType::AttackPenetrationModifier;
        if (value == "cooldown_modifier") return SkillEffectType::CooldownModifier;
        if (value == "healing_received_modifier") return SkillEffectType::HealingReceivedModifier;
        if (value == "stunned") return SkillEffectType::Stunned;
        if (value == "silenced") return SkillEffectType::Silenced;
        if (value == "rooted") return SkillEffectType::Rooted;
        if (value == "mark") return SkillEffectType::Mark;
        if (value == "none" || value.empty()) return SkillEffectType::None;
        throw std::runtime_error("Unknown skill effect type: " + value);
    }

    SkillEffectTarget SkillEffectTargetFromString(const std::string& value)
    {
        if (value == "self") return SkillEffectTarget::Self;
        if (value == "ally") return SkillEffectTarget::Ally;
        if (value == "enemy") return SkillEffectTarget::Enemy;
        if (value == "player_lineup") return SkillEffectTarget::PlayerLineup;
        if (value == "opponent") return SkillEffectTarget::Opponent;
        throw std::runtime_error("Unknown skill effect target: " + value);
    }

    Spec SpecFromString(const std::string& value)
    {
        if (value == "Top" || value == "top") return Spec::Top;
        if (value == "Jungle" || value == "jungle") return Spec::Jungle;
        if (value == "Mid" || value == "mid") return Spec::Mid;
        if (value == "ADC" || value == "Adc" || value == "adc") return Spec::Adc;
        if (value == "Support" || value == "support") return Spec::Support;
        throw std::runtime_error("Unknown spec: " + value);
    }

    GameType GameTypeFromString(const std::string& value)
    {
        if (value == "League of Legends" || value == "league_of_legends")
        {
            return GameType::LeagueOfLegends;
        }

        throw std::runtime_error("Unknown game type: " + value);
    }

    TraitEffectType TraitEffectTypeFromString(const std::string& value)
    {
        if (value == "none" || value.empty()) return TraitEffectType::None;
        if (value == "low_hp_mana_discount_percent") return TraitEffectType::LowHpManaDiscountPercent;
        if (value == "setup_disrupt_effect_bonus_percent") return TraitEffectType::SetupDisruptEffectBonusPercent;
        if (value == "super_effective_damage_bonus_percent") return TraitEffectType::SuperEffectiveDamageBonusPercent;
        if (value == "damaging_accuracy_bonus_percent") return TraitEffectType::DamagingAccuracyBonusPercent;
        if (value == "positive_effect_bonus_percent") return TraitEffectType::PositiveEffectBonusPercent;
        throw std::runtime_error("Unknown trait effect type: " + value);
    }

    std::vector<SkillEffectDefinition> ParseEffects(const std::string& value)
    {
        std::vector<SkillEffectDefinition> effects;
        if (value.empty() || value == "none")
        {
            return effects;
        }

        for (const std::string& effect_text : Split(value, ';'))
        {
            if (effect_text.empty())
            {
                continue;
            }

            const std::vector<std::string> fields = Split(effect_text, ':');
            if (fields.size() != 5)
            {
                throw std::runtime_error("Expected effect format type:target:value:duration:mark, got: " + effect_text);
            }

            effects.push_back({
                SkillEffectTypeFromString(fields[0]),
                SkillEffectTargetFromString(fields[1]),
                std::stoi(fields[2]),
                std::stoi(fields[3]),
                std::stoi(fields[4]),
            });
        }

        return effects;
    }

    bool FileExists(const std::string& path)
    {
        std::ifstream input(path);
        return input.good();
    }

    SkillTone GetSkillTone(const Skill& skill)
    {
        if (skill.cooldownTurns >= 5)
        {
            return SkillTone::Risky;
        }

        if (skill.effectType == SkillEffectType::Heal
            || skill.effectType == SkillEffectType::AttackModifier
            || skill.effectType == SkillEffectType::AttackPenetrationModifier
            || skill.effectType == SkillEffectType::CooldownModifier
            || skill.effectType == SkillEffectType::HealingReceivedModifier
            || skill.effectType == SkillEffectType::Stunned
            || skill.effectType == SkillEffectType::Silenced
            || skill.effectType == SkillEffectType::Rooted
            || skill.effectType == SkillEffectType::Mark
            || (skill.effectType == SkillEffectType::DefenseModifier && skill.effectValue > 0))
        {
            return SkillTone::Utility;
        }

        if (skill.power >= 30)
        {
            return SkillTone::Pressure;
        }

        if (skill.power > 0)
        {
            return SkillTone::Reliable;
        }

        return SkillTone::Basic;
    }

    std::string CreateSkillDescription(const Skill& skill)
    {
        if (!skill.description.empty())
        {
            return skill.description;
        }

        if (skill.effectType == SkillEffectType::Heal)
        {
            return "Recover HP and stay in the set longer.";
        }

        if (skill.effectType == SkillEffectType::AttackModifier)
        {
            if (skill.effectValue >= 0)
            {
                if (skill.effectTarget == SkillEffectTarget::PlayerLineup)
                {
                    return "Boost the lineup's attacks for a short window.";
                }
                if (skill.effectTarget == SkillEffectTarget::Ally)
                {
                    return "Boost an ally's attacks for a short window.";
                }
                return "Set up a stronger follow-up play.";
            }
            return "Disrupt the opponent's next attacks.";
        }

        if (skill.effectType == SkillEffectType::AttackPenetrationModifier)
        {
            return "Prepare attacks that cut through enemy defenses.";
        }

        if (skill.effectType == SkillEffectType::CooldownModifier)
        {
            return skill.effectTarget == SkillEffectTarget::Self
                ? "Speed up your next ability cycle."
                : "Slow the opponent's next ability cycle.";
        }

        if (skill.effectType == SkillEffectType::HealingReceivedModifier)
        {
            return "Reduce the opponent's recovery window.";
        }

        if (skill.effectType == SkillEffectType::Stunned)
        {
            return "Stun the opponent and deny their next action.";
        }

        if (skill.effectType == SkillEffectType::Silenced)
        {
            return "Silence the opponent's non-basic abilities.";
        }

        if (skill.effectType == SkillEffectType::Rooted)
        {
            return "Root the opponent and prevent switching.";
        }

        if (skill.effectType == SkillEffectType::Mark)
        {
            return "Mark the opponent for a high-damage follow-up hit.";
        }

        if (skill.effectType == SkillEffectType::DefenseModifier)
        {
            return skill.effectValue < 0
                ? "A risky play that hits hard but leaves you exposed."
                : "Reduce incoming pressure for a few hits.";
        }

        if (skill.power >= 30)
        {
            return "Push tempo with a high-pressure offensive play.";
        }

        if (skill.power > 0)
        {
            return "Trade consistently without overcommitting.";
        }

        return "A reliable basic play.";
    }

    void FillSkillMetadata(std::vector<Skill>& skills)
    {
        for (Skill& skill : skills)
        {
            skill.tone = GetSkillTone(skill);
            skill.description = CreateSkillDescription(skill);
            if (skill.sourceSpec.empty())
            {
                skill.sourceSpec = InferSkillSourceSpec(skill.id);
            }
            if (skill.colorId.empty())
            {
                skill.colorId = skill.tone == SkillTone::Basic
                    ? "neutral"
                    : skill.sourceSpec;
            }
        }
    }
}

SimulationData::SimulationData()
{
    // SAMPLE COMBAT DATA:
    // Row format:
    // ID, name, mana cost, mana gain, cooldown, hidden power, accuracy,
    // optional effect, target, effect value, duration, mark bonus.
    skills_ = {
        MakeSkill("top-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("top-hold-line", "Stomp", 35, 0, 2, 0, 1.00, SkillEffectType::Rooted, SkillEffectTarget::Enemy, 0, 1),
        MakeSkill("top-sunder", "Purge", 45, 0, 2, 28, 0.95, SkillEffectType::AttackPenetrationModifier, SkillEffectTarget::Self, 35, 2),
        MakeSkill("top-gamebreaker", "Unbreakable", 100, 0, 5, 28, 0.90, SkillEffectType::DefenseModifier, SkillEffectTarget::Self, 30, 2),

        MakeSkill("jungle-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("jungle-gank", "Gank", 35, 0, 2, 14, 0.95, SkillEffectType::Stunned, SkillEffectTarget::Enemy, 0, 1),
        MakeSkill("jungle-invade", "Invade", 45, 0, 2, 30, 0.95, SkillEffectType::CooldownModifier, SkillEffectTarget::Enemy, 50, 2),
        MakeSkill("jungle-smite-fight", "Decimate", 100, 0, 5, 62, 0.90, SkillEffectType::Mark, SkillEffectTarget::Enemy, 0, 2, 28),

        MakeSkill("mid-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("mid-silence", "Light binding", 35, 0, 2, 12, 0.95, SkillEffectType::Silenced, SkillEffectTarget::Enemy, 0, 1),
        MakeSkill("mid-burst", "Spark", 45, 0, 2, 34, 0.92, SkillEffectType::AttackModifier, SkillEffectTarget::Enemy, -30, 2),
        MakeSkill("mid-ultimate", "Sonic wave", 100, 0, 5, 65, 0.88, SkillEffectType::CooldownModifier, SkillEffectTarget::Self, -30, 2),

        MakeSkill("adc-basic", "Basic Attack", 0, Balance::BasicManaGain, 0, 16, 0.95),
        MakeSkill("adc-trap", "Place Trap", 35, 0, 2, 8, 1.00, SkillEffectType::Stunned, SkillEffectTarget::Enemy, 0, 2),
        MakeSkill("adc-multi-strike", "Multi Strike", 45, 0, 2, 36, 0.94),
        MakeSkill("adc-bullet-time", "Bullet Time", 100, 0, 5, 72, 0.88),

        MakeSkill("support-basic", "Basic attack", 0, Balance::BasicManaGain, 0, 14, 0.95),
        MakeSkill("support-peel", "Peel", 35, 0, 2, 0, 1.00, SkillEffectType::Heal, SkillEffectTarget::Ally, 40, 0),
        MakeSkill("support-shotcall", "Shotcall", 45, 0, 2, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::Ally, 25, 2),
        MakeSkill("support-teamfight", "Teamfight plan", 100, 0, 5, 0, 1.00, SkillEffectType::AttackModifier, SkillEffectTarget::PlayerLineup, 10, 2)
    };
    FillSkillMetadata(skills_);

    drills_ = {
        MakeDrill(
            GameType::LeagueOfLegends,
            "moba-farm",
            "Farm",
            "Take a timing challenge to generate mana instead of attacking.",
            5,
            30,
            50)
    };

    traits_ = {
        MakeTrait(
            "clutch-player",
            "Clutch Player",
            "Below 35% HP, paid abilities cost 25% less mana.",
            TraitEffectType::LowHpManaDiscountPercent,
            25),
        MakeTrait(
            "shotcaller",
            "Shotcaller",
            "Setup and disruption effects are 20% stronger.",
            TraitEffectType::SetupDisruptEffectBonusPercent,
            20),
        MakeTrait(
            "lane-bully",
            "Lane Bully",
            "Super-effective hits deal 15% more damage.",
            TraitEffectType::SuperEffectiveDamageBonusPercent,
            15),
        MakeTrait(
            "precision-carry",
            "Precision Carry",
            "Damaging skills gain 5% accuracy.",
            TraitEffectType::DamagingAccuracyBonusPercent,
            5),
        MakeTrait(
            "stabilizer",
            "Stabilizer",
            "Healing and defensive effects are 20% stronger.",
            TraitEffectType::PositiveEffectBonusPercent,
            20)
    };

    // Edit counteredSpec values to change the matchup cycle.
    // Current rule: Top > Jungle > Mid > ADC > Support > Top.
    specs_ = {
        { Spec::Top, "Top", { "top-basic", "top-hold-line", "top-sunder", "top-gamebreaker" }, Spec::Jungle, "clutch-player", 4, 0.20 },
        { Spec::Jungle, "Jungle", { "jungle-basic", "jungle-gank", "jungle-invade", "jungle-smite-fight" }, Spec::Mid, "shotcaller", 2, 0.35 },
        { Spec::Mid, "Mid", { "mid-basic", "mid-silence", "mid-burst", "mid-ultimate" }, Spec::Adc, "lane-bully", 1, 0.45 },
        { Spec::Adc, "ADC", { "adc-basic", "adc-trap", "adc-multi-strike", "adc-bullet-time" }, Spec::Support, "precision-carry", 1, 0.50 },
        { Spec::Support, "Support", { "support-basic", "support-peel", "support-shotcall", "support-teamfight" }, Spec::Top, "stabilizer", 3, 0.25 }
    };
    LoadExternalDataIfAvailable();
    BuildIndexes();
    ValidateReferences();
}

const Skill* SimulationData::FindSkill(const std::string& id) const
{
    const auto found = skillIndexById_.find(id);
    if (found == skillIndexById_.end())
    {
        return nullptr;
    }

    return &skills_[found->second];
}

const DrillDefinition* SimulationData::FindDrill(GameType gameType) const
{
    const auto found = drillIndexByGameType_.find(static_cast<int>(gameType));
    if (found == drillIndexByGameType_.end())
    {
        return nullptr;
    }

    return &drills_[found->second];
}

const TraitDefinition* SimulationData::FindTrait(const std::string& id) const
{
    const auto found = traitIndexById_.find(id);
    if (found == traitIndexById_.end())
    {
        return nullptr;
    }

    return &traits_[found->second];
}

const SpecData* SimulationData::FindSpec(Spec spec) const
{
    const auto found = specIndexBySpec_.find(static_cast<int>(spec));
    if (found == specIndexBySpec_.end())
    {
        return nullptr;
    }

    return &specs_[found->second];
}

const std::vector<SpecData>& SimulationData::GetSpecs() const
{
    return specs_;
}

bool SimulationData::LoadedExternalSkills() const
{
    return loadedExternalSkills_;
}

bool SimulationData::LoadedExternalTraits() const
{
    return loadedExternalTraits_;
}

bool SimulationData::LoadedExternalSpecs() const
{
    return loadedExternalSpecs_;
}

bool SimulationData::LoadedExternalDrills() const
{
    return loadedExternalDrills_;
}

void SimulationData::LoadExternalDataIfAvailable()
{
    if (FileExists("poke-clone-esports-ui/data/skills.csv")) LoadSkillsFromCsv("poke-clone-esports-ui/data/skills.csv");
    else if (FileExists("data/skills.csv")) LoadSkillsFromCsv("data/skills.csv");

    if (FileExists("poke-clone-esports-ui/data/traits.csv")) LoadTraitsFromCsv("poke-clone-esports-ui/data/traits.csv");
    else if (FileExists("data/traits.csv")) LoadTraitsFromCsv("data/traits.csv");

    if (FileExists("poke-clone-esports-ui/data/drills.csv")) LoadDrillsFromCsv("poke-clone-esports-ui/data/drills.csv");
    else if (FileExists("data/drills.csv")) LoadDrillsFromCsv("data/drills.csv");

    if (FileExists("poke-clone-esports-ui/data/specs.csv")) LoadSpecsFromCsv("poke-clone-esports-ui/data/specs.csv");
    else if (FileExists("data/specs.csv")) LoadSpecsFromCsv("data/specs.csv");
}

void SimulationData::LoadSkillsFromCsv(const std::string& path)
{
    std::ifstream input(path);
    if (!input.good())
    {
        return;
    }

    std::vector<Skill> loaded_skills;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line))
    {
        ++line_number;
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::vector<std::string> fields = ParseCsvRow(line);
        if (IsHeaderRow(fields, "id"))
        {
            continue;
        }
        if (fields.size() != 8 && fields.size() != 10)
        {
            throw std::runtime_error("Invalid skill row in " + path + " at line " + std::to_string(line_number));
        }

        Skill skill = MakeSkill(
            fields[0],
            fields[1],
            std::stoi(fields[2]),
            std::stoi(fields[3]),
            std::stoi(fields[4]),
            std::stoi(fields[5]),
            std::stod(fields[6]));
        skill.effects = ParseEffects(fields[7]);
        if (!skill.effects.empty())
        {
            const SkillEffectDefinition& primary_effect = skill.effects[0];
            skill.effectType = primary_effect.type;
            skill.effectTarget = primary_effect.target;
            skill.effectValue = primary_effect.value;
            skill.durationTurns = primary_effect.durationTurns;
            skill.markBonusDamage = primary_effect.markBonusDamage;
        }
        if (fields.size() == 10)
        {
            skill.sourceSpec = fields[8];
            skill.colorId = fields[9];
        }
        loaded_skills.push_back(skill);
    }

    if (loaded_skills.empty())
    {
        throw std::runtime_error("No skills loaded from " + path);
    }

    FillSkillMetadata(loaded_skills);
    skills_ = loaded_skills;
    loadedExternalSkills_ = true;
}

void SimulationData::LoadTraitsFromCsv(const std::string& path)
{
    std::ifstream input(path);
    if (!input.good())
    {
        return;
    }

    std::vector<TraitDefinition> loaded_traits;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line))
    {
        ++line_number;
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::vector<std::string> fields = ParseCsvRow(line);
        if (IsHeaderRow(fields, "id"))
        {
            continue;
        }
        if (fields.size() != 5)
        {
            throw std::runtime_error("Invalid trait row in " + path + " at line " + std::to_string(line_number));
        }

        loaded_traits.push_back(MakeTrait(
            fields[0],
            fields[1],
            fields[2],
            TraitEffectTypeFromString(fields[3]),
            std::stoi(fields[4])));
    }

    if (loaded_traits.empty())
    {
        throw std::runtime_error("No traits loaded from " + path);
    }

    traits_ = loaded_traits;
    loadedExternalTraits_ = true;
}

void SimulationData::LoadSpecsFromCsv(const std::string& path)
{
    std::ifstream input(path);
    if (!input.good())
    {
        return;
    }

    std::vector<SpecData> loaded_specs;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line))
    {
        ++line_number;
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::vector<std::string> fields = ParseCsvRow(line);
        if (IsHeaderRow(fields, "spec"))
        {
            continue;
        }
        if (fields.size() != 7)
        {
            throw std::runtime_error("Invalid spec row in " + path + " at line " + std::to_string(line_number));
        }

        loaded_specs.push_back({
            SpecFromString(fields[0]),
            fields[1],
            Split(fields[2], '|'),
            SpecFromString(fields[3]),
            fields[4],
            std::stoi(fields[5]),
            std::stod(fields[6]),
        });
    }

    if (loaded_specs.empty())
    {
        throw std::runtime_error("No specs loaded from " + path);
    }

    specs_ = loaded_specs;
    loadedExternalSpecs_ = true;
}

void SimulationData::LoadDrillsFromCsv(const std::string& path)
{
    std::ifstream input(path);
    if (!input.good())
    {
        return;
    }

    std::vector<DrillDefinition> loaded_drills;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line))
    {
        ++line_number;
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::vector<std::string> fields = ParseCsvRow(line);
        if (IsHeaderRow(fields, "game_type"))
        {
            continue;
        }
        if (fields.size() != 7)
        {
            throw std::runtime_error("Invalid drill row in " + path + " at line " + std::to_string(line_number));
        }

        loaded_drills.push_back(MakeDrill(
            GameTypeFromString(fields[0]),
            fields[1],
            fields[2],
            fields[3],
            std::stoi(fields[4]),
            std::stoi(fields[5]),
            std::stoi(fields[6])));
    }

    if (loaded_drills.empty())
    {
        throw std::runtime_error("No drills loaded from " + path);
    }

    drills_ = loaded_drills;
    loadedExternalDrills_ = true;
}

void SimulationData::BuildIndexes()
{
    skillIndexById_.clear();
    for (std::size_t index = 0; index < skills_.size(); ++index)
    {
        const auto inserted = skillIndexById_.emplace(skills_[index].id, index);
        if (!inserted.second)
        {
            throw std::runtime_error("Duplicate skill id: " + skills_[index].id);
        }
    }

    drillIndexByGameType_.clear();
    for (std::size_t index = 0; index < drills_.size(); ++index)
    {
        const auto inserted = drillIndexByGameType_.emplace(static_cast<int>(drills_[index].gameType), index);
        if (!inserted.second)
        {
            throw std::runtime_error("Duplicate drill game type: " + ToString(drills_[index].gameType));
        }
    }

    traitIndexById_.clear();
    for (std::size_t index = 0; index < traits_.size(); ++index)
    {
        const auto inserted = traitIndexById_.emplace(traits_[index].id, index);
        if (!inserted.second)
        {
            throw std::runtime_error("Duplicate trait id: " + traits_[index].id);
        }
    }

    specIndexBySpec_.clear();
    for (std::size_t index = 0; index < specs_.size(); ++index)
    {
        const auto inserted = specIndexBySpec_.emplace(static_cast<int>(specs_[index].spec), index);
        if (!inserted.second)
        {
            throw std::runtime_error("Duplicate spec: " + ToString(specs_[index].spec));
        }
    }
}

void SimulationData::ValidateReferences() const
{
    for (const SpecData& spec : specs_)
    {
        if (FindTrait(spec.defaultTraitId) == nullptr)
        {
            throw std::runtime_error("Spec references missing default trait: " + spec.defaultTraitId);
        }

        for (const std::string& skill_id : spec.skillIds)
        {
            if (FindSkill(skill_id) == nullptr)
            {
                throw std::runtime_error("Spec references missing skill: " + skill_id);
            }
        }
    }
}
