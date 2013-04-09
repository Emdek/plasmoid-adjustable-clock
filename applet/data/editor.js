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
	if (!event.target.hasAttribute('component'))
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
		if (startElement.hasAttribute && startElement.hasAttribute('component'))
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
		if (endElement.hasAttribute && endElement.hasAttribute('component'))
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

function insertComponent(component, options, title, value)
{
	var element = document.createElement('span');
	element.setAttribute('component', component);
	element.setAttribute('title', title);
	element.textContent = value;

	if (options != '')
	{
		element.setAttribute('options', options);
	}

	var range = window.getSelection().getRangeAt(0);
	range.deleteContents();
	range.insertNode(element);
}

function getStyle(property)
{
	var selection = window.getSelection();

	if (selection.anchorNode)
	{
		return window.getComputedStyle(selection.anchorNode.parentNode).getPropertyValue(property);
	}
}

function setStyle(property, value)
{
	var selection = window.getSelection();

	if (selection.isCollapsed)
	{
		document.body.style.setProperty(property, value);

		return;
	}

	var startElement = selection.anchorNode.parentNode;
	var endElement = selection.focusNode.parentNode;

	if (startElement == endElement)
	{
		startElement.style.setProperty(property, value);
	}
	else
	{
		var parentElement = null;
		var range = selection.getRangeAt(0);
		var contents = range.extractContents();
		var container = ((startElement.parentNode == endElement.parentNode) ? startElement.parentNode : null);

		if (startElement.hasAttribute('component'))
		{
			startElement.parentNode.removeChild(startElement);
		}

		if (endElement.hasAttribute('component'))
		{
			endElement.parentNode.removeChild(endElement);
		}

		if (container && !container.hasChildNodes())
		{
			parentElement = container;
		}
		else
		{
			parentElement = document.createElement('span');
		}

		parentElement.appendChild(contents);
		parentElement.style.setProperty(property, value);

		range.insertNode(parentElement)
	}
}
