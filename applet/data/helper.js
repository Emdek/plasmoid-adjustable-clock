function parseBool(value)
{
	return (value == 'true');
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
