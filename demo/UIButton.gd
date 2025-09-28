extends Button

class_name UIButton

@export var buttonCommit: Callable

func _ready() -> void:
	connect("pressed", _buttonPressed)
	
func _buttonPressed() -> void:
	if self.disabled: return
	
	if buttonCommit && buttonCommit.is_valid():
		buttonCommit.call()
