extends CanvasLayer
## Login / registration / character selection screen. Pure presentation:
## it only forwards the player's choices to NetClient and shows what the
## server answers — it never decides anything itself (see
## docs/ARCHITECTURE.md). Built in code rather than a hand-written .tscn
## layout so the whole UI is reviewable in one file.
##
## Also drives the same flow non-interactively (auto_login) for headless
## end-to-end tests — through the same handlers the buttons use.

signal reconnect_requested(host: String, port: int)

const REASON_TEXT := {
	"bad_credentials": "Błędny login lub hasło.",
	"username_taken": "Ta nazwa konta jest już zajęta.",
	"invalid_input": "Login: 3-32 znaki (litery, cyfry, _). Hasło: 6-128 znaków.",
	"rate_limited": "Za dużo prób — odczekaj chwilę.",
	"server_error": "Błąd serwera, spróbuj ponownie.",
	"invalid_name": "Nazwa postaci: 3-20 znaków (litery, cyfry, spacje).",
	"name_taken": "Ta nazwa postaci jest już zajęta.",
	"not_found": "Nie znaleziono tej postaci.",
}

var _net: Node
var _status: Label
var _server_address: LineEdit
var _login_page: VBoxContainer
var _char_page: VBoxContainer
var _username: LineEdit
var _password: LineEdit
var _login_button: Button
var _register_button: Button
var _reconnect_button: Button
var _char_list: ItemList
var _enter_button: Button
var _new_char_name: LineEdit
var _create_button: Button

var _auto := {}  # non-empty only in auto_login mode


func setup(net: Node) -> void:
	_net = net
	_net.secure_connection_ready.connect(_on_secure_connection_ready)
	_net.connection_failed.connect(_on_connection_failed)
	_net.auth_result.connect(_on_auth_result)
	_net.characters_listed.connect(_on_characters_listed)
	_net.character_created.connect(_on_character_created)
	_net.character_create_failed.connect(_on_character_failed)
	_net.character_select_failed.connect(_on_character_failed)
	_net.entered_world.connect(_on_entered_world)


func _ready() -> void:
	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(560, 0)
	center.add_child(panel)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 16)
	panel.add_child(root)

	_status = Label.new()
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_status.text = "Łączenie z serwerem..."
	root.add_child(_status)

	_login_page = VBoxContainer.new()
	root.add_child(_login_page)
	# On a phone the default 127.0.0.1 is the phone itself — the player has
	# to be able to enter the PC's LAN address (applied via "Połącz ponownie").
	_server_address = _line_edit(_login_page, "Serwer (host:port)")
	_username = _line_edit(_login_page, "Login")
	_password = _line_edit(_login_page, "Hasło")
	_password.secret = true
	var auth_row := HBoxContainer.new()
	_login_page.add_child(auth_row)
	_login_button = _button(auth_row, "Zaloguj", _on_login_pressed)
	_register_button = _button(auth_row, "Zarejestruj", _on_register_pressed)
	_reconnect_button = _button(root, "Połącz ponownie", _on_reconnect_pressed)
	_reconnect_button.visible = false

	_char_page = VBoxContainer.new()
	_char_page.visible = false
	root.add_child(_char_page)
	_char_list = ItemList.new()
	_char_list.custom_minimum_size = Vector2(0, 240)
	_char_page.add_child(_char_list)
	_enter_button = _button(_char_page, "Wejdź do gry", _on_enter_pressed)
	_new_char_name = _line_edit(_char_page, "Nazwa nowej postaci")
	_create_button = _button(_char_page, "Utwórz postać", _on_create_pressed)

	_set_auth_enabled(false)


func _line_edit(parent: Control, placeholder: String) -> LineEdit:
	var edit := LineEdit.new()
	edit.placeholder_text = placeholder
	edit.custom_minimum_size = Vector2(0, 64)
	parent.add_child(edit)
	return edit


