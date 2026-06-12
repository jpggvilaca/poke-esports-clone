class_name BattleEventPlayer
extends Node

var message_log: BattleMessageLog
var player_status: BattleStatusPanel
var opponent_status: BattleStatusPanel
var reward_presenter: BattleRewardPresenter
var current_state: Dictionary = {}


func setup(next_message_log: BattleMessageLog, next_player_status: BattleStatusPanel, next_opponent_status: BattleStatusPanel, next_reward_presenter: BattleRewardPresenter) -> void:
	message_log = next_message_log
	player_status = next_player_status
	opponent_status = next_opponent_status
	reward_presenter = next_reward_presenter


func play_events(events: Array, state: Dictionary) -> void:
	current_state = state
	for event in events:
		_apply_event(event)
		await get_tree().create_timer(0.45).timeout


func _apply_event(event: Dictionary) -> void:
	var type := String(event.get("type", "none"))
	match type:
		"battle_started":
			message_log.push_log("Battle start!")
			message_log.set_message("%s wants to battle." % opponent_status.get_display_name())
		"player_switched":
			if String(event.get("actor", "")) == "opponent":
				message_log.set_message("Opponent sends out %s." % event.get("player_name", "opponent"))
			elif String(event.get("reason", "")) == "turn":
				message_log.set_message("Next up: %s." % event.get("player_name", "player"))
			else:
				message_log.set_message("Switched to %s." % event.get("player_name", "player"))
			message_log.push_log(message_log.get_message())
		"skill_started":
			var actor := _display_event_actor(event)
			message_log.set_message("%s used %s." % [actor, BattleRewardPresenter.format_skill_id(String(event.get("skill_id", "a skill")))])
			message_log.push_log(message_log.get_message())
		"drill_started":
			var actor := _display_actor(String(event.get("actor", "none")))
			var drill_name := String(event.get("reason", "Drill"))
			message_log.set_message("%s chose %s." % [actor, drill_name])
			message_log.push_log(message_log.get_message())
		"drill_completed":
			var actor := _display_actor(String(event.get("actor", "none")))
			message_log.push_log("%s drill result: %s (+%s mana)." % [
				actor,
				String(event.get("reason", "good")).capitalize(),
				event.get("amount", 0),
			])
		"mana_changed":
			_animate_mana(String(event.get("actor", "none")), int(event.get("old_value", 0)), int(event.get("new_value", 0)), int(event.get("actor_player_index", -1)))
			message_log.push_log("%s mana %s -> %s." % [_display_event_actor(event), event.get("old_value", 0), event.get("new_value", 0)])
		"cooldown_started":
			message_log.push_log("%s cooldown: %s turn(s)." % [BattleRewardPresenter.format_skill_id(String(event.get("skill_id", ""))), event.get("new_value", 0)])
		"cooldown_ready":
			message_log.push_log("%s is ready." % BattleRewardPresenter.format_skill_id(String(event.get("skill_id", ""))))
		"action_blocked":
			message_log.push_log("%s could not act: %s." % [_display_actor(String(event.get("actor", "none"))), event.get("reason", "blocked")])
		"attack_missed":
			message_log.set_message("It missed.")
			message_log.push_log("The play missed.")
		"damage_applied":
			_animate_hp(String(event.get("target", "none")), int(event.get("old_value", 0)), int(event.get("new_value", 0)), int(event.get("target_player_index", -1)))
			message_log.push_log("%s took %s damage." % [_display_event_target(event), event.get("amount", 0)])
		"healing_applied":
			_animate_hp(String(event.get("target", "none")), int(event.get("old_value", 0)), int(event.get("new_value", 0)), int(event.get("target_player_index", -1)))
			message_log.push_log("%s recovered %s HP." % [_display_event_target(event), event.get("amount", 0)])
		"status_applied":
			message_log.push_log(_format_status_log(event))
		"status_expired":
			message_log.push_log("%s status expired." % _display_event_target(event))
		"mark_applied":
			message_log.push_log("%s was marked." % _display_event_target(event))
		"mark_triggered":
			message_log.push_log("Mark triggered for %s bonus damage." % event.get("amount", 0))
		"mark_expired":
			message_log.push_log("%s mark expired." % _display_event_target(event))
		"farming_triggered":
			message_log.push_log("%s: lineup gained up to %s mana." % [event.get("reason", "Farm secured"), event.get("amount", 0)])
		"skill_xp_gained":
			reward_presenter.defer_progress_event(event)
		"skill_leveled_up":
			reward_presenter.defer_progress_event(event)
		"battle_finished":
			message_log.set_message("The battle is over.")
			message_log.push_log("Winner: %s" % event.get("winner", "none"))
		"reward_granted":
			var reward: Dictionary = event.get("reward", {})
			message_log.push_log("Team earned %s battle XP." % reward.get("total_xp", 0))


func _animate_hp(actor: String, old_value: int, new_value: int, player_index: int = -1) -> void:
	if actor == "player" and player_index != int(current_state.get("active_player_index", 0)):
		return
	if actor == "opponent":
		opponent_status.animate_hp(old_value, new_value)
	else:
		player_status.animate_hp(old_value, new_value)


func _animate_mana(actor: String, old_value: int, new_value: int, player_index: int = -1) -> void:
	if actor == "player" and player_index != int(current_state.get("active_player_index", 0)):
		return
	if actor == "opponent":
		opponent_status.animate_mana(old_value, new_value)
	else:
		player_status.animate_mana(old_value, new_value)


func _display_actor(actor: String) -> String:
	if actor == "player":
		return "Your player"
	if actor == "opponent":
		return "Opponent"
	return "Someone"


func _display_event_actor(event: Dictionary) -> String:
	var actor_name := String(event.get("actor_name", ""))
	if not actor_name.is_empty():
		return actor_name
	return _display_actor(String(event.get("actor", "none")))


func _display_event_target(event: Dictionary) -> String:
	var target_name := String(event.get("target_name", ""))
	if not target_name.is_empty():
		return target_name
	return _display_actor(String(event.get("target", "none")))


func _format_status_log(event: Dictionary) -> String:
	var actor := _display_event_actor(event)
	var target := _display_event_target(event)
	var amount := int(event.get("amount", 0))
	match String(event.get("effect", "none")):
		"attack_modifier":
			if amount >= 0:
				return "%s boosted attack pressure by %s%%." % [target, amount]
			return "%s attack pressure fell by %s%%." % [target, abs(amount)]
		"defense_modifier":
			if amount >= 0:
				return "%s reduced incoming damage by %s%%." % [target, amount]
			return "%s became exposed and takes %s%% more damage." % [target, abs(amount)]
		"attack_penetration_modifier":
			return "%s gained %s%% attack penetration." % [target, amount]
		"cooldown_modifier":
			if amount >= 0:
				return "%s cooldowns increased by %s%%." % [target, amount]
			return "%s cooldowns reduced by %s%%." % [target, abs(amount)]
		"healing_received_modifier":
			return "%s healing changed by %s%%." % [target, amount]
		"stunned":
			return "%s is stunned." % target
		"silenced":
			return "%s is silenced." % target
		"rooted":
			return "%s is rooted." % target
	return "%s changed %s's battle state." % [actor, target]
