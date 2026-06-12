class_name BattleLineupPanel
extends PanelContainer

const LINEUP_CARD_SCENE := preload("res://ui/components/lineup_card.tscn")

@onready var lineup_grid: GridContainer = $Margin/LineupGrid

var lineup_cards: Array[BattleLineupCard] = []


func refresh(state: Dictionary) -> void:
	var active_index := int(state.get("active_player_index", 0))
	var player_team: Array = state.get("player_team", [])
	_ensure_card_count(player_team.size())
	for index in range(player_team.size()):
		lineup_cards[index].setup(player_team[index], index, active_index)
	for index in range(player_team.size(), lineup_cards.size()):
		lineup_cards[index].visible = false


func _ensure_card_count(count: int) -> void:
	while lineup_cards.size() < count:
		var card := LINEUP_CARD_SCENE.instantiate() as BattleLineupCard
		lineup_grid.add_child(card)
		lineup_cards.push_back(card)
