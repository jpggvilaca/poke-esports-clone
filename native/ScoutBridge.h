#pragma once

#include "SimulationData.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

class ScoutBridge : public godot::Node
{
    GDCLASS(ScoutBridge, godot::Node)

public:
    godot::Dictionary get_pending_scout_offer(
        int rating,
        const godot::Array& completed_offer_ids,
        const godot::Array& declined_offer_ids) const;
    godot::Dictionary accept_scout_candidate(
        const godot::Array& roster,
        const godot::Dictionary& pending_offer,
        int candidate_index) const;

protected:
    static void _bind_methods();

private:
    SimulationData data_;
};
