extends Control

const FIELD_COLOR := Color(0.56, 0.72, 0.55)
const PANEL_COLOR := Color(0.94, 0.91, 0.78)
const PANEL_BORDER := Color(0.20, 0.20, 0.18)
const PLAYER_COLOR := Color(0.24, 0.43, 0.82)
const OPPONENT_COLOR := Color(0.78, 0.28, 0.24)
const LOG_LINES := 5

var bridge: BattleBridge
var opponent_name: Label
var opponent_meta: Label
var opponent_hp: ProgressBar
var opponent_focus: ProgressBar
var player_name: Label
var player_meta: Label
var player_hp: ProgressBar
var player_focus: ProgressBar
var message_label: Label
var log_label: Label
var skill_grid: GridContainer
var skill_buttons: Array[Button] = []
var log_entries: Array[String] = []
var input_locked := false


func _ready() -> void:
    _build_ui()
    bridge = BattleBridge.new()
    add_child(bridge)

    var result := bridge.start_demo_battle()
    _update_state(result.get("state", bridge.get_battle_state()))
    _refresh_skills()
    await _play_events(result.get("events", []))
    _set_message("What will your player do?")


func _build_ui() -> void:
    var background := ColorRect.new()
    background.color = FIELD_COLOR
    background.set_anchors_preset(Control.PRESET_FULL_RECT)
    add_child(background)

    var opponent_platform := ColorRect.new()
    opponent_platform.color = Color(0.78, 0.84, 0.62)
    opponent_platform.anchor_left = 0.58
    opponent_platform.anchor_top = 0.19
    opponent_platform.anchor_right = 0.91
    opponent_platform.anchor_bottom = 0.31
    add_child(opponent_platform)

    var player_platform := ColorRect.new()
    player_platform.color = Color(0.68, 0.80, 0.58)
    player_platform.anchor_left = 0.09
    player_platform.anchor_top = 0.57
    player_platform.anchor_right = 0.42
    player_platform.anchor_bottom = 0.70
    add_child(player_platform)

    var opponent_sprite := _make_battler_block(OPPONENT_COLOR, "OPPONENT")
    opponent_sprite.anchor_left = 0.68
    opponent_sprite.anchor_top = 0.08
    opponent_sprite.anchor_right = 0.82
    opponent_sprite.anchor_bottom = 0.25
    add_child(opponent_sprite)

    var player_sprite := _make_battler_block(PLAYER_COLOR, "PLAYER")
    player_sprite.anchor_left = 0.18
    player_sprite.anchor_top = 0.41
    player_sprite.anchor_right = 0.32
    player_sprite.anchor_bottom = 0.61
    add_child(player_sprite)

    var opponent_panel := _make_status_panel()
    opponent_panel.anchor_left = 0.07
    opponent_panel.anchor_top = 0.08
    opponent_panel.anchor_right = 0.43
    opponent_panel.anchor_bottom = 0.25
    add_child(opponent_panel)
    opponent_name = opponent_panel.get_node("Content/Name")
    opponent_meta = opponent_panel.get_node("Content/Meta")
    opponent_hp = opponent_panel.get_node("Content/Hp")
    opponent_focus = opponent_panel.get_node("Content/Focus")

    var player_panel := _make_status_panel()
    player_panel.anchor_left = 0.57
    player_panel.anchor_top = 0.45
    player_panel.anchor_right = 0.93
    player_panel.anchor_bottom = 0.63
    add_child(player_panel)
    player_name = player_panel.get_node("Content/Name")
    player_meta = player_panel.get_node("Content/Meta")
    player_hp = player_panel.get_node("Content/Hp")
    player_focus = player_panel.get_node("Content/Focus")

    var bottom := PanelContainer.new()
    bottom.anchor_left = 0.035
    bottom.anchor_top = 0.74
    bottom.anchor_right = 0.965
    bottom.anchor_bottom = 0.965
    bottom.add_theme_stylebox_override("panel", _panel_style(Color(0.93, 0.90, 0.76), PANEL_BORDER, 10))
    add_child(bottom)

    var bottom_split := HBoxContainer.new()
    bottom_split.add_theme_constant_override("separation", 22)
    bottom.add_child(bottom_split)

    var text_box := VBoxContainer.new()
    text_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
    text_box.size_flags_vertical = Control.SIZE_EXPAND_FILL
    text_box.add_theme_constant_override("separation", 12)
    bottom_split.add_child(text_box)

    message_label = Label.new()
    message_label.text = ""
    message_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
    message_label.add_theme_font_size_override("font_size", 34)
    message_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
    text_box.add_child(message_label)

    log_label = Label.new()
    log_label.text = ""
    log_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
    log_label.add_theme_font_size_override("font_size", 20)
    log_label.modulate = Color(0.22, 0.22, 0.20)
    text_box.add_child(log_label)

    skill_grid = GridContainer.new()
    skill_grid.columns = 2
    skill_grid.custom_minimum_size = Vector2(560, 0)
    skill_grid.add_theme_constant_override("h_separation", 10)
    skill_grid.add_theme_constant_override("v_separation", 10)
    bottom_split.add_child(skill_grid)


