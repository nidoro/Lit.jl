
function DD_GetElement(selectorOrElem) {
    if (typeof selectorOrElem == "string") {
        return document.querySelector(selectorOrElem);
    } else {
        return selectorOrElem;
    }
}

function DD_GetElements(selectorOrElem) {
    if (typeof selectorOrElem == "string") {
        return document.querySelectorAll(selectorOrElem);
    } else if (Array.isArray(selectorOrElem)) {
        return selectorOrElem;
    } else {
        return [selectorOrElem];
    }
}

function DD_CreateElement(tag, innerHTML) {
    let result = document.createElement(tag);
    if (innerHTML != undefined) result.innerHTML = innerHTML;
    return result;
}

function DD_GetLastMatch(selectorOrElem) {
    let elements = DD_GetElements(selectorOrElem);
    if (elements.length) {
        return elements[elements.length-1];
    } else {
        return null;
    }
}

function DD_GetNthMatch(selectorOrElem, n) {
    let elements = DD_GetElements(selectorOrElem);
    if (elements.length > n) {
        return elements[n];
    } else {
        return null;
    }
}

function DD_ForEach(selectorOrElem, action) {
    let elements = DD_GetElements(selectorOrElem);
    for (elem of elements) {
        action(elem);
    }
}

function DD_RemoveElements(selectorOrElem) {
    let fragment = document.createDocumentFragment(); 
    fragment.textContent = ' ';
    
    let elements = DD_GetElements(selectorOrElem);
    
    if (elements.length) {
        fragment.firstChild.replaceWith(...elements);
    }
}

function DD_RemoveChildren(selectorOrElem) {
    DD_ForEach(selectorOrElem, function(elem) {
        while (elem.firstChild) elem.firstChild.remove();
    });
}

function DD_AddEventListener(selectorOrElem, eventType, listener, options) {
    if (options == undefined) options = {};
    DD_ForEach(selectorOrElem, function(elem) {
        elem.addEventListener(eventType, listener, options);
    });
}

function DD_RemoveEventListener(selectorOrElem, eventType, listener, options) {
    if (options == undefined) options = {};
    DD_ForEach(selectorOrElem, function(elem) {
        elem.removeEventListener(eventType, listener, options);
    });
}

function DD_SetEventListener(selectorOrElem, eventType, listener) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem[eventType] = listener;
    });
}

function DD_SetAttribute(selectorOrElem, attribute, value) {
    if (value == undefined) value = "";
    DD_ForEach(selectorOrElem, function(elem) {
        elem.setAttribute(attribute, value);
    });
}

function DD_GetAttribute(selectorOrElem, attribute) {
    return DD_GetElement(selectorOrElem).getAttribute(attribute);
}

function DD_HasAttribute(selectorOrElem, attribute) {
    return DD_GetElement(selectorOrElem).hasAttribute(attribute);
}

function DD_RemoveAttribute(selectorOrElem, attribute) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.removeAttribute(attribute);
    });
}

function DD_SetStyle(selectorOrElem, property, value) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.style[property] = value;
    });
}

function DD_GetStyle(selectorOrElem, style) {
    let elem = DD_GetElement(selectorOrElem);
    return elem.style[style];
}

function DD_SetProperty(selectorOrElem, property, value) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.style.setProperty(property, value);
    });
}

function DD_SetValue(selectorOrElem, value) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.value = value;
    });
}

function DD_GetValue(selectorOrElem, value) {
    return DD_GetElement(selectorOrElem).value;
}

function DD_SetChecked(selectorOrElem, value) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.checked = value;
    });
}

function DD_ToggleCheckbox(selectorOrElem) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.checked = !elem.checked;
    });
}

function DD_Focus(selectorOrElem) {
    DD_GetElement(selectorOrElem).focus();
}

function DD_Click(selectorOrElem) {
    DD_GetElement(selectorOrElem).click();
}

function DD_IsElement(elem, selector) {
    return elem == DD_GetElement(selector);
}

function DD_IsChecked(selectorOrElem) {
    return DD_GetElement(selectorOrElem).checked;
}

function DD_SetInnerHTML(selectorOrElem, html) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.innerHTML = html;
    });
}

function DD_AddClass(selectorOrElem, className) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.classList.add(className);
    });
}

function DD_RemoveClass(selectorOrElem, className) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.classList.remove(className);
    });
}

function DD_HasClass(selectorOrElem, className) {
    return DD_GetElement(selectorOrElem).classList.contains(className);
}

function DD_ToggleClass(selectorOrElem, className) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.classList.toggle(className);
    });
}

function DD_SetInnerText(selectorOrElem, html) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.innerText = html;
    });
}

function DD_ClearInnerText(selectorOrElem) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.innerText = "";
    });
}

function DD_AppendChild(selectorOrElem, elem) {
    DD_GetElement(selectorOrElem).appendChild(elem);
}

function DD_Show(selectorOrElem) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.removeAttribute("hidden");
    });
}

function DD_Hide(selectorOrElem) {
    DD_ForEach(selectorOrElem, function(elem) {
        elem.setAttribute("hidden", "");
    });
}

function DD_ShowIf(selectorOrElem, condition, selectorOrElemPlaceholder) {
    if (condition) {
        DD_Show(selectorOrElem);
        
        if (selectorOrElemPlaceholder != undefined) {
            DD_Hide(selectorOrElemPlaceholder);
        }
    } else {
        DD_Hide(selectorOrElem);
        
        if (selectorOrElemPlaceholder != undefined) {
            DD_Show(selectorOrElemPlaceholder);
        }
    }
}

function DD_HideIf(selectorOrElem, condition, selectorOrElemPlaceholder) {
    return DD_ShowIf(selectorOrElem, !condition, selectorOrElemPlaceholder);
}


