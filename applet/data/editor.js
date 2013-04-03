document.ondragstart = preventDrag;
document.onclick = selectComponent;
document.onmouseup = fixFinalSelection;
document.onkeyup = fixFinalSelection;

var previousRange = null;
var forward = true;

function preventDrag(event)
{
	event.preventDefault();
}

function selectComponent(event)
{
	if (event.target.className.indexOf('component') == -1)
	{
		return;
	}

	window.setTimeout(function()
	{
		var range = document.createRange();
		range.selectNodeContents(event.target);

		var selection = window.getSelection();
		selection.removeAllRanges();
		selection.addRange(range);
	}, 1);
}

function fixFinalSelection(event)
{
	if (event && event.shiftKey)
	{
		return;
	}

	var selection = window.getSelection();
	var range = selection.getRangeAt(0);
	var startElement = selection.anchorNode;
	var endElement = selection.focusNode;

	while (startElement)
	{
		if (startElement.className && startElement.className.indexOf('component') >= 0)
		{
			if (forward)
			{
				range.setStartBefore(startElement, 0);
			}
			else
			{
				range.setEndAfter(startElement, startElement.childNodes.length);
			}

			break;
		}

		startElement = startElement.parentNode;
	}

	while (endElement)
	{
		if (endElement.className && endElement.className.indexOf('component') >= 0)
		{
			if (forward)
			{
				range.setEndAfter(endElement, endElement.childNodes.length);
			}
			else
			{
				range.setStartBefore(endElement, 0);
			}

			break;
		}

		endElement = endElement.parentNode;
	}

	selection.removeAllRanges();
	selection.addRange(range);
}

function fixSelection()
{
	var currentRange = window.getSelection().getRangeAt(0);

	if (previousRange)
	{
		forward = (previousRange.compareBoundaryPoints(Range.START_TO_START, currentRange) == 0 && previousRange.compareBoundaryPoints(Range.END_TO_END, currentRange) != 0);
	}
	else
	{
		forward = true;
	}

	previousRange = window.getSelection().getRangeAt(0);
}

function setStyle(attribute, value)
{
	var selection = window.getSelection();
	var startElement = selection.anchorNode.parentNode;
	var endElement = selection.focusNode.parentNode;

	if (startElement == endElement)
	{
		startElement.style.setProperty(attribute, value);
	}
	else
	{
		var range = selection.getRangeAt(0);
		var newElement = document.createElement('span');
		newElement.style.setProperty(attribute, value);
		newElement.appendChild(range.extractContents());

		if (startElement.className && startElement.className.indexOf('component') >= 0)
		{
			startElement.parentNode.removeChild(startElement);
		}

		if (endElement.className && endElement.className.indexOf('component') >= 0)
		{
			endElement.parentNode.removeChild(endElement);
		}

		range.insertNode(newElement)
	}
}