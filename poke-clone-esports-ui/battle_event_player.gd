class_name BattleEventPlayer
extends Node

var message_log: BattleMessageLog
var player_status: BattleStatusPanel
var opponent_status: BattleStatusPanel
var player_float_anchor: Control
var opponent_float_anchor: Control
var reward_presenter: BattleRewardPresenter
var current_state: Dictionary = {}


func setup(next_message_log: BattleMessageLog, next_player_status: BattleStatusPanel, next_opponent_status: BattleStatusPanel, next_reward_presenter: BattleRewardPresenter, next_player_float_anchor: Control = null, next_opponent_float_anchor: Control = null) -> void:
	message_log = next_message_log
	player_status = next_player_status
	opponent_status = next_opponent_status
	reward_presenter = next_reward_presenter
	player_float_anchor = next_player_float_anchor
	opponent_float_anchor = next_opponent_float_anchor


func play_events(events: Array, state: Dictionary) -> void:
	current_state = state
	for event in events:
		_apply_event(event)
		await get_tree().create_timer(0.45).timeout


func _apply_event(event: Dictionary) -> void:
	var type := String(event.get("type", "none"))
	match type:
		BattleEventTypes.BATTLE_STARTED:
			message_log.push_log("Battle start!")
			message_log.set_message("%s wants to battle." % opponent_status.get_display_name())
		BattleEventTypes.PLAYER_SWITCHED:
			if String(event.get("actor", "")) == "opponent":
				message_log.set_message("Opponent sends out %s." % event.get("player_name", "opponent"))
			elif String(event.get("reason", "")) == "turn":
				message_log.set_message("Next up: %s." % event.get("player_name", "player"))
			else:
				message_log.set_message("Switched to %s." % event.get("player_name", "player"))
			message_log.push_log(message_log.get_message())
		BattleEventTypes.SKILL_STARTED:
			var actor := _display_event_actor(event)
			message_log.set_message("%s used %s." % [actor, BattleRewardPresenter.format_skill_id(String(event.get("skill_id", "a skill")))])
			message_log.push_log(message_log.get_message())
		BattleEventTypes.DRILL_STARTED:
			var actor := _display_actor(String(event.get("actor", "none")))
			var drill_name := String(event.get("reason", "Drill"))
			message_log.set_message("%s chose %s." % [actor, drill_name])
			message_log.push_log(message_log.get_message())
		BattleEventTypes.DRILL_COMPLETED:
			var actor := _display_actor(String(event.get("actor", "none")))
			message_log.push_log("%s drill result: %s (+%s mana)." % [
				actor,
				String(event.get("reason", "good")).capitalize(),
				event.get("amount", 0),
			])
		BattleEventTypes.MANA_CHANGED:
			_animate_mana(String(event.get("actor", "none")), int(event.get("old_value", 0)), int(event.get("new_value", 0)), int(event.get("actor_player_index", -1)))
			message_log.push_log("%s mana %s -> %s." % [_display_event_actor(event), event.get("old_value", 0), event.get("new_value", 0)])
		BattleEventTypes.COOLDOWN_STARTED:
			message_log.push_log("%s cooldown: %s turn(s)." % [BattleRewardPresenter.format_skill_id(String(event.get("skill_id", ""))), event.get("new_value", 0)])
		BattleEventTypes.COOLDOWN_READY:
			message_log.push_log("%s is ready." % BattleRewardPresenter.format_skill_id(String(event.get("skill_id", ""))))
		BattleEventTypes.ACTION_BLOCKED:
			message_log.push_log("%s could not act: %s." % [_display_actor(String(event.get("actor", "none"))), event.get("reason", "blocked")])
		BattleEventTypes.ATTACK_MISSED:
			message_log.set_message("It missed.")
			message_log.push_log("The play missed.")
		BattleEventTypes.DAMAGE_APPLIED:
			_animate_hp(String(event.get("target", "none")), int(event.get("old_value", 0)), int(event.get("new_value", 0)), int(event.get("target_player_index", -1)))
			_show_floating_number(String(event.get("target", "none")), int(event.get("amount", 0)), false, int(event.get("target_player_index", -1)))
			message_log.push_log("%s took %s damage." % [_display_event_target(event), event.get("amount", 0)])
		BattleEventTypes.HEALING_APPLIED:
			_animate_hp(String(event.get("target", "none")), int(event.get("old_value", 0)), int(event.get("new_value", 0)), int(event.get("target_player_index", -1)))
			_show_floating_number(String(event.get("target", "none")), int(event.get("amount", 0)), true, int(event.get("target_player_index", -1)))
			message_log.push_log("%s recovered %s HP." % [_display_event_target(event), event.get("amount", 0)])
		BattleEventTypes.STATUS_APPLIED:
			message_log.push_log(_format_status_log(event))
		BattleEventTypes.STATUS_EXPIRED:
			message_log.push_log("%s status expired." % _display_event_target(event))
		BattleEventTypes.MARK_APPLIED:
			message_log.push_log("%s was marked." % _display_event_target(event))
		BattleEventTypes.MARK_TRIGGERED:
			message_log.push_log("Mark triggered for %s bonus damage." % event.get("amount", 0))
		BattleEventTypes.MARK_EXPIRED:
			message_log.push_log("%s mark expired." % _display_event_target(event))
		BattleEventTypes.FARMING_TRIGGERED:
			message_log.push_log("%s: lineup gained up to %s mana." % [event.get("reason", "Farm secured"), event.get("amount", 0)])
		BattleEventTypes.SKILL_XP_GAINED:
			reward_presenter.defer_progress_event(event)
		BattleEventTypes.SKILL_LEVELED_UP:
			reward_presenter.defer_progress_event(event)
		BattleEventTypes.BATTLE_FINISHED:
			message_log.set_message("The battle is over.")
			message_log.push_log("Winner: %s" % event.get("winner", "none"))
		BattleEventTypes.REWARD_GRANTED:
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


func _show_floating_number(actor: String, amount: int, is_healing: bool, player_index: int = -1) -> void:
	if amount <= 0:
		return
	if actor != "player" and actor != "opponent":
		return
	if actor == "player" and player_index != int(current_state.get("active_player_index", 0)):
		return

	var anchor := opponent_float_anchor if actor == "opponent" else player_float_anchor
	if anchor == null:
		return

	var label := Label.new()
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	label.text = "+%s" % amount if is_healing else "-%s" % amount
	label.z_index = 100
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.add_theme_font_size_override("font_size", 52)
	label.add_theme_color_override("font_color", Color(0.36, 1.0, 0.44, 1.0) if is_healing else Color(1.0, 0.28, 0.08, 1.0))
	label.add_theme_color_override("font_outline_color", Color(0.02, 0.015, 0.01, 1.0))
	label.add_theme_constant_override("outline_size", 8)
	anchor.add_child(label)

	var label_size := Vector2(150, 70)
	label.size = label_size
	label.position = Vector2((anchor.size.x - label_size.x) * 0.5, -38.0)
	label.scale = Vector2(0.75, 0.75)

	var tween := create_tween()
	tween.set_parallel(true)
	tween.tween_property(label, "position:y", label.position.y - 88.0, 0.85).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "modulate:a", 0.0, 0.85).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.tween_property(label, "scale", Vector2(1.22, 1.22), 0.16).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.chain().tween_callback(label.queue_free)


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
