var UpdateControl = function(element, config) {
	config = config || {};

	var cssApply = function(key, control) {
		if (key in config)
			control.css(key, config[key]);
	};

	var container = $('<div style="position: absolute" />');
	cssApply("left", container);
	cssApply("top", container);

	var box = $('<input type="text" style="position: relative" />');
	cssApply("width", box);
	cssApply("height", box);
	box.attr("value", config.value);
	container.append(box);

	var button = $('<input type="button" value="OK" style="position: absolute" />')
		.css("left", box.css("right"));
	container.append(button);

	if (config.font != undefined) {
		for (key in config.font) {
			box.css(key, config.font[key]);
			button.css(key, config.font[key]);
		}
	}



	var finish = function(orderly) {
		container.remove();
		if (!orderly && config.abort) {
			config.abort();
		}
	};

	var accept = function() {
		finish(true);
		if (config.done) {
			config.done(box.prop("value"));
		}
	};

	var focusLostHandler;
	var cancelFocusLost = function() {
		if (focusLostHandler != undefined) {
			window.clearTimeout(focusLostHandler);
		}
	};
	var focusLost = function() {
		cancelFocusLost();
		focusLostHandler = window.setTimeout(finish, 100);
	};

	var focusedControl = box;
	this.focus = function() {
		focusedControl.focus();
	};

	box.blur(focusLost);
	box.focus(function() {
		cancelFocusLost();
		focusedControl = box;
	});
	box.keydown(function(ev) {
		if (ev.which == 13) {
			accept();
			ev.preventDefault();
		} else if (ev.which == 27) {
			finish(false);
		}
	});

	button.blur(focusLost);
	button.focus(function() {
		cancelFocusLost();
		focusedControl = button;
	});
	button.click(accept);

	element.append(container);
	this.focus();
};