func _button(parent: Control, text: String, handler: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(0, 72)  # touch-sized
	button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	button.pressed.connect(handler)
	parent.add_child(button)
	return button


func _set_auth_enabled(enabled: bool) -> void:
	_login_button.disabled = not enabled
	_register_button.disabled = not enabled


## Runs the whole flow without input: log in (or register first), then
## select `character_name`, creating it if the account doesn't have it yet.
func auto_login(username: String, password: String, register: bool, character_name: String) -> void:
	_auto = {"register": register, "character": character_name}
	_username.text = username
	_password.text = password


func set_server_address(host: String, port: int) -> void:
	_server_address.text = "%s:%d" % [host, port]


func show_login(message: String) -> void:
	visible = true
	_login_page.visible = true
	_char_page.visible = false
	_status.text = message


func _on_secure_connection_ready() -> void:
	_status.text = "Połączono (szyfrowane). Zaloguj się lub załóż konto."
	_reconnect_button.visible = false
	_set_auth_enabled(true)
	if not _auto.is_empty():
		if _auto["register"]:
			_on_register_pressed()
		else:
			_on_login_pressed()


func _on_connection_failed(reason: String) -> void:
	show_login("Brak połączenia z serwerem (%s)." % reason)
	_set_auth_enabled(false)
	_reconnect_button.visible = true


func _on_reconnect_pressed() -> void:
	var address := _server_address.text.strip_edges()
	var host := address
	var port := 7777
	var colon := address.rfind(":")
	if colon != -1:
		host = address.substr(0, colon)
		var port_text := address.substr(colon + 1)
		if not port_text.is_valid_int() or int(port_text) < 1 or int(port_text) > 65535:
			_status.text = "Niepoprawny adres serwera — użyj formatu host:port."
			return
		port = int(port_text)
	if host.is_empty():
		_status.text = "Podaj adres serwera."
		return
	_reconnect_button.visible = false
	_status.text = "Łączenie z %s:%d..." % [host, port]
	reconnect_requested.emit(host, port)


func _on_login_pressed() -> void:
	_set_auth_enabled(false)
	_status.text = "Logowanie..."
	_net.send_login(_username.text.strip_edges(), _password.text)


func _on_register_pressed() -> void:
	_set_auth_enabled(false)
	_status.text = "Rejestracja..."
	_net.send_register(_username.text.strip_edges(), _password.text)


func _on_auth_result(ok: bool, reason: String) -> void:
	if not ok:
		# Auto mode: registering an account that already exists falls back to
		# logging in, so the same test command works on every run.
		if not _auto.is_empty() and _auto["register"] and reason == "username_taken":
			_auto["register"] = false
			await get_tree().create_timer(1.1).timeout  # server auth throttle is 1 s
			_on_login_pressed()
			return
		_status.text = REASON_TEXT.get(reason, "Logowanie nieudane (%s)." % reason)
		_set_auth_enabled(true)
		return
	_password.text = ""  # don't keep the password around once it's been used
	_login_page.visible = false
	_char_page.visible = true
	_status.text = "Wybierz postać."
	_net.request_character_list()


func _on_characters_listed(characters: Array) -> void:
	_char_list.clear()
	for character in characters:
		var index := _char_list.add_item(character["name"])
		_char_list.set_item_metadata(index, character["id"])
	if characters.is_empty():
		_status.text = "Nie masz jeszcze postaci — utwórz pierwszą."

	if not _auto.is_empty():
		for character in characters:
			if character["name"] == _auto["character"]:
				_net.send_character_select(character["id"])
				return
		_new_char_name.text = _auto["character"]
		_on_create_pressed()


func _on_create_pressed() -> void:
	_net.send_character_create(_new_char_name.text.strip_edges())


func _on_character_created(_id: int, character_name: String) -> void:
	_new_char_name.text = ""
	_status.text = "Utworzono postać %s." % character_name
	_net.request_character_list()


func _on_character_failed(reason: String) -> void:
	_status.text = REASON_TEXT.get(reason, "Operacja nieudana (%s)." % reason)


func _on_enter_pressed() -> void:
	var selected := _char_list.get_selected_items()
	if selected.is_empty():
		_status.text = "Najpierw wybierz postać z listy."
		return
	_net.send_character_select(_char_list.get_item_metadata(selected[0]))


func _on_entered_world(_local_entity_id: int) -> void:
	_auto = {}
	visible = false
