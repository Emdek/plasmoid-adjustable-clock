Clock.sendEvent = function (name)
{
	var event = document.createEvent('Event');
	event.initEvent(name, false, false);

	document.dispatchEvent(event);
}

Clock.setStyleSheet = function (url)
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

	link.setAttribute('href', url);
}

Clock.setStyle = function (object, property, value)
{
	object.style.setProperty(property, value);
}

Clock.setRuleStyle = function (rule, property, value)
{
	document.styleSheets[0].cssRules[rule].style.setProperty(property, value);
}

Clock.getStyle = function (object, property)
{
	return window.getComputedStyle(object).getPropertyValue(property);
}
