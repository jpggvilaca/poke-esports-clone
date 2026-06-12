class_name MapNpcController
extends Node2D

var npc_frame_timer := 0.0
var npc_base_rows: Dictionary = {}


func setup() -> void:
	_cache_npc_directions()
	refresh_defeated_states()


func tick_animation(delta: float, player_position: Vector2) -> void:
	npc_frame_timer += delta
	var npc_index := 0
	for npc in get_children():
		if not _is_battle_npc(npc):
			continue
		if not GameState.is_npc_available(String(npc.name)):
			continue

		var sprite := npc.get_node_or_null("Sprite")
		if not (sprite is Sprite2D):
			continue

		var npc_id := String(npc.name)
		var row := int(npc_base_rows.get(npc_id, sprite.frame_coords.y))
		if not GameState.is_npc_defeated(npc_id):
			var to_player = player_position - npc.global_position
			if to_player.length() <= MapInteractionController.INTERACT_DISTANCE:
				row = _direction_row(to_player)
			sprite.frame_coords = Vector2i((int(npc_frame_timer * 4.0) + npc_index) % 4, row)
		else:
			sprite.frame_coords = Vector2i(0, row)

		npc_index += 1


func refresh_defeated_states() -> void:
	for npc in get_children():
		if not _is_battle_npc(npc):
			continue

		var npc_id := String(npc.name)
		var available := GameState.is_npc_available(npc_id)
		npc.visible = available
		var collision := npc.get_node_or_null("Collision")
		if collision is CollisionShape2D:
			collision.disabled = not available
		if not available:
			continue

		var defeated := GameState.is_npc_defeated(npc_id)
		var sprite := npc.get_node_or_null("Sprite")
		if sprite is CanvasItem:
			sprite.modulate = Color(0.55, 0.55, 0.55, 0.85) if defeated else Color.WHITE

		var label := npc.get_node_or_null("%sLabel" % npc.name)
		if label is Label:
			label.text = get_npc_label_text(npc, defeated)


func find_nearest_battle_npc(player_position: Vector2, max_distance: float) -> Node2D:
	var nearest_npc: Node2D
	var nearest_distance := max_distance + 1.0
	for npc in get_children():
		if not _is_battle_npc(npc):
			continue
		if not GameState.is_npc_available(String(npc.name)):
			continue
		if GameState.is_npc_defeated(String(npc.name)):
			continue

		var distance := player_position.distance_to(npc.global_position)
		if distance <= max_distance and distance < nearest_distance:
			nearest_distance = distance
			nearest_npc = npc

	return nearest_npc


func get_npc(npc_id: String) -> Node2D:
	var npc := get_node_or_null(npc_id)
	return npc if npc is Node2D else null


func get_npc_display_name(npc: Node) -> String:
	var fallback_name := GameState.get_npc_display_name(String(npc.name))
	var profile := npc as MapNpcProfile
	if profile != null:
		return profile.get_display_name(fallback_name)
	return fallback_name


func get_npc_battle_overrides(npc: Node) -> Dictionary:
	var profile := npc as MapNpcProfile
	if profile == null:
		return {}
	return profile.get_battle_overrides(GameState.get_npc_display_name(String(npc.name)))


func get_npc_label_text(npc: Node, defeated := false) -> String:
	var name := get_npc_display_name(npc)
	var label_text := name
	if defeated:
		label_text += "\nDefeated"
	return label_text


func _cache_npc_directions() -> void:
	for npc in get_children():
		if not _is_battle_npc(npc):
			continue
		var sprite := npc.get_node_or_null("Sprite")
		if sprite is Sprite2D:
			npc_base_rows[String(npc.name)] = sprite.frame_coords.y


func _is_battle_npc(node: Node) -> bool:
	return node is Node2D and node.is_in_group("battle_npc")


func _direction_row(direction: Vector2) -> int:
	if abs(direction.x) > abs(direction.y):
		return 2 if direction.x > 0.0 else 1
	if direction.y != 0.0:
		return 0 if direction.y > 0.0 else 3
	return 0
