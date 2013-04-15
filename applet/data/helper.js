Clock.setStyle = function (object, property, value)
{
	object.style.setProperty(property, value);
}

Clock.getStyle = function (object, property)
{
	return window.getComputedStyle(object).getPropertyValue(property);
}
