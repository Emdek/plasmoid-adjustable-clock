document.ondragstart = preventDrag;
document.onmousedown = clearPrevention;
document.onmouseup = fixFinalSelection;
document.onkeyup = fixFinalSelection;

var previousRange = null;
var preventSelectionFix = false;
var forward = true;

function preventDrag(event)
{
	event.preventDefault();
}

function clearPrevention()
{
	preventSelectionFix = false;
}

function fixFinalSelection(event)
{
	var selection = window.getSelection();

	if (event.shiftKey)
	{
		return;
	}

	preventSelectionFix = true;

	var element = selection.anchorNode;

	do
	{
		if (element.nodeName == 'PLACEHOLDER')
		{
			var range = selection.getRangeAt(0);

			if (forward)
			{
				range.setStartBefore(element, 0);
			}
			else
			{
				range.setEndAfter(element, element.childNodes.length);
			}

			selection.removeAllRanges();
			selection.addRange(range);

			break;
		}

		element = element.parentNode;

	} while (element.parentNode);

	selection = window.getSelection();

	var element = (forward ? selection.focusNode : selection.anchorNode);

	do
	{
		if (element.nodeName == 'PLACEHOLDER')
		{
			var range = selection.getRangeAt(0);

			if (forward)
			{
				range.setEndAfter(element, element.childNodes.length);
			}
			else
			{
				range.setStartBefore(element, 0);
			}

			selection.removeAllRanges();
			selection.addRange(range);

			break;
		}

		element = element.parentNode;

	} while (element.parentNode);
}

function fixSelection(event)
{
	if (preventSelectionFix)
	{
		return;
	}

	var selection = window.getSelection();

	if (previousRange)
	{
		forward = (previousRange.compareBoundaryPoints(Range.START_TO_START, selection.getRangeAt(0)) == 0 && previousRange.compareBoundaryPoints(Range.END_TO_END, selection.getRangeAt(0)) != 0);
	}
	else
	{
		forward = true;
	}

	var element = selection.focusNode;

	do
	{
		if (element.nodeName == 'PLACEHOLDER')
		{
			selection.extend(element, (forward ? element.childNodes.length : 0));

			break;
		}

		element = element.parentNode;

	} while (element.parentNode);

	previousRange = window.getSelection().getRangeAt(0);
}
