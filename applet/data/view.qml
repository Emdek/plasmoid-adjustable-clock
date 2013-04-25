import QtQuick 1.0
import QtWebKit 1.0

WebView
{
	javaScriptWindowObjects: QtObject
	{
		WebView.windowObjectName: 'Clock'

		function getOption(key, defaultValue)
		{
			return adjustableClock.getOption(key, defaultValue);
		}

		function getValue(component, options)
		{
			return adjustableClock.getValue(component, options);
		}
	}
}
