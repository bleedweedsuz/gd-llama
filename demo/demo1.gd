extends Node

@onready var gdl_lama: GDLLama = $GDLLama
@onready var text_edit: TextEdit = $CanvasLayer/Control/TextEdit
@onready var ask: UIButton = $CanvasLayer/Control/Ask
@onready var reset_context: UIButton = $CanvasLayer/Control/ResetContext
@onready var output: Label = $CanvasLayer/Control/Output

func _ready():
	ask.buttonCommit = Callable(self, "_ask_button_pressed")
	reset_context.buttonCommit = Callable(self, "_reset_context_button_pressed")
	
	# Load the Model
	gdl_lama.set_context_size(1080)
	gdl_lama.load_model("res://models/mistral-7b-instruct-v0.2.Q4_K_M.gguf")
	
func _ask_button_pressed():
	if text_edit.text.is_empty():
		output.text = "Please ask something.."
		return
	
	# Send prompt to model
	var response = gdl_lama.ask(text_edit.text)
	output.text = response
	 
	# Cache info
	print(gdl_lama.kv_cache_info())

func _reset_context_button_pressed():
	# Reset model and load again
	gdl_lama.reset_context()