func _make_status_panel() -> PanelContainer:
    var panel := PanelContainer.new()
    panel.add_theme_stylebox_override("panel", _panel_style(PANEL_COLOR, PANEL_BORDER, 8))

    var content := VBoxContainer.new()
    content.name = "Content"
    content.add_theme_constant_override("separation", 5)
    panel.add_child(content)

    var name_label := Label.new()
    name_label.name = "Name"
    name_label.add_theme_font_size_override("font_size", 26)
    content.add_child(name_label)

    var meta_label := Label.new()
    meta_label.name = "Meta"
    meta_label.add_theme_font_size_override("font_size", 18)
    meta_label.modulate = Color(0.25, 0.25, 0.23)
    content.add_child(meta_label)

    var hp_bar := ProgressBar.new()
    hp_bar.name = "Hp"
    hp_bar.show_percentage = false
    hp_bar.custom_minimum_size = Vector2(0, 24)
    content.add_child(hp_bar)

    var focus_bar := ProgressBar.new()
    focus_bar.name = "Focus"
    focus_bar.show_percentage = false
    focus_bar.custom_minimum_size = Vector2(0, 18)
    content.add_child(focus_bar)
    return panel


func _make_battler_block(color: Color, label_text: String) -> PanelContainer:
    var panel := PanelContainer.new()
    panel.add_theme_stylebox_override("panel", _panel_style(color, Color(0.12, 0.12, 0.12), 12))

    var label := Label.new()
    label.text = label_text
    label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
    label.add_theme_font_size_override("font_size", 24)
    label.add_theme_color_override("font_color", Color.WHITE)
    panel.add_child(label)
    return panel


func _panel_style(color: Color, border: Color, radius: int) -> StyleBoxFlat:
    var style := StyleBoxFlat.new()
    style.bg_color = color
    style.border_color = border
    style.set_border_width_all(4)
    style.set_corner_radius_all(radius)
    style.content_margin_left = 18
    style.content_margin_right = 18
    style.content_margin_top = 12
    style.content_margin_bottom = 12
    return style


func _refresh_skills() -> void:
    for child in skill_grid.get_children():
        child.queue_free()
    skill_buttons.clear()

    for skill in bridge.get_available_skills():
        var button := Button.new()
        button.text = "%s\nPWR %s / FOCUS %s" % [skill.get("name", "Skill"), skill.get("power", 0), skill.get("focus_cost", 0)]
        button.custom_minimum_size = Vector2(270, 86)
        button.disabled = input_locked
        button.pressed.connect(_on_skill_pressed.bind(skill.get("id", "")))
        skill_grid.add_child(button)
        skill_buttons.push_back(button)


func _on_skill_pressed(skill_id: String) -> void:
    if input_locked:
        return

    input_locked = true
    _set_buttons_disabled(true)
    var result := bridge.use_skill(skill_id)
    if not result.get("accepted", false):
        _set_message(result.get("error", "That action failed."))
        input_locked = false
        _set_buttons_disabled(false)
        return

    await _play_events(result.get("events", []))
    _update_state(result.get("state", bridge.get_battle_state()))
    _refresh_skills()

    if result.get("battle_finished", false):
        _set_message("Battle finished. Winner: %s" % result.get("winner", "none"))
    else:
        _set_message("What will your player do?")
        input_locked = false
        _set_buttons_disabled(false)


