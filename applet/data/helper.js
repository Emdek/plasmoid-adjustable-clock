Clock.sendEvent = function (name)
{
	var event = document.createEvent('Event');
	event.initEvent(name, false, false);

	document.dispatchEvent(event);
}

Clock.setStyleSheet = function (data)
{
	var link = document.getElementById('theme_css');

	if (!link)
	{
		link = document.createElement('link');
		link.setAttribute('id', 'theme_css');
		link.setAttribute('rel', 'stylesheet');
		link.setAttribute('type', 'text/css');

		document.head.insertBefore(link, document.head.firstChild);
	}

	link.setAttribute('href', ('data:text/css;charset=utf-8;base64,' + window.btoa('html, body {margin: 0; padding: 0;} body {padding: 3px;} ' + data)));
}

Clock.setStyle = function (object, property, value)
{
	object.style.setProperty(property, value);
}

Clock.setRuleStyle = function (rule, property, value)
{
	document.styleSheets[1].cssRules[rule].style.setProperty(property, value);
}

Clock.getStyle = function (object, property)
{
	return window.getComputedStyle(object).getPropertyValue(property);
}