func _play_events(events: Array) -> void:
    for event in events:
        _apply_event(event)
        await get_tree().create_timer(0.45).timeout


func _apply_event(event: Dictionary) -> void:
    var type := String(event.get("type", "none"))
    match type:
        "battle_started":
            _push_log("Battle start!")
            _set_message("A normal opponent wants to battle.")
        "skill_started":
            var actor := _display_actor(event.get("actor", "none"))
            _set_message("%s used %s." % [actor, event.get("skill_id", "a skill")])
            _push_log(message_label.text)
        "focus_changed":
            _animate_focus(event.get("actor", "none"), event.get("old_value", 0), event.get("new_value", 0))
        "attack_missed":
            _set_message("It missed.")
            _push_log("The play missed.")
        "damage_applied":
            _animate_hp(event.get("target", "none"), event.get("old_value", 0), event.get("new_value", 0))
            _push_log("%s took %s damage." % [_display_actor(event.get("target", "none")), event.get("amount", 0)])
        "healing_applied":
            _animate_hp(event.get("actor", "none"), event.get("old_value", 0), event.get("new_value", 0))
            _push_log("%s recovered %s HP." % [_display_actor(event.get("actor", "none")), event.get("amount", 0)])
        "status_applied":
            _push_log("A status effect changed momentum.")
        "skill_xp_gained":
            _push_log("%s gained skill XP." % _display_actor(event.get("actor", "none")))
        "skill_leveled_up":
            _push_log("A skill leveled up.")
        "battle_finished":
            _set_message("The battle is over.")
            _push_log("Winner: %s" % event.get("winner", "none"))
        "reward_granted":
            _push_log("Battle XP awarded.")


func _update_state(state: Dictionary) -> void:
    var player := state.get("player", {})
    var opponent := state.get("opponent", {})
    _update_status(player_name, player_meta, player_hp, player_focus, player, "Your Player")
    _update_status(opponent_name, opponent_meta, opponent_hp, opponent_focus, opponent, "Opponent")


func _update_status(name_label: Label, meta_label: Label, hp_bar: ProgressBar, focus_bar: ProgressBar, data: Dictionary, fallback_name: String) -> void:
    var max_hp := int(data.get("max_hp", 1))
    var hp := int(data.get("hp", 0))
    var max_focus := int(data.get("max_focus", 1))
    var focus := int(data.get("focus", 0))
    name_label.text = "%s" % data.get("name", fallback_name)
    meta_label.text = "%s / %s" % [data.get("spec", "Spec"), data.get("style", "Style")]
    hp_bar.max_value = max(1, max_hp)
    hp_bar.value = clamp(hp, 0, max_hp)
    hp_bar.tooltip_text = "HP %s/%s" % [hp, max_hp]
    focus_bar.max_value = max(1, max_focus)
    focus_bar.value = clamp(focus, 0, max_focus)
    focus_bar.tooltip_text = "Focus %s/%s" % [focus, max_focus]


func _animate_hp(actor: String, old_value: int, new_value: int) -> void:
    var bar := opponent_hp if actor == "opponent" else player_hp
    bar.value = old_value
    create_tween().tween_property(bar, "value", new_value, 0.30)


func _animate_focus(actor: String, old_value: int, new_value: int) -> void:
    var bar := opponent_focus if actor == "opponent" else player_focus
    bar.value = old_value
    create_tween().tween_property(bar, "value", new_value, 0.25)


func _set_message(text: String) -> void:
    message_label.text = text


func _push_log(text: String) -> void:
    if text.is_empty():
        return

    log_entries.push_front(text)
    if log_entries.size() > LOG_LINES:
        log_entries.resize(LOG_LINES)
    log_label.text = "\n".join(log_entries)


func _display_actor(actor: String) -> String:
    if actor == "player":
        return "Your player"
    if actor == "opponent":
        return "Opponent"
    return "Someone"


func _set_buttons_disabled(disabled: bool) -> void:
    for button in skill_buttons:
        button.disabled = disabled
