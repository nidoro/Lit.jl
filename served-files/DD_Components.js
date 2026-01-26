
let DD_Components = {};

DD_Components.getPreferredLang = function() {
    const preferredLanguages = navigator.languages;
    if (preferredLanguages) {
        const primary = preferredLanguages[0];
        return primary.split('-')[0].toLowerCase();
    }
    return undefined;
};

// https://stackoverflow.com/questions/5598743/finding-elements-position-relative-to-the-document
DD_Components.getPositionRelativeToPage = function(elem) {
    let box = elem.getBoundingClientRect();

    let body = document.body;
    let docEl = document.documentElement;

    let scrollTop = window.pageYOffset || docEl.scrollTop || body.scrollTop;
    let scrollLeft = window.pageXOffset || docEl.scrollLeft || body.scrollLeft;

    let clientTop = docEl.clientTop || body.clientTop || 0;
    let clientLeft = docEl.clientLeft || body.clientLeft || 0;

    let top  = box.top +  scrollTop - clientTop;
    let left = box.left + scrollLeft - clientLeft;

    return { top: top, left: left };
}

DD_Components.setStylePropertiesFromAttributes = function(elem, attributeMap) {
    for (const [attribute, property] of Object.entries(attributeMap)) {
        if (elem.hasAttribute(attribute)) {
            elem.style.setProperty(property, elem.getAttribute(attribute));
        }
    }
}

DD_Components.getSomethingByPath = function(path, context) {
    if (context == undefined) context = window;
    
    let end = path.indexOf("(");
    if (end >= 0) path = path.substring(0, end);
    
    let namespaces = path.split(/[.,\[,\]]+/);
    DD_Components.removeItemFromArrayIfCondition(namespaces, (entry) => entry.length == 0);
    let d = namespaces.pop();
    
    for (let i = 0; i < namespaces.length; i++) {
        if (Array.isArray(context)) {
            let idx = parseInt(namespaces[i]);
            if (idx < context.length) {
                context = context[idx];
            } else {
                return undefined;
            }
        } else {
            if (namespaces[i] in context) {
                context = context[namespaces[i]];
            } else {
                return undefined;
            }
        }
    }
    
    if (Array.isArray(context)) {
        let idx = parseInt(d);
        if (idx < context.length) {
            return {something: context[idx], context: context};
        } else {
            return undefined;
        }
    } else if (typeof context == 'object' && context != null) {
        if (d in context) {
            return {something: context[d], context: context};
        } else {
            return undefined;
        }
    } else {
        return {something: context, context: context};
    }
}

DD_Components.getDataByPath = function(path, context) {
    let result = DD_Components.getSomethingByPath(path, context);
    if (result != undefined) {
        return result.something;
    } else {
        return undefined;
    }
}

DD_Components.executeFunctionByName = function(functionName, context /*, args */) {
    let result = DD_Components.getSomethingByPath(functionName, context);
    if (result) {
        let args = Array.prototype.slice.call(arguments, 2);
        
        if (typeof result.something == 'function') {
            return {returned: result.something.apply(result.context, args), success: true};
        } else {
            return {returned: null, success: false};
        }
    } else {
        return {returned: null, success: false};
    }
}

DD_Components.getAssociatedData = function(elem) {
    while (elem && !("associatedData" in elem)) {
        elem = elem.parentElement;
    }
    
    if (elem) return elem.associatedData;
    else return undefined;
}

DD_Components.genSorterFunction = function(dataType, dataPath) {
    if (dataType == "string") {
        return function(data1, data2) {
            let something1 = DD_Components.getSomethingByPath(dataPath, data1);
            let something2 = DD_Components.getSomethingByPath(dataPath, data2);
            if (something1 != undefined && something2 != undefined) {
                let value1 = something1.something;
                let value2 = something2.something;
                if (typeof value1 == dataType && typeof value2 == dataType) {
                    return value1.localeCompare(value2);
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        }
    } else if (dataType == "number") {
        return function(data1, data2) {
            let something1 = DD_Components.getSomethingByPath(dataPath, data1);
            let something2 = DD_Components.getSomethingByPath(dataPath, data2);
            
            if (something1 != undefined && something2 != undefined) {
                let value1 = something1.something;
                let value2 = something2.something;
                if (typeof value1 == dataType && typeof value2 == dataType) {
                    return value1 - value2;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        }
    } else {
        return () => 0;
    }
}

DD_Components.createSortIcon = function(dataType, order) {
    let i = document.createElement("i");
    i.classList.add("dd-sort-icon");
    i.classList.add("fa");
    
    if (dataType == "string") {
        if (order == "ascending") {
            i.classList.add("fa-sort-alpha-asc");
        } else if (order == "descending") {
            i.classList.add("fa-sort-alpha-desc");
        }
    } else if (dataType == "number") {
        if (order == "ascending") {
            i.classList.add("fa-sort-numeric-asc");
        } else if (order == "descending") {
            i.classList.add("fa-sort-numeric-desc");
        }
    } else if (order == "ascending") {
        i.classList.add("fa-sort-asc");
    } else if (order == "descending") {
        i.classList.add("fa-sort-desc");
    }
    
    return i;
}

DD_Components.getSorter = function(elem) {
    let table = elem;
    if (table.tagName == "DD-CELL") {
        table = elem.getTable();
    }
    
    if (elem.hasAttribute("dd-sorter")) {
        let sorter = elem.getAttribute("dd-sorter");
        let func = undefined;
        let dataType = undefined;
        
        if (sorter.startsWith("[*]")) {
            elem.setAttribute("dd-sorter-args", "associated-data");
            let dataPath = sorter.substring(4);
            dataType = table.getDataType(dataPath);
            func = DD_Components.genSorterFunction(dataType, dataPath);
        } else {
            let something = DD_Components.getSomethingByPath(sorter);
            if (something && typeof something.something == "function") {
                func = something.something;
            }
        }
        
        if (func) {
            let sorter = {
                sorter: func,
                arg: elem.getAttribute("dd-sorter-args"),
                header: elem,
                dataType: dataType
            };
            
            let order = elem.getAttribute("dd-sort");
            if (order) {
                order = order.toLowerCase();
                if (order == "desc" || order == "descending") {
                    sorter.sorter = function(arg1, arg2) {return -1 * func(arg1, arg2)};
                }
            }
            
            return sorter;
        } else {
            return undefined;
        }
    }
}

DD_Components.getLevenshteinDistance = function(a, b) {
    const m = a.length;
    const n = b.length;

    // Initialize distance matrix
    const dp = Array.from({ length: m + 1 }, () => new Array(n + 1));

    for (let i = 0; i <= m; i++) dp[i][0] = i;
    for (let j = 0; j <= n; j++) dp[0][j] = j;

    // Fill matrix
    for (let i = 1; i <= m; i++) {
        for (let j = 1; j <= n; j++) {
            const cost = a[i - 1] === b[j - 1] ? 0 : 1;

            dp[i][j] = Math.min(
                dp[i - 1][j] + 1,      // deletion
                dp[i][j - 1] + 1,      // insertion
                dp[i - 1][j - 1] + cost // substitution
            );
        }
    }

    return dp[m][n];
}

DD_Components.setFocusable = function(elem) {
    elem.setAttribute("tabindex", "0");
}

DD_Components.setUnfocusable = function(elem) {
    elem.removeAttribute("tabindex");
}

DD_Components.isFocused = function(elem) {
    return document.activeElement === elem;
}

DD_Components.clickHandler = function(event) {
    if (event.currentTarget.hasAttribute("disabled")) return;
    if (DD_Components.isKeyEnter(event.keyCode) || DD_Components.isKeySpace(event.keyCode)) {
        event.currentTarget.click();
        if (DD_Components.isKeySpace(event.keyCode)) event.preventDefault();
    }
}

DD_Components.setClickable = function(elem) {
    DD_Components.setFocusable(elem);
    elem.addEventListener("click", (event) => {
        if (event.currentTarget.hasAttribute("disabled")) {
            event.preventDefault();
            event.stopPropagation();
            event.stopImmediatePropagation();
        }
    }, true);
    elem.addEventListener("keydown", DD_Components.clickHandler);
}

DD_Components.findInArray = function(array, condition) {
    for (let i = 0; i < array.length; ++i) {
        if (condition(array[i])) {
            return array[i];
        }
    }
    return undefined;
}

DD_Components.removeIndexFromArray = function(array, index) {
    return array.splice(index, 1)[0];
}

DD_Components.removeItemFromArrayIfCondition = function(array, condition) {
    for (let i = 0; i < array.length; ++i) {
        if (condition(array[i])) {
            DD_Components.removeIndexFromArray(array, i);
            return true;
        }
    }
    return false;
}

DD_Components.concatenateArrays = function(array1, array2) {
    array1.splice(array1.length, 0, ...array2);
}

DD_Components.isNumber = function(char) {
    let code = char.charCodeAt(0);
    return code >= '0'.charCodeAt(0) && code <= '9'.charCodeAt(0);
}

DD_Components.isAlpha = function(char) {
    let code = char.charCodeAt(0);
    return (code >= 'a'.charCodeAt(0) && code <= 'z'.charCodeAt(0)) || (code >= 'A'.charCodeAt(0) && code <= 'Z'.charCodeAt(0));
}

DD_Components.isAlphaNum = function(char) {
    return DD_Components.isAlpha(char) || DD_Components.isNumber(char);
}

DD_Components.isValidDataPathCharacter = function(char) {
    return DD_Components.isAlphaNum(char) || char == '[' || char == ']' || char == '.';
}

/* KEY CODES */
/* KEY CODES */
/* KEY CODES */

DD_Components.isKeyEnter = function(keyCode) {
    return keyCode === 13;
}

DD_Components.isKeyEsc = function(keyCode) {
    return keyCode === 27;
}

DD_Components.isKeyTab = function(keyCode) {
    return keyCode === 9;
}

DD_Components.isKeySpace = function(keyCode) {
    return keyCode === 32;
}

DD_Components.isKeyShift = function(keyCode) {
    return keyCode === 16;
}

DD_Components.isKeyCtrl = function(keyCode) {
    return keyCode === 17;
}

DD_Components.isKeyAlt = function(keyCode) {
    return keyCode === 18;
}

/* BODY OBSERVER */
/* BODY OBSERVER */
/* BODY OBSERVER */
    
DD_Components.bodyObserver = new MutationObserver(function() {
    for (let tooltip of DD_Tooltip.all) {
        if (!tooltip.isAnchorConnected()) {
            tooltip.remove();
        }
    }
    
    for (let menu of DD_Menu.all) {
        if (("anchor" in menu) && !menu.isAnchorConnected()) {
            menu.remove();
        }
    }
});

window.addEventListener("DOMContentLoaded", function() {
    DD_Components.bodyObserver.observe(document.body, {childList: true, subtree: true});
});

/* MOUSE */
/* MOUSE */
/* MOUSE */

DD_Components.mouse = {pageX: 0, pageY: 0, clientX: 0, clientY: 0};

DD_Components.updateMousePosition = function(event) {
    DD_Components.mouse = {
        pageX: event.pageX,
        pageY: event.pageY,
        clientX: event.clientX,
        clientY: event.clientY
    };
}

window.addEventListener("click", DD_Components.updateMousePosition);
window.addEventListener("mousedown", DD_Components.updateMousePosition);
window.addEventListener("mouseup", DD_Components.updateMousePosition);

/* ROUTING */
/* ROUTING */
/* ROUTING */

DD_Routing = {};

DD_Routing.move = function() {
    return false;
}

DD_Routing.getLocation = function(stringOrEvent) {
    if (typeof stringOrEvent == "object") {
        let event = stringOrEvent;
        if (event.target.tagName == "A") {
            return new URL(event.target.href);
        } else {
            return undefined;
        }
    } else if (typeof stringOrEvent == "string") {
        return new URL(stringOrEvent, window.location.toString());
    } else {
        return undefined;
    }
}

DD_Routing.go = function(stringOrEvent, action) {
    // If the user handled the location change, returns true; otherwise, false.
    
    if (stringOrEvent == undefined) stringOrEvent = window.location.toString();
    if (action == undefined) action = "auto";
    
    let url = DD_Routing.getLocation(stringOrEvent);
    
    if (url) {
        let changed = {
            pathname: window.location.pathname != url.pathname,
            search: window.location.search != url.search,
            hash: window.location.hash != url.hash
        };
        
        let handled = DD_Routing.move(url, changed);
        
        if (typeof handled != "string") {
            if (action == "auto") {
                if (changed.pathname || changed.search) {
                    action = "push";
                } else {
                    action = "nothing";
                }
            }
            
            if (action == "push") {
                DD_Routing.push(url);
            } else if (action == "set") {
                DD_Routing.replace(url);
            }
            
            return handled;
        } else {
            return DD_Routing.go(handled, action);
        }
    } else {
        return false;
    }
}

DD_Routing.set = function(destination) {
    return DD_Routing.go(destination, "set");
}

DD_Routing.goto = function(event) {
    let destination = event.target.getAttribute("dd-goto");
    if (destination == "" && event.target.tagName == "A") {
        destination = event.target.href;
    }
    
    if (event.type == "click" && event.button == 0 && event.ctrlKey) {
        let url = DD_Routing.getLocation(destination);
        window.open(url.toString(), "_blank");
        event.preventDefault();
    } else {
        let handled = DD_Routing.go(destination, "auto");
        
        if (event.target.tagName == "A" && handled) {
            event.preventDefault();
        }
    }
}

DD_Routing.replace = function(url) {
    DD_Routing.currentURL = url;
    window.history.replaceState(DD_Routing.historyState, "", url.toString());
}

DD_Routing.push = function(url) {
    DD_Routing.currentURL = url;
    window.history.pushState(DD_Routing.historyState, "", url.toString());
};

DD_Routing.getURLChanges = function(url1, url2) {
    return {
        pathname: url1.pathname != url2.pathname,
        search: url1.search != url2.search,
        hash: url1.hash != url2.hash
    };
};

(function() {
    DD_Routing.currentURL = new URL(window.location);
    DD_Routing.historyState = window.location.origin;
    
    window.history.replaceState(DD_Routing.historyState, "");
    
    window.addEventListener("popstate", function(event) {
        let fromURL = DD_Routing.currentURL;
        let toURL = new URL(window.location);
        let changed = DD_Routing.getURLChanges(fromURL, toURL);
        
        if (event.state == DD_Routing.historyState || changed.hash) {
            let handled = DD_Routing.move(toURL, changed);
            DD_Routing.currentURL = toURL;
            
            if (typeof handled == "string") {
                DD_Routing.go(handled, "nothing");
            } else if (!handled) {
                window.open(toURL.toString(), "_self");
            }
        }
    });
    
    window.addEventListener("click", function(event) {
        if (event.target.hasAttribute("dd-goto")) {
            event.target.addEventListener("click", DD_Routing.goto);
        }
    }, true);
})();

/* POPUP */
/* POPUP */
/* POPUP */

DD_Popup = {};

DD_Popup.stack = [];

DD_Popup.isAnyShowing = function() {
    return DD_Popup.stack.length > 0;
}

DD_Popup.windowKeyDownHandler = function(event) {
    if (event.keyCode === 27) {
        DD_Popup.close();
    }
}

DD_Popup.outsideClickHandler = function(event) {
    let popupElem = document.querySelector("dd-popup-container");
    if (event.target == popupElem) {
        DD_Popup.close();
    }
}

DD_Popup.clearCloseEventListeners = function() {
    let popupElem = document.querySelector("dd-popup-container");
    popupElem.removeEventListener("click", DD_Popup.outsideClickHandler);
    window.removeEventListener("keydown", DD_Popup.windowKeyDownHandler);
}

DD_Popup.hideAll = function() {
    let popups = document.querySelectorAll("dd-popup");
    
    for (let popup of popups) {
        popup.setAttribute("hidden", "");
        popup.classList.remove("dd-show");
    }
}

DD_Popup.closeAll = function() {
    let popupElem = document.querySelector("dd-popup-container");
    if (!popupElem) return;
    
    DD_Popup.hideAll();
    
    popupElem.setAttribute("hidden", "");
    DD_Popup.stack = [];
    
    DD_Popup.clearCloseEventListeners();
}
    
DD_Popup.close = function() {
    let popupElem = document.querySelector("dd-popup-container");
    
    DD_Popup.hideAll();
    
    popupElem.setAttribute("hidden", "");
    DD_Popup.stack.pop();
    
    if (DD_Popup.stack.length) {
        let elem = DD_Popup.stack[DD_Popup.stack.length-1];
        DD_Popup.show(elem);
        DD_Popup.stack.pop();
    } else {
        DD_Popup.clearCloseEventListeners();
    }
}

DD_Popup.show = function(selectorOrElem) {
    let popupElem = document.querySelector("dd-popup-container");
    let elem = selectorOrElem;
    
    if (typeof selectorOrElem == "string") {
        elem = popupElem.querySelector(`${selectorOrElem}`);
    }
    
    DD_Popup.stack.push(elem);
    
    DD_Popup.hideAll();
    DD_Popup.clearCloseEventListeners();
    
    elem.removeAttribute("hidden");
    elem.classList.add("dd-show");
    
    popupElem.removeAttribute("hidden");
    
    if (elem.hasAttribute("dd-outside-click-close")) {
        popupElem.addEventListener("click", DD_Popup.outsideClickHandler);
    }
    
    if (elem.hasAttribute("dd-esc-close")) {
        window.addEventListener("keydown", DD_Popup.windowKeyDownHandler);
    }
    
    let closeElem = elem.querySelector("dd-x");
    if (closeElem) {
        closeElem.addEventListener("click", DD_Popup.close);
    }
}

/* BUTTON */
/* BUTTON */
/* BUTTON */

class DD_Button extends HTMLElement {
    constructor() {
        super();
    }
    
    connectedCallback() {
        if (this.hasAttribute("dd-reconnecting")) {
            this.removeAttribute("dd-reconnecting");
            return;
        }

        this.initializeButton();
    }

    attributeChangedCallback(name, oldValue, newValue) {
    }
    
    initializeButton() {
        DD_Components.setStylePropertiesFromAttributes(this, DD_Button.attributeMap);
        
        if (!this.hasAttribute("dd-link")) {
            DD_Components.setClickable(this);
        }
    }
}

DD_Button.attributeMap = {
    "dd-border-color" : "--button-border-color",
    "dd-border-style" : "--button-border-style",
    "dd-border-width" : "--button-border-width",
    
    "dd-border-top" : "--button-border-top",
    "dd-border-right" : "--button-border-right",
    "dd-border-bottom" : "--button-border-bottom",
    "dd-border-left" : "--button-border-left",
    
    "dd-margin-top" : "--button-margin-top",
    "dd-margin-right" : "--button-margin-right",
    "dd-margin-bottom" : "--button-margin-bottom",
    "dd-margin-left" : "--button-margin-left",
    
    "dd-after-content" : "--button-after-content",
    "dd-after-font-size" : "--button-after-font-size",
    "dd-after-font-weight" : "--button-after-font-weight",
    "dd-after-transform" : "--button-after-transform",
    "dd-after-padding" : "--button-after-padding",

    "dd-margin" : "--button-margin",
    
    "dd-display" : "--button-display",
    "dd-gap" : "--button-gap",
    "dd-justify-content" : "--button-justify-content",
    
    "dd-bg" : "--button-bg",
    "dd-color" : "--button-color",
    "dd-padding" : "--button-padding",
    
    "dd-border-radius" : "--button-border-radius",
    "dd-font-size" : "--button-font-size",
    "dd-line-height" : "--button-line-height",
    "dd-font-weight" : "--button-font-weight",
    "dd-cursor" : "--button-cursor",
    "dd-text-transform" : "--button-text-transform",
    "dd-width" : "--button-width",
    "dd-white-space" : "--button-white-space",
    
    "dd-hover-color" : "--button-hover-color",
    "dd-hover-bg" : "--button-hover-bg",
    "dd-hover-border-color" : "--button-hover-border-color",
    
    "dd-active-color" : "--button-active-color",
    "dd-active-bg" : "--button-active-bg",
    "dd-active-border-color" : "--button-active-border-color",
    "dd-active-font-weight" : "--button-active-font-weight"
};
    
class DD_Tag extends DD_Button {}
class DD_Label extends DD_Button {}
class DD_Header extends DD_Button {}

class DD_Checkbox extends DD_Button {
    static observedAttributes = ["checked"];

    connectedCallback() {
        if (this.hasAttribute("dd-reconnecting")) {
            this.removeAttribute("dd-reconnecting");
            return;
        }

        this.initializeButton();
        this.initializeCheckbox();
    }
    
    disconnectedCallback() {
        if (this.hasAttribute("dd-reconnecting")) {
            return;
        }

        this.deinitializeCheckbox();
    }
    
    attributeChangedCallback(name, oldValue, newValue) {
        this.attributeChangedCheckboxCallback(name, oldValue, newValue);
    }
    
    initializeCheckbox() {
        this.addEventListener("click", this.clickHandler);
        
        if (this.hasAttribute("dd-group")) {
            let group = this.getGroup();
            if (!group) {
                group = {
                    name: this.getAttribute("dd-group"),
                    checkboxes: [],
                    selectAllCheckbox: undefined,
                    selectAllUpdateScheduled: false,
                    onAnyChange: this.getAttribute("dd-onanychange"),
                    onAnyChangeCalled: false
                };
                DD_Checkbox.groups.push(group);
            } 
            
            if (this.hasAttribute("dd-select-all")) {
                group.selectAllCheckbox = this;
            } else {
                group.checkboxes.push(this);
            }
        }
    }
    
    deinitializeCheckbox() {
        if (this.hasAttribute("dd-group")) {
            let group = this.getGroup();
            let thisCheckbox = this;
            if (group) {
                if (group.selectAllCheckbox == this) {
                    group.selectAllCheckbox = undefined;
                } else {
                    DD_Components.removeItemFromArrayIfCondition(group.checkboxes, (entry) => entry == thisCheckbox);
                    this.scheduleSelectAllCheckboxUpdateIfNeeded();
                }
            }
        }
    }
    
    clickHandler(event) {
        if (this.hasAttribute("disabled")) return;
        if (!this.hasAttribute("dd-noclick")) {
            if (this.hasAttribute("checked")) this.removeAttribute("checked");
            else this.setAttribute("checked", "");
        }
    }
    
    get checked() {
        return this.hasAttribute("checked");
    }
    
    set checked(v) {
        if (!v) {
            this.removeAttribute("checked");
        } else if (this.getAttribute("checked") != v) {
            this.setAttribute("checked", v);
        }
    }
    
    isDisabled() {
        return this.hasAttribute("disabled") || this.offsetParent == null;
    }
    
    isEnabled() {
        return !this.isDisabled();
    }
    
    updateSelectAllCheckbox() {
        let group = this.getGroup();
        if (group) {
            if (group.selectAllCheckbox) {
                let allSelected = this.areAllCheckboxesInGroupSelected();
                group.selectAllCheckbox.removeAttribute("dd-select-all");
                if (allSelected) {
                    group.selectAllCheckbox.setAttribute("checked", "");
                } else {
                    group.selectAllCheckbox.removeAttribute("checked");
                }
                group.selectAllCheckbox.setAttribute("dd-select-all", "");
            }
        }
    }
    
    scheduleSelectAllCheckboxUpdateIfNeeded() {
        let group = this.getGroup();
        if (!group.selectAllUpdateScheduled) {
            let thisElem = this;
            requestAnimationFrame(function() {
                thisElem.updateSelectAllCheckbox();
                group.selectAllUpdateScheduled = false;
            });
            group.selectAllUpdateScheduled = true;
        }
    }
    
    attributeChangedCheckboxCallback(name, oldValue, newValue) {
        oldValue = oldValue == null ? false : true;
        newValue = newValue == null ? false : true;
        
        if (name == "checked" && oldValue != newValue) {
            let group = this.getGroup();
            if (group) {
                if (this.hasAttribute("dd-select-all")) {
                    if (this.hasAttribute("checked")) {
                        this.checkAll();
                    } else {
                        this.uncheckAll();
                    }
                } else {
                    let selectAllCheckbox = this.getSelectAllCheckbox();
                    if (selectAllCheckbox) {
                        this.scheduleSelectAllCheckboxUpdateIfNeeded();
                    }
                }
                
                if (!this.hasAttribute("dd-silent") && group.onAnyChange && !group.onAnyChangeCalled) {
                    group.onAnyChangeCalled = true;
                    
                    requestAnimationFrame(function() {
                        DD_Components.executeFunctionByName(group.onAnyChange, window, group.name);
                        group.onAnyChangeCalled = false;
                    });
                }
            }
            
            if (!this.hasAttribute("dd-silent") && this.hasAttribute("dd-onchange")) {
                DD_Components.executeFunctionByName(this.getAttribute("dd-onchange"), window, this, oldValue, newValue, this);
            }
        }
    }
    
    get value() {
        return this.getAttribute("value");
    }
    
    set value(v) {
        this.setAttribute("value", v);
    }
    
    static get observedAttributes() { return ['checked']; }
    
    getGroup() {
        if (this.hasAttribute("dd-group")) {
            let groupName = this.getAttribute("dd-group");
            return DD_Checkbox.getGroup(groupName);
        }
        return undefined;
    }
    
    static getGroup(groupName) {
        for (let group of DD_Checkbox.groups) {
            if (group.name == groupName) {
                return group;
            }
        }
        return undefined;
    }

    static removeGroup(groupName) {
        DD_Components.removeItemFromArrayIfCondition(DD_Checkbox.groups, (entry) => entry.name == groupName);
    }
    
    static getEnabledCheckboxesInGroup(groupName) {
        let group = DD_Checkbox.getGroup(groupName);
        if (group) {
            let result = [];
            for (let cb of group.checkboxes) {
                if (cb.isEnabled())
                    result.push(cb);
            }
            return result;
        } else {
            return undefined;
        }
    }
    
    getSelectAllCheckbox() {
        let group = this.getGroup();
        if (group && group.selectAllCheckbox && group.selectAllCheckbox.hasAttribute("dd-select-all")) {
            return group.selectAllCheckbox;
        }
        return undefined;
    }
    
    areAllCheckboxesInGroupSelected() {
        let groupName = this.getAttribute("dd-group");
        let enabled = DD_Checkbox.getEnabledCheckboxesInGroup(groupName);
        
        if (enabled.length == 0) return false;
        
        for (let cb of enabled) {
            if (!cb.hasAttribute("checked")) {
                return false;
            }
        }
        return true;
    }
    
    uncheckAll() {
        if (this.hasAttribute("dd-group")) {
            let groupName = this.getAttribute("dd-group");
            DD_Checkbox.uncheckAllInGroup(groupName);
        }
    }
    
    checkAll() {
        if (this.hasAttribute("dd-group")) {
            let groupName = this.getAttribute("dd-group");
            DD_Checkbox.checkAllInGroup(groupName);
        }
    }
    
    static checkAllInGroup(groupName) {
        let group = DD_Checkbox.getGroup(groupName);
        if (group) {
            for (let checkbox of group.checkboxes) {
                checkbox.setAttribute("checked", "");
            }
        }
    }
    
    static uncheckAllInGroup(groupName) {
        let group = DD_Checkbox.getGroup(groupName);
        if (group) {
            for (let checkbox of group.checkboxes) {
                checkbox.removeAttribute("checked");
            }
        }
    }
    
    static isAnyCheckedInGroup(groupName) {
        let enabled = DD_Checkbox.getEnabledCheckboxesInGroup(groupName);
        
        if (enabled) {
            for (let cb of enabled) {
                if (cb.hasAttribute("checked")) {
                    return true;
                }
            }
            return false;
        } else {
            return undefined;
        }
    }
    
    static getCheckedInGroup(groupName) {
        let enabled = DD_Checkbox.getEnabledCheckboxesInGroup(groupName);
        if (enabled) {
            let result = [];
            for (let cb of enabled) {
                if (cb.hasAttribute("checked")) {
                    result.push(cb);
                }
            }
            
            return result;
        } else {
            return undefined;
        }
    }
    
    static getCheckedValuesInGroup(groupName) {
        let checkboxes = DD_Checkbox.getCheckedInGroup(groupName);
        if (checkboxes) {
            let result = [];
            for (let cb of checkboxes) {
                result.push(cb.value);
            }
            return result;
        }
        return undefined;
    }
    
    static selectInGroup(groupName, value, options) {
        if (!options) options = {};
        if (!("silent" in options)) options.silent = false;
        let group = DD_Checkbox.getGroup(groupName);
        
        let result = false;
        if (group) {
            let cb = DD_Components.findInArray(group.checkboxes, (entry) => entry.value == value);
            
            if (cb) {
                result = !cb.checked;
                
                if (options.silent) {
                    cb.setAttribute("dd-silent", "");
                    cb.checked = true;
                    cb.removeAttribute("dd-silent");
                } else {
                    cb.checked = true;
                }
                
                return result;
            } else {
                return undefined;
            }
        } else {
            return undefined;
        }
    }
}

DD_Checkbox.groups = [];

class DD_Toggle extends DD_Checkbox {
    connectedCallback() {
        super.connectedCallback();
        const track = document.createElement("div");
        track.classList.add("toggle-track");
        this.prepend(track);
    }

    disconnectedCallback() {
        super.disconnectedCallback();
    }
}

Object.assign(DD_Checkbox.attributeMap, {
    "dd-size" : "--checkbox-size",
    "dd-checkbox-bg" : "--checkbox-bg",
    "dd-checked-bg" : "--checkbox-checked-bg",
    "dd-checked-color" : "--checkbox-checked-color"
});

/* RADIO */
/* RADIO */
/* RADIO */

class DD_Radio extends DD_Checkbox {
    static observedAttributes = ["checked"];

    connectedCallback() {
        if (this.hasAttribute("dd-reconnecting")) {
            this.removeAttribute("dd-reconnecting");
            return;
        }

        this.initializeButton();
        this.initializeCheckbox();
    }
    
    disconnectedCallback() {
        if (this.hasAttribute("dd-reconnecting")) {
            return;
        }

        this.deinitializeCheckbox();
    }
    
    attributeChangedCallback(name, oldValue, newValue) {
        this.attributeChangedCheckboxCallback(name, oldValue, newValue);
        
        if (name == "checked" && newValue) {
            this.uncheckAllOthers();
        }
    }
    
    clickHandler() {
        if (this.hasAttribute("disabled")) return;
        if (!this.hasAttribute("checked")) {
            this.uncheckAllOthers();
            this.setAttribute("checked", "");
        }
    }
    
    uncheckAllOthers() {
        let group = this.getGroup();
        if (group) {
            for (let checkbox of group.checkboxes) {
                if (checkbox != this && checkbox.hasAttribute("checked")) {
                    checkbox.setAttribute("dd-silent", "");
                    checkbox.removeAttribute("checked");
                    checkbox.removeAttribute("dd-silent");
                }
            }
        }
    }
    
    static getGroupValue(groupName) {
        let values = DD_Checkbox.getCheckedValuesInGroup(groupName);
        if (values && values.length) {
            return values[0];
        }
        return undefined;
    }
    
    static selectInGroup(groupName, value, options) {
        return DD_Checkbox.selectInGroup(groupName, value, options);
    }
}

/* TAB AND AREA */
/* TAB AND AREA */
/* TAB AND AREA */

class DD_Tab extends DD_Button {    
    connectedCallback() {
        this.initializeButton();
        
        this.addEventListener("click", function(event) {
            if (!this.hasAttribute("dd-noclick")) {
                if (!this.hasAttribute("dd-link")) {
                    this.showArea();
                }
            }
        });
        
        DD_Area.addArea(this.getAttribute("dd-space"), this.getAttribute("dd-area"));
        DD_Area.addAreaTab(this.getAttribute("dd-space"), this.getAttribute("dd-area"), this);
    }
    
    activate() {
        this.classList.add("dd-active");
    }
    
    deactivate() {
        this.classList.remove("dd-active");
    }
    
    toggle() {
        this.classList.toggle("dd-active");
    }
    
    getAreaElement() {
        let space = this.getAttribute("dd-space");
        let area = this.getAttribute("dd-area");
        return DD_Area.getAreaElement(space, area);
    }
    
    showArea() {
        let area = this.getAreaElement();
        if (area) {
            area.show();
        }
    }
}

class DD_Area extends HTMLElement {
    connectedCallback() {
        this.onShow = function(){};
        this.onFinishShowing = null;
        
        DD_Area.addArea(this.getSpaceName(), this.getName(), this);
        let activeArea = DD_Area.getActiveAreaData(this.getSpaceName());
        
        if (!activeArea) {
            this.unhide();
            this.setActive();
        } else {
            this.hide();
        }
    }
    
    show(options) {
        if (options == undefined) options = {};
        if (!("navHistoryAction" in options)) options.navHistoryAction = "push";
        if (!("searchParams" in options)) options.searchParams = undefined;
        if (!("onShow" in options)) options.onShow = function(){};
        if (!("onFinishShowing" in options)) options.onFinishShowing = null;
        
        if (options.searchParams != undefined) {
            this.setAttribute("dd-search-params", options.searchParams);
        }
        
        this.onShow = options.onShow;
        this.onFinishShowing = options.onFinishShowing;
        
        let space = this.getSpace();
        let name = this.getName();
        
        if (space) {
            this.showParent();
            if (space.active != name) {
                for (const [key, areaData] of Object.entries(space.areas)) {
                    if (areaData.element) {
                        if (name != key) {
                            areaData.element.startHiding();
                            DD_Area.deactivateTabs(areaData.tabs);
                        } else {
                            areaData.element.startUnhiding();
                            DD_Area.activateTabs(areaData.tabs);
                            space.active = key;
                        }
                    }
                }
            } else {
                this.unhide();
            }
            
            if (options.navHistoryAction != "nothing") {
                //DD_Area.updateNavigationHistory(options.navHistoryAction);
            }
        }
    }
    
    getParentArea() {
        let parent = this.parentElement;
        while (parent && parent.tagName != "DD-AREA") {
            parent = parent.parentElement;
        }
        return parent;
    }
    
    showParent() {
        let parent = this.getParentArea();
        if (parent) {
            parent.show({navHistoryAction: "nothing"});
        }
    }
    
    hide() {
        this.classList.add("dd-hidden");
        this.classList.remove("dd-unhiding");
        this.classList.remove("dd-hiding");
        
        if (this.hasAttribute("dd-onhide")) {
            let onhide = this.getAttribute("dd-onhide");
            DD_Components.executeFunctionByName(onhide, window);
        }
    }
    
    startHiding() {
        if (this.hasAttribute("dd-onhidestart")) {
            let func = this.getAttribute("dd-onhidestart");
            DD_Components.executeFunctionByName(func, window);
        }
        
        if (this.hasAttribute("dd-fade")) {
            if (!this.classList.contains("dd-hiding")) {
                this.style.setProperty("--fade-speed", this.getAttribute("dd-fade"));
                
                if (this.offsetParent != null) {
                    this.classList.remove("dd-unhiding");
                    this.classList.add("dd-hiding");
                    
                    function afterTransition(event) {
                        this.hide();
                    }
                    
                    this.ontransitionend = afterTransition;
                    this.ontransitioncancel = afterTransition;
                } else {
                    this.hide();
                }
            } else {
                this.hide();
            }
        } else {
            this.hide();
        }
    }   
    
    unhide() {
        this.classList.remove("dd-hidden");
        this.classList.remove("dd-unhiding");
        this.classList.remove("dd-hiding");
        
        this.onShow();
        this.onShow = function(){};
        
        if (this.hasAttribute("dd-onshow")) {
            DD_Components.executeFunctionByName(this.getAttribute("dd-onshow"));
        }
        
        if (this.onFinishShowing) {
            let opacity = parseFloat(getComputedStyle(this).opacity);
            if (opacity != 1.0) {
                this.addEventListener("transitionend", function() {
                    if (this.onFinishShowing) {
                        this.onFinishShowing();
                        this.onFinishShowing = null;
                    }
                }, {once: true});
            } else {
                this.onFinishShowing();
                this.onFinishShowing = null;
            }
        }
    }
    
    startUnhiding() {
        if (this.hasAttribute("dd-fade")) {
            if (!this.classList.contains("dd-unhiding")) {
                this.style.setProperty("--fade-speed", this.getAttribute("dd-fade"));
                
                let delay = 300;
                let activeArea = DD_Area.getActiveAreaData(this.getSpaceName());
                
                if (activeArea && !activeArea.element.classList.contains("dd-hidden")) {
                    let fs = getComputedStyle(this).getPropertyValue("--fade-speed");
                    
                    if (!fs) {
                        fs = "300ms";
                    }
                    
                    let toMS = 1;
                    if (fs.endsWith("ms")) {
                        toMS = 1;
                    } else if (fs.endsWith("s")) {
                        toMS = 1000.0;
                    }
                    
                    delay = parseFloat(fs) * toMS;
                }
                
                this.classList.remove("dd-hiding");
                this.classList.add("dd-unhiding");
                
                this.ontransitionend = undefined;
                this.ontransitioncancel = undefined;
                
                let area = this;
                
                setTimeout(function() {
                    if (area.classList.contains("dd-unhiding")) {
                        area.classList.remove("dd-hidden");
                        
                        setTimeout(function() {
                            area.hideAllOthers();
                            area.unhide();
                        }, 30);
                    } else if (area.parentOffset == null) {
                        area.hideAllOthers();
                        area.unhide();
                    }
                }, delay);
            } else {
                this.hideAllOthers();
                this.unhide();
            }
        } else {
            this.hideAllOthers();
            this.unhide();
        }
    }
    
    toggle() {
        this.classList.toggle("dd-hidden");
    }
    
    getSpaceName() {
        if (this.hasAttribute("dd-space")) {
            return this.getAttribute("dd-space");
        } else {
            return "";
        }
    }
    
    getSpace() {
        return DD_Area.getSpace(this.getSpaceName());
    }
    
    getName() {
        return this.getAttribute("dd-name");
    }
    
    isActive() {
        let space = this.getSpace();
        return space.active == this.getName();
    }
    
    isInvisible() {
        if (this.classList.contains("dd-unhiding")) return false;
        if (!this.isActive()) return true;
        
        let el = this;
        
        while (el) {
            if (el.tagName == "DD-AREA")  {
                if (!el.classList.contains("dd-unhiding") && (el.classList.contains("dd-hiding") || el.classList.contains("dd-hidden"))) {
                    return true;
                }
            }
            
            el = el.parentElement;
        }
        
        return false;
    }
    
    isVisible() {
        return !this.isInvisible();
    }
    
    getId() {
        if (this.getSpaceName() != "") {
            return this.getSpaceName() + "-" + this.getName();
        } else {
            return this.getName();
        }
    }
    
    descendsFromAny(elements) {
        for (let el of elements) {
            if (el.contains(this))
                return el;
        }
        return null;
    }
    
    containsAny(elements) {
        for (let el of elements) {
            if (this.contains(el)) {
                return el;
            }
        }
        return null;
    }
    
    hideAllOthers() {
        let space = this.getSpace();
        let name = this.getName();
        if (space) {
            for (const [key, areaData] of Object.entries(space.areas)) {
                if (key == name) continue;
                let elem = areaData.element;
                if (elem && !elem.classList.contains("dd-hidden") && !elem.classList.contains("dd-hiding")) {
                    elem.hide();
                }
            }
        }
    }
    
    setSearchParams(params) {
        let currentParams = undefined;
        if (this.hasAttribute("dd-search-params")) {
            currentParams = this.getAttribute("dd-search-params");
        }
        
        this.setAttribute("dd-search-params", params);
        
        if (currentParams != undefined && currentParams != params) {
            if (this.isVisible()) {
                this.updateNavigationHistory("auto");
            }
        }
    }
    
    static isShowing(id) {
        let area = DD_Area.getArea(id);
        if (area) {
            return area.element.isVisible();
        }
        return undefined;
    }
    
    static getSpace(spaceName) {
        if (spaceName in DD_Area.spaces) {
            return DD_Area.spaces[spaceName];
        } else {
            return undefined;
        }
    }
    
    static hideAllAreas(spaceName) {
        let space = DD_Area.getSpace(spaceName);
        if (space) {
            for (const [key, areaData] of Object.entries(space.areas)) {
                if (areaData.element) {
                    areaData.element.hide();
                    DD_Area.deactivateTabs(areaData.tabs);
                }
            }
            space.active = undefined;
        }
    }
    
    static getArea(id) {
        let spaceName = "";
        let areaName = "";
        
        if (id.indexOf("-") >= 0) {
            let values = id.split("-");
            spaceName = values[0];
            areaName = values[1];
        } else {
            areaName = id;
        }
        
        let space = DD_Area.getSpace(spaceName);
        if (space != undefined && areaName in space.areas) {
            return space.areas[areaName];
        } else {
            return undefined;
        }
    }
    
    static getAreaElement(spaceName, areaName) {
        let area = DD_Area.getArea(spaceName + "-" + areaName);
        if (area) {
            return area.element;
        } else {
            return undefined;
        }
    }
    
    static get(id) {
        let area = DD_Area.getArea(id);
        if (area) {
            return area.element;
        } else {
            return undefined;
        }
    }
    
    static getAreaTabs(spaceName, areaName) {
        let area = DD_Area.getArea(spaceName + "-" + areaName);
        if (area) {
            return area.tabs;
        } else {
            return undefined;
        }
    }
    
    static addSpace(spaceName) {
        if (!DD_Area.getSpace(spaceName)) {
            DD_Area.spaces[spaceName] = {
                areas: [],
                active: undefined
            };
        }
        
        return DD_Area.spaces[spaceName];
    }
    
    static addArea(spaceName, areaName, element) {
        let space = DD_Area.addSpace(spaceName);
        
        if (!(areaName in space.areas)) {
            space.areas[areaName] = {
                element: element,
                tabs: []
            };
        } else if (element) {
            space.areas[areaName].element = element;
        }
    }
    
    static addAreaTab(spaceName, areaName, element) {
        let tabs = DD_Area.getAreaTabs(spaceName, areaName);
        if (tabs && !tabs.includes(element)) {
            tabs.push(element);
        }
    }
    
    static show(id, options) {
        let spaceName = "";
        let areaName = "";
        
        if (id.indexOf("-") >= 0) {
            let values = id.split("-");
            spaceName = values[0];
            areaName = values[1];
        } else {
            areaName = id;
        }
        
        let area = DD_Area.getAreaElement(spaceName, areaName);
        if (area) {
            area.show(options);
        }
    }
    
    static showAreasInURL(options) {
        if (options == undefined) options = {};
        if (!("navHistoryAction" in options)) options.navHistoryAction = "nothing";
        
        let searchParams = new URLSearchParams(window.location.search);
        let shows = searchParams.getAll("show");
        for (let show of shows) {
            DD_Area.show(show, options);
        }
    }
    
    static activateTabs(tabs) {
        for (let tab of tabs) {
            tab.activate();
        }
    }
    
    static deactivateTabs(tabs) {
        for (let tab of tabs) {
            tab.deactivate();
        }
    }
    
    static getActiveAreaData(spaceName) {
        let space = DD_Area.getSpace(spaceName);
        if (space && space.active) {
            return space.areas[space.active];
        } else {
            return undefined;
        }
    }
    
    static setActiveArea(spaceName, areaName) {
        let space = DD_Area.getSpace(spaceName);
        space.active = areaName;
        
        let areaData = DD_Area.getActiveAreaData(spaceName);
        DD_Area.activateTabs(areaData.tabs);
    }
    
    setActive() {
        DD_Area.setActiveArea(this.getSpaceName(), this.getName());
    }
    
    static updateNavigationHistory(action) {
        if (!DD_Area.DOMContentLoaded) return;
        
        // Get all areas that need to be in the new search string
        let showElements = [];
        
        for (const [spaceName, space] of Object.entries(DD_Area.spaces)) {
            for (const [areaName, area] of Object.entries(space.areas)) {
                if (area.element && area.element.isVisible()) {
                    let el = area.element.descendsFromAny(showElements);
                    
                    if (el) DD_Components.removeItemFromArrayIfCondition(showElements, (entry) => entry == el);
                    
                    if (!area.element.containsAny(showElements)) {
                        showElements.push(area.element);
                    }
                }
            }
        }
        
        // Compare the new and current search strings and decide if we should push,
        // update or do nothing with the URL.
        let currentSearchParams = new URLSearchParams(window.location.search);
        let newSearchParams = new URLSearchParams();
        
        for (let el of showElements) {
            if (!el.hasAttribute("dd-search-params")) {
                newSearchParams.append("show", el.getId());
            } else {
                let custom = new URLSearchParams(el.getAttribute("dd-search-params"));
                for (const [key, value] of custom) {
                    newSearchParams.append(key, value);
                }
            }
        }
        
        currentSearchParams.sort();
        newSearchParams.sort();
        
        let newSearchParamsString = newSearchParams.toString();
        
        let newURL = "";
        if (newSearchParamsString.length) {
            newURL = window.location.origin + window.location.pathname + "?" + newSearchParams.toString();
        } else {
            newURL = window.location.origin + window.location.pathname;
        }
        
        if (currentSearchParams.toString() != newSearchParams.toString()) {
            if (action == "auto") {
                action = "push";
            }
        } else {
            action = "nothing";
        }
        
        if (action == "push") {
            DD_Area.pushToNavigationHistory(newURL);
        } else if (action == "update") {
            window.history.replaceState(DD_Area.historyNavigator.toString(), "", newURL);
        }
    }
    
    static pushToNavigationHistory(newURL) {
        ++DD_Area.historyNavigator;
        window.history.pushState(DD_Area.historyNavigator.toString(), "", newURL);
    }
    
    static locationContainsArea() {
        let searchParams = new URLSearchParams(window.location.search);
        let ids = searchParams.getAll("show");
        if (ids.length) {
            for (let id of ids) {
                if (DD_Area.getArea(id)) {
                    return true;
                }
            }
        }
        return false;
    }
}

DD_Area.spaces = {};
DD_Area.DOMContentLoaded = false;

(function() {
    if (document.currentScript.hasAttribute("dd-onlocationupdate")) {
        DD_Area.onLocationUpdate = document.currentScript.getAttribute("dd-onlocationupdate");
    }
})();

/* INPUT */
/* INPUT */
/* INPUT */

class DD_Input extends HTMLElement {
    static observedAttributes = ["value", "disabled", "placeholder"];

    constructor() {
        super();

        this.selectMenu = null;
        this.inputContainer = document.createElement("div");
        this.inputContainer.classList.add("dd-input-container");
        this.input = document.createElement("input");

        const thisInput = this;

        this.input.addEventListener("input", function(event) {
            if (thisInput.selectMenu) {
                thisInput.updateOptionsOrder();

                const inputValue = thisInput.input.value;
                if (inputValue) {
                    if (!thisInput.selectMenu.isShowing()) {
                        thisInput.selectMenu.show(event, false);
                    }
                    thisInput.clearButton.classList.add("dd-show");
                } else {
                    if (thisInput.selectMenu.getSelected().length == 0) {
                        thisInput.clearButton.classList.remove("dd-show");
                    }
                }
            }
        });

        this.clearButton = document.createElement("dd-button");
        this.clearButton.innerHTML = "×";
        this.clearButton.classList.add("dd-clear-button");
        this.clearButton.addEventListener("click", function(event) {
            if (thisInput.selectMenu) {
                thisInput.selectMenu.setValue([]);
            } else {
                thisInput.updateSelectionView();
            }
            event.preventDefault();
            event.stopPropagation();
        });

        this.inputContainer.appendChild(this.input);
    }
    
    connectedCallback() {
        const thisElem = this;

        if (this.hasAttribute("dd-reconnecting")) {
            this.removeAttribute("dd-reconnecting");
            return;
        }

        DD_Components.setStylePropertiesFromAttributes(this, DD_Input.attributeMap);

        if (this.hasAttribute("placeholder")) {
            this.input.setAttribute("placeholder", this.getAttribute("placeholder"));
        }

        if (this.hasAttribute("oninput")) {
            this.input.setAttribute("oninput", this.getAttribute("oninput"));
            this.setAttribute("oninput", "");
        }

        if (this.hasAttribute("onchange")) {
            this.input.setAttribute("onchange", this.getAttribute("onchange"));
            this.setAttribute("onchange", "");
        }

        if (this.hasAttribute("value") && this.input.value != this.getAttribute("value")) {
            this.input.value = this.getAttribute("value");
        }

        if (this.hasAttribute("disabled")) {
            this.input.setAttribute("disabled", this.getAttribute("disabled"));
        }

        this.appendChild(this.inputContainer);
        this.appendChild(this.clearButton);
    }
    
    attributeChangedCallback(name, oldValue, newValue) {
        if (name == "value") {
            this.input.value = newValue;
        } else if (["disabled", "placeholder"].includes(name)) {
            if (newValue != null) {
                this.input.setAttribute(name, newValue);
            } else {
                this.input.removeAttribute(name);
            }
        }
    }
    
    get value() {
        return this.input.value;
    }
    
    set value(v) {
        this.input.value = v;
    }

    updateSelectionView() {
        while (this.inputContainer.firstChild != this.input) {
            this.inputContainer.firstChild.remove();
        }

        let selection = this.selectMenu.getSelected();
        if (!this.selectMenu.hasAttribute("dd-multiple")) {
            if (selection) {
                selection = [selection];
            } else {
                selection = [];
            }
        }

        let thisElem = this;

        if (this.selectMenu.hasAttribute("dd-multiple")) {
            for (let op of selection.slice().reverse()) {
                const elem = document.createElement("dd-selected-option");
                elem.innerText = this.selectMenu.getOptionText(op);

                const removeButton = document.createElement("dd-button");
                removeButton.innerHTML = "×";

                removeButton.addEventListener("click", function(event) {
                    thisElem.selectMenu.deselectOption(op);
                    event.stopPropagation();
                });

                elem.appendChild(removeButton);

                this.inputContainer.prepend(elem);
            }
        }

        if (selection.length == 0) {
            this.clearButton.classList.remove("dd-show");
        } else {
            this.clearButton.classList.add("dd-show");
        }

        this.input.value = "";

        if (!this.selectMenu.hasAttribute("dd-multiple")) {
            let placeholder = "";
            if (selection.length > 0) placeholder = this.selectMenu.getOptionText(selection[0]);
            this.setAttribute("placeholder", placeholder);
        }
    }

    saveOptionsOrder() {
        this.optionsOriginalOrder = Array.from(this.selectMenu.querySelectorAll("dd-option"));
    }

    restoreOptionsOrder() {
        for (const op of this.optionsOriginalOrder.slice().reverse()) {
            this.selectMenu.prepend(op);
        }
    }

    updateOptionsOrder() {
        const inputValue = this.input.value;

        if (inputValue.length) {
            let optionWeights = [];
            for (const elem of this.optionsOriginalOrder) {
                const w = DD_Components.getLevenshteinDistance(elem.getAttribute("dd-text").toLowerCase(), inputValue.toLowerCase());
                optionWeights.push({elem: elem, weight: w});
            }

            optionWeights.sort((a, b) => a.weight - b.weight);

            let options = this.selectMenu.querySelectorAll("dd-option");

            for (const op of optionWeights.slice().reverse()) {
                this.selectMenu.prepend(op.elem);
            }
        } else {
            this.restoreOptionsOrder();
        }

        if (this.selectMenu.hasAttribute("dd-multiple")) {
            for (const elem of this.optionsOriginalOrder) {
                if (this.selectMenu.isSelected(elem.value)) {
                    elem.classList.add("dd-hide");
                } else {
                    elem.classList.remove("dd-hide");
                }
            }
        }
    }

    bindSelectMenu(selectElem) {
        this.selectMenu = selectElem;
    }
}

class DD_SelectedOption extends HTMLElement {}

DD_Input.attributeMap = {
    "dd-border-top-left-radius" : "--input-border-top-left-radius",
    "dd-border-top-right-radius" : "--input-border-top-right-radius",
    "dd-border-bottom-right-radius" : "--input-border-bottom-right-radius",
    "dd-border-bottom-left-radius" : "--input-border-bottom-left-radius",

    "dd-border-color" : "--input-border-color",
    "dd-border-style" : "--input-border-style",
    "dd-border-width" : "--input-border-width",

    "dd-display" : "--input-display",
    "dd-font-size" : "--input-font-size",
    "dd-line-height" : "--input-line-height",
    "dd-outline" : "--input-outline",
    "dd-max-width" : "--input-max-width",
    "dd-max-height" : "--input-max-height",
    
    "dd-width" : "--input-width",
    
    "dd-border-radius" : "--input-border-radius",
    
    "dd-bg" : "--input-bg",
    "dd-padding" : "--input-padding",
    
    "dd-focus-border-color" : "--input-focus-border-color",
    "dd-focus-outline" : "--input-focus-outline"
};
    
/* TABLE */
/* TABLE */
/* TABLE */

class DD_Table extends HTMLElement {
    constructor() {
        super();
        this.filters = [];
        this.sorters = [];
        this.rows = [];
        this.updateScheduled = false;
    }
    
    connectedCallback() {
        DD_Components.setStylePropertiesFromAttributes(this, DD_Table.attributeMap);
        
        this.updateDefaultSorter();
    }
    
    updateDefaultSorter() {
        let sorter = DD_Components.getSorter(this);
        if (sorter) {
            this.removeSorterByHeader(this);
            this.addSorter(sorter);
        }
    }
    
    addFilter(options) {
        if (!("filter" in options)) return;
        if (!("arg" in options)) options.arg = "row";
        
        this.filters.push(options);
    }
    
    clearFilters() {
        this.filters = [];
    }
    
    addSorter(options) {
        if (!("sorter" in options)) return;
        if (!("arg" in options)) options.arg = "row";
        if (!("header" in options)) options.header = null;
        
        this.sorters.push(options);
    }
    
    removeSorterByHeader(header) {
        DD_Components.removeItemFromArrayIfCondition(this.sorters, (entry) => entry.header == header);
    }
    
    removeSorter(sorterFunc) {
        DD_Components.removeItemFromArrayIfCondition(this.sorters, (entry) => entry.sorter == sorterFunc);
    }
    
    addRow(row) {
        if (this.isSorting) return;
        
        this.rows.push(row);
        if (this.rows.length == 1) {
            this.updateDefaultSorter();
        }
        
        this.scheduleUpdateIfNeeded();
    }
    
    removeRow(row) {
        if (this.isSorting) return;
        
        DD_Components.removeItemFromArrayIfCondition(this.rows, (entry) => entry == row);
        this.scheduleUpdateIfNeeded();
    }
    
    scheduleUpdateIfNeeded() {
        if (!this.updateScheduled) {
            let table = this;
            requestAnimationFrame(function() {
                table.update();
                table.updateScheduled = false;
            });
            this.updateScheduled = true;
        }
    }
    
    applyFilters() {
        let rows = this.rows;
        
        let result = {
            retained: [],
            passed: []
        };
        
        for (let row of rows) {
            let include = true;
            for (let filter of this.filters) {
                let arg = null;
                if (filter.arg == "associated-data") {
                    arg = row.associatedData;
                } else if (filter.arg == "row") {
                    arg = row;
                }
                
                if (!filter.filter(arg)) {
                    include = false;
                    break;
                }
            }
            
            if (include) {
                row.classList.remove("dd-filtered");
                result.passed.push(row);
            } else {
                row.classList.add("dd-filtered");
                result.retained.push(row);
            }
        }
        
        return result;
    }
    
    applySorters() {
        let rows = this.rows;
        
        if (rows.length && this.sorters.length) {
            let referenceNodeForReinsertion = rows[rows.length-1].nextSibling;
            let parentElem = rows[rows.length-1].parentElement;
            
            for (let sorter of this.sorters) {
                if (sorter.arg == "associated-data") {
                    rows.sort((row1, row2) => sorter.sorter(row1.associatedData, row2.associatedData));
                } else if (sorter.arg == "row") {
                    rows.sort((row1, row2) => sorter.sorter(row1, row2));
                }
            }
            
            this.isSorting = true;
            let fragment = document.createDocumentFragment(); 
            fragment.textContent = ' ';
            fragment.firstChild.replaceWith(...rows);
            
            parentElem.insertBefore(fragment, referenceNodeForReinsertion);
            this.isSorting = false;
        }
    }
    
    update() {
        this.applySorters();
        let result = this.applyFilters();
        
        let filteredPlaceholder = this.querySelector(":scope > .dd-filtered-placeholder");
        
        if (filteredPlaceholder) {
            if (result.passed.length == 0 && result.retained.length > 0) {
                filteredPlaceholder.classList.remove("dd-hidden");
            } else {
                filteredPlaceholder.classList.add("dd-hidden");
            }
        }
        
        let emptyPlaceholder = this.querySelector(":scope > .dd-empty-placeholder");
        
        if (emptyPlaceholder) {
            if (result.passed.length == 0 && result.retained.length == 0) {
                emptyPlaceholder.classList.remove("dd-hidden");
            } else {
                emptyPlaceholder.classList.add("dd-hidden");
            }
        }
    }    
    
    isEmpty() {
        return this.rows.length == 0;
    }
    
    getDataType(dataPath) {
        if (!this.isEmpty()) {
            let row = this.rows[0];
            let obj = DD_Components.getAssociatedData(row);
            let something = DD_Components.getSomethingByPath(dataPath, obj);
            if (something) {
                return typeof something.something;
            } else {
                return undefined;
            }
        } else {
            return undefined;
        }
    }
}
    
DD_Table.attributeMap = {
    "dd-c1" : "--c1",
    "dd-c2" : "--c2",
    "dd-c3" : "--c3",
    "dd-c4" : "--c4",

    "dd-c1-display" : "--c1-display",
    "dd-c2-display" : "--c2-display",

    "dd-c1-color" : "--c1-color",
    "dd-c2-color" : "--c2-color",

    "dd-body-bg" : "--body-bg",
    "dd-body-color" : "--body-color",

    "dd-row-cursor" : "--row-cursor",
    "dd-row-hover-bg" : "--row-hover-bg",

    "dd-cell-display" : "--cell-display",
    "dd-cell-padding" : "--cell-padding",
    "dd-cell-bg" : "--cell-bg",
    "dd-cell-color" : "--cell-color",

    "dd-cell-hover-bg" : "--cell-hover-bg",

    "dd-header-bg" : "--header-bg",
    "dd-header-color" : "--header-color",
    "dd-header-padding" : "--header-padding",
    "dd-header-font-weight" : "--header-font-weight",
    "dd-header-font-size" : "--header-font-size",
    "dd-header-text-transform" : "--header-text-transform",
    "dd-header-cell-hover-bg" : "--header-cell-hover-bg"
};
    
class DD_TableHeader extends HTMLElement {}
class DD_TableBody extends HTMLElement {}

class DD_Row extends HTMLElement {
    connectedCallback() {
        this.table = this.getTable();
        if (this.table) {
            this.table.addRow(this);
        }
    }
    
    disconnectedCallback() {
        if (this.table) {
            this.table.removeRow(this);
        }
    }
    
    getTable() {
        let el = this.parentElement;
        while (el && el.tagName != "DD-TABLE") {
            el = el.parentElement;
        }
        return el;
    }
}

class DD_Cell extends HTMLElement {
    connectedCallback() {
        if (this.hasAttribute("dd-data")) {
            let dataElem = document.createElement("dd-data");
            dataElem.setAttribute("dd-data", this.getAttribute("dd-data"));
            this.appendChild(dataElem);
        }
        
        if (this.hasAttribute("dd-sorter")) {            
            DD_Components.setClickable(this);
            
            this.addEventListener("click", function(event) {
                this.switchSortOrder();
                this.updateSortIcon();
                this.applySorter();
                let table = this.getTable();
                table.update();
            });
        } else if (this.hasAttribute("dd-clickable")) {
            DD_Components.setClickable(this);
        }
    }
    
    switchSortOrder() {
        let newOrder = undefined;
        
        if (this.hasAttribute("dd-sort")) {
            let order = this.getAttribute("dd-sort");
            if (order == "ascending") {
                newOrder = "descending";
            } else if (order == "descending") {
                newOrder = undefined;
            }
        } else {
            newOrder = "ascending";
        }
        
        if (newOrder != undefined) {
            this.setAttribute("dd-sort", newOrder);
        } else {
            this.removeAttribute("dd-sort");
        }
        return newOrder;
    }
    
    applySorter() {
        let table = this.getTable();
        table.removeSorterByHeader(this);
        
        if (this.hasAttribute("dd-sort")) {
            let sorter = DD_Components.getSorter(this);
            table.addSorter(sorter);
        }
    }
    
    updateSortIcon() {
        let icon = this.querySelector(":scope .dd-sort-icon");
        if (icon) icon.remove();
        
        if (this.hasAttribute("dd-sort")) {
            let order = this.getAttribute("dd-sort");
            
            let iconDataType = this.getDataType();
            if (this.hasAttribute("dd-sort-icon")) {
                if (this.getAttribute("dd-sort-icon") == "generic") {
                    iconDataType = undefined;
                }
            }
            
            icon = DD_Components.createSortIcon(iconDataType, order);
            this.appendChild(icon);
        }
    }
    
    getTable() {
        let el = this.parentElement;
        while (el && el.tagName != "DD-TABLE") {
            el = el.parentElement;
        }
        return el;
    }
    
    getDataType() {
        let sorter = this.getAttribute("dd-sorter");
        if (sorter.startsWith("[*].")) {
            let dataPath = sorter.substring(4);
            return this.getTable().getDataType(dataPath);
        }
        return undefined;
    }
    
    getRow() {
        let el = this.parentElement;
        while (el && el.tagName != "DD-ROW") {
            el = el.parentElement;
        }
        return el;
    }
    
    getAssociatedData() {
        let el = this;
        while (el && !("associatedData" in el)) {
            el = el.parentElement;
        }
        
        if (el) return el.associatedData;
        else return undefined;
    }
    
    getSorter() {
        return DD_Components.getSorter(this);
    }
}

/* TOOLTIP */
/* TOOLTIP */
/* TOOLTIP */

class DD_Tooltip extends HTMLElement {
    initializeTooltip() {
        if (this.parentElement != document.body) {
            this.anchor = this.parentElement;
            this.initializeCSSProperties();
            document.body.appendChild(this);
            return;
        }
        
        if (this.hasAttribute("dd-data")) {
            let dataElem = document.createElement("dd-data");
            dataElem.setAttribute("dd-data", this.getAttribute("dd-data"));
            this.appendChild(dataElem);
        }
        
        this.anchorMouseEnterHandler = DD_Tooltip.genAnchorMouseEnterHandler(this);
        this.anchorMouseLeaveHandler = DD_Tooltip.genAnchorMouseLeaveHandler(this);
        
        this.anchor.addEventListener("mouseenter", this.anchorMouseEnterHandler);
        this.anchor.addEventListener("mouseleave", this.anchorMouseLeaveHandler);
        
        let tooltip = this;
        
        this.anchor.addEventListener("click", function() {
            if (tooltip.isShowing()) {
                tooltip.startHiding();
            }
        });
        
        DD_Tooltip.all.push(this);
    }
    
    initializeCSSProperty(propName, anchorStyle) {
        let value = anchorStyle.getPropertyValue(propName);
        if (value) {
            this.style.setProperty(propName, value);
        }
    }
    
    initializeCSSProperties() {
        let style = getComputedStyle(this);
        
        let propertyNames = ["--tooltip-max-width", "--tooltip-white-space", "--tooltip-font-family", "--tooltip-font-size", "--tooltip-font-weight", 
                             "--tooltip-padding", "--tooltip-border-radius", "--tooltip-text-transform", "--tooltip-delay", "--tooltip-text-align",
                             "--tooltip-position", "--tooltip-offset", "--tooltip-bg", "--tooltip-color", "--tooltip-before-content", "--tooltip-after-content"];
        
        for (let p of propertyNames) {
            this.initializeCSSProperty(p, style);
        }
    }
    
    connectedCallback() {
        this.initializeTooltip();
    }
    
    disconnectedCallback() {
        DD_Components.removeItemFromArrayIfCondition(DD_Tooltip.all, (entry) => entry == this);
    }
    
    isShowing() {
        return this.classList.contains("dd-showing");
    }
    
    isAnchorConnected() {
        return document.body.contains(this.anchor);
    }
    
    updatePosition() {
        let anchor = this.anchor;
        let boundingRect = anchor.getBoundingClientRect();
        let p = DD_Components.getPositionRelativeToPage(anchor);
        
        let anchorRect = {
            top: p.top,
            left: p.left,
            width: boundingRect.width,
            height: boundingRect.height,
            cx: p.left + boundingRect.width/2,
            cy: p.top + boundingRect.height/2
        };
        
        let style = getComputedStyle(this);
        
        let position = style.getPropertyValue("--tooltip-position").trim();
        let offset = style.getPropertyValue("--tooltip-offset").trim();
        let edgesOffset = style.getPropertyValue("--tooltip-edges-offset").trim();
        let alignment = style.getPropertyValue("--tooltip-alignment").trim();
        
        if (!position) position = "top";
        if (!offset) offset = ".5em";
        edgesOffset = edgesOffset ? parseFloat(edgesOffset) : 5;
        
        this.style.top = "";
        this.style.left = "";
        
        if (position == "top" || position == "bottom") {
            let tooltipRect = this.getBoundingClientRect();
            let leftCorrection = Math.max(0, tooltipRect.width/2 - anchorRect.cx + edgesOffset);
            
            if (leftCorrection == 0) {
                let vw = document.documentElement.clientWidth;
                let vh = document.documentElement.clientHeight;
                
                leftCorrection = -Math.max(0, anchorRect.cx + tooltipRect.width/2 - vw + edgesOffset);
            }
            
            this.style.left = `${anchorRect.left + anchorRect.width/2 + leftCorrection}px`;
        }
        
        if (position == "top") {
            this.style.top = `calc(${anchorRect.top}px - ${offset})`;
            this.style.transform = "translateY(-100%) translateX(-50%)";
        } else if (position == "bottom") {
            this.style.top = `calc(${anchorRect.top + anchorRect.height}px + ${offset})`;
            this.style.transform = "translateX(-50%)";
        } else if (position == "right") {
            this.style.top = `${anchorRect.cy}px`;
            this.style.left = `calc(${anchorRect.left + anchorRect.width}px + ${offset})`;
            this.style.transform = "translateY(-50%)";
        } else if (position == "left") {
            this.style.left = `${anchorRect.left}px`;
        }
    }
    
    show() {
        if (this.anchor.hasAttribute("dd-tooltip")) {
            let tooltipContent = this.anchor.getAttribute("dd-tooltip");
            if (tooltipContent) {
                this.innerHTML = this.anchor.getAttribute("dd-tooltip");
            } else {
                return;
            }
        }
        
        if (!this.classList.contains("dd-showing")) {
            this.classList.add("dd-showing");
            this.classList.add("dd-transparent");
            this.ontransitionend = null;
            this.ontransitioncancel = null;
            let tooltip = this;
            requestAnimationFrame(() => {tooltip.classList.remove("dd-transparent")});
            this.updatePosition();
        }
    }
    
    hide() {
        this.classList.remove("dd-showing");
        this.classList.remove("dd-transparent");
    }
    
    startHiding() {
        let style = getComputedStyle(this);
        
        if (style.opacity > 0) {
            this.classList.add("dd-transparent");
            this.ontransitionend = () => {this.hide()};
            this.ontransitioncancel = this.ontransitionend;
        } else {
            this.hide();
        }
    }
    
    static genAnchorMouseEnterHandler(tooltipElem) {
        return function(event) {
            tooltipElem.show();
        }
    }
    
    static genAnchorMouseLeaveHandler(tooltipElem) {
        return function(event) {
            tooltipElem.startHiding();
        }
    }
};

DD_Tooltip.all = [];

/* MENU */
/* MENU */
/* MENU */

class DD_Menu extends HTMLElement {
    initializeMenu() {
        this.addEventListener("keydown", function(event) {
            if (event.target == this && DD_Components.isKeyTab(event.keyCode) && event.shiftKey) {
                event.preventDefault();
                this.anchor.focus();
            }
        });
        
        let menuElem = this;
        DD_Components.setFocusable(menuElem);
        
        // TODO: Currently, the menu element is considered to be already initialized if
        // its parent is already the document body element. However, we should be able
        // to place menus at the body. But that would not work because once we reach
        // the code below, we will consider that the element is already initialized.
        if (menuElem.parentElement != document.body) {
            menuElem.originalParent = menuElem.parentElement;
            menuElem.originalPreviousSibling = menuElem.previousElementSibling;
            document.body.appendChild(menuElem);
            return;
        }
        
        let anchor = null;
        
        if (menuElem.hasAttribute("dd-context-menu")) {
            this.windowClickHandler = DD_Menu.genWindowClickHandler(this);
            this.windowKeyDownHandler = DD_Menu.genWindowKeyDownHandler(this);
        } else if (menuElem.hasAttribute("dd-anchor")) {
            let anchorSelector = menuElem.getAttribute("dd-anchor");
            anchor = document.querySelector(anchorSelector);
        } else {
            anchor = menuElem.originalPreviousSibling;
        }
        
        if (anchor) {
            this.anchor = anchor;
            if (anchor.tagName.includes("BUTTON")) {
                this.anchorType = "BUTTON";
            } else if (anchor.tagName.includes("INPUT")) {
                this.anchorType = "INPUT";
            }
            
            let trigger = this.getAttribute("dd-trigger");
            
            if (trigger == "hover") {
                this.anchorMouseEnterHandler = DD_Menu.genAnchorMouseEnterHandler(this);
                this.anchorMouseLeaveHandler = DD_Menu.genAnchorMouseLeaveHandler(this);
                
                this.addEventListener("mouseenter", function() {
                    this.classList.add("dd-showing");
                });
                
                this.addEventListener("mouseleave", function() {
                    this.hide();
                });
                
                anchor.addEventListener("mouseenter", this.anchorMouseEnterHandler);
                anchor.addEventListener("mouseleave", this.anchorMouseLeaveHandler);
            } else {
                this.windowClickHandler = DD_Menu.genWindowClickHandler(this);
                this.windowKeyDownHandler = DD_Menu.genWindowKeyDownHandler(this);
                this.anchorKeyDownHandler = DD_Menu.genAnchorKeyDownHandler(this);
                this.anchorClickHandler = DD_Menu.genAnchorClickHandler(this);

                anchor.addEventListener("keydown", this.anchorKeyDownHandler);

                if (this.anchorType == "BUTTON") {
                    if (!this.hasAttribute("dd-custom-open")) {
                        anchor.addEventListener("click", this.anchorClickHandler);
                    }
                } else if (this.anchorType == "INPUT") {
                    this.anchorFocusHandler = DD_Menu.genAnchorFocusHandler(this);
                    this.anchorBlurHandler = DD_Menu.genAnchorBlurHandler(this);

                    if (!this.hasAttribute("dd-custom-open")) {
                        anchor.input.addEventListener("focus", this.anchorFocusHandler);
                        anchor.addEventListener("click", this.anchorClickHandler);
                    }

                    anchor.input.addEventListener("blur", this.anchorBlurHandler);
                }
            }
            
            this.anchor.getMenu = () => this;
        }
        
        DD_Menu.all.push(this);
    }
    
    connectedCallback() {
        this.initializeMenu();
        this.addEventListener("click", function(event) {
            this.hide();
        });
    }
    
    disconnectedCallback() {
        DD_Components.removeItemFromArrayIfCondition(DD_Menu.all, (entry) => entry == this);
    }
    
    isAnchorConnected() {
        return document.body.contains(this.anchor);
    }
    
    isShowing() {
        return this.classList.contains("dd-showing");
    }
    
    updatePositionAndDimensions() {
        let anchorRect = {};
        
        if (this.hasAttribute("dd-context-menu")) {
            anchorRect = {
                top: DD_Components.mouse.pageY,
                left: DD_Components.mouse.pageX,
                width: 0,
                height: 0
            };
        } else if (this.anchor) {
            let anchorBoundingRect = this.anchor.getBoundingClientRect();
            let p = DD_Components.getPositionRelativeToPage(this.anchor);
            anchorRect = {
                top: p.top,
                left: p.left,
                bottom: p.top + anchorBoundingRect.height,
                width: anchorBoundingRect.width,
                height: anchorBoundingRect.height
            };
        }

        const spaceBelow = window.innerHeight - anchorRect.bottom;
        const spaceAbove = anchorRect.top;
        let upwards = false;

        if (spaceBelow >= 200) {
            this.style.maxHeight = `${spaceBelow}px`;
        } else {
            if (spaceAbove > spaceBelow) {
                upwards = true;
                this.style.maxHeight = `${spaceAbove}px`;
            } else {
                this.style.maxHeight = `${spaceBelow}px`;
            }
        }

        let alignment = this.getAttribute("dd-alignment");

        this.style.top = "";
        this.style.left = "";
        this.style.transform = "";

        if (upwards) {
            this.style.top = `${anchorRect.top}px`;
            this.style.transform = "translateY(-100%)";
        } else {
            this.style.top = `${anchorRect.top + anchorRect.height}px`;
        }

        if (alignment == "right") {
            this.style.left = `${anchorRect.left + anchorRect.width}px`;
            this.style.transform = "translateX(-100%)";
        } else {
            this.style.left = `${anchorRect.left}px`;
        }

        if (this.hasAttribute("dd-width")) {
            if (this.getAttribute("dd-width") == "anchor") {
                this.style.width = `${anchorRect.width}px`;
            }
        }
    }
    
    show(event, focus) {
        if (typeof focus == "undefined") focus = true;

        if (!this.classList.contains("dd-showing")) {
            this.classList.add("dd-showing");
            let menuElem = this;

            requestAnimationFrame(function() {
                window.addEventListener("click", menuElem.windowClickHandler);
                window.addEventListener("contextmenu", menuElem.windowClickHandler);
                window.addEventListener("keydown", menuElem.windowKeyDownHandler);
                menuElem.updatePositionAndDimensions();
            });
            
            if (focus) this.focus();
        }
        
        if (event && (this.hasAttribute("dd-context-menu") || focus == false)) {
            event.preventDefault();
        }
    }
    
    hide() {
        this.classList.remove("dd-showing");
        window.removeEventListener("click", this.windowClickHandler);
        window.removeEventListener("contextmenu", this.windowClickHandler);
        window.removeEventListener("keydown", this.windowKeyDownHandler);
        if (this.anchor) {
            this.anchor.removeEventListener("keydown", this.anchorKeyDownHandler);
        }
    }
    
    static genAnchorMouseEnterHandler(menuElem) {
        return function(event) {
            menuElem.updatePositionAndDimensions();
            menuElem.classList.add("dd-showing");
        }
    }
    
    static genAnchorMouseLeaveHandler(menuElem) {
        return function(event) {
            menuElem.hide();
        }
    }

    static genAnchorClickHandler(menuElem) {
        if (menuElem.anchorType != "INPUT") {
            return function(event) {
                if (this.hasAttribute("disabled")) return;
                if (!menuElem.isShowing()) {
                    menuElem.show();
                } else {
                    menuElem.hide();
                }
            }
        } else {
            return function(event) {
                if (this.hasAttribute("disabled")) return;
                menuElem.anchor.input.focus();
                if (!menuElem.isShowing()) {
                    menuElem.show(event, false);
                }
            }
        }
    }
    
    static genAnchorKeyDownHandler(menuElem) {
        return function(event) {
            if (event.target == this && DD_Components.isKeyTab(event.keyCode) && !event.shiftKey) {
                if (menuElem.isShowing()) {
                    menuElem.focus();
                    event.preventDefault();
                }
            }
        }
    }

    static genAnchorFocusHandler(menuElem) {
        return function(event) {
            if (this.hasAttribute("disabled")) return;
            if (!menuElem.isShowing()) {
                menuElem.show(event, false);
                if (menuElem.anchorType == "INPUT") {
                    menuElem.anchor.saveOptionsOrder();
                    menuElem.anchor.updateOptionsOrder();
                }
            }
        }
    }

    static genAnchorBlurHandler(menuElem) {
        return function(event) {
            if (this.hasAttribute("disabled")) return;
            if (menuElem.isShowing()) {
                menuElem.hide();
                if (menuElem.anchorType == "INPUT") {
                    menuElem.anchor.restoreOptionsOrder();
                }
            }
        }
    }
    
    static genWindowClickHandler(menuElem) {
        return function(event) {
            if (menuElem.anchorType != "INPUT") {
                if (menuElem != event.target && !menuElem.contains(event.target)) {
                    menuElem.hide();
                }
            } else {
                if (!DD_Components.isFocused(menuElem.anchor.input)) {
                    menuElem.hide();
                }
                if (menuElem == event.target || menuElem.contains(event.target) || menuElem.anchor.inputContainer == event.target || menuElem.anchor.inputContainer.contains(event.target)) {
                    menuElem.anchor.input.focus();
                }
            }
        }
    }
    
    static genWindowKeyDownHandler(menuElem) {
        return function(event) {
            if (DD_Components.isKeyEsc(event.keyCode)) {
                menuElem.hide();
                menuElem.anchor.focus();
            }
        }
    }
}

DD_Menu.all = [];

/* SELECT */
/* SELECT */
/* SELECT */

class DD_Select extends DD_Menu {
    connectedCallback() {
        this.initializeMenu();
        
        if (!this.hasAttribute("dd-anchor")) {
            this.anchor = this.originalPreviousSibling;
        }

        if (this.anchor.hasAttribute("dd-no-arrow")) {
            this.anchor.classList.add("dd-hide-arrow");
        } else {
            this.anchor.classList.add("dd-show-arrow");
            if (this.anchorType == "BUTTON") {
                this.anchor.style.setProperty("--button-after-content", "'\u02c5'");
                this.anchor.style.setProperty("--button-after-font-size", "1.2em");
                this.anchor.style.setProperty("--button-after-transform", "scale(1.8, 1)");
                this.anchor.style.setProperty("--button-after-padding", "0 .3em");
                this.anchor.style.setProperty("--button-justify-content", "space-between");
            } else if (this.anchorType == "INPUT") {
            }
        }
        
        if (!this.hasAttribute("dd-selected")) {
            if (this.hasAttribute("dd-default")) {
                this.setAttribute("dd-selected", this.getAttribute("dd-default"));
            } else {
                this.setAttribute("dd-selected", "");
            }
        }
        
        if (this.anchor) {
            if (this.anchorType == "BUTTON") {
                this.anchor.getSelected = () => this.getSelected();
                Object.defineProperty(this.anchor, "value", {get () {return this.getSelected()}, configurable: true});
                this.anchor.setValue = (value, options) => this.setValue(value, options);
                Object.defineProperty(this.anchor, "value", {set (v) {this.setValue(v)}, configurable: true});
            } else if (this.anchorType == "INPUT") {
                this.anchor.bindSelectMenu(this);
                this.anchor.saveOptionsOrder();
            }
        }
        
        this.updateText();
    }
    
    setValue(value, options) {
        if (options == undefined) options = {};
        if (!("silent" in options)) options.silent = false;

        if (typeof value != "string") {
            value = value.join(";");
        }
        
        let oldValue = this.getAttribute("dd-selected");
        this.setAttribute("dd-selected", value);
        this.updateText();

        if (this.anchorType == "INPUT") {
            this.anchor.updateSelectionView();
            this.anchor.updateOptionsOrder();
        }

        if (!options.silent && this.hasAttribute("dd-onchange") && oldValue != value) {
            let func = this.getAttribute("dd-onchange");
            if (this.hasAttribute("dd-multiple")) {
                if (oldValue.length == 0) {
                    oldValue = [];
                } else {
                    oldValue = oldValue.split(";");
                }

                let newValue = value.split(";");

                if (value.length == 0) {
                    newValue = [];
                }

                DD_Components.executeFunctionByName(func, window, oldValue, newValue, this);
            } else {
                DD_Components.executeFunctionByName(func, window, oldValue, value, this);
            }
        }
    }

    selectOption(optionValue) {
        if (this.hasAttribute("dd-multiple")) {
            this.setValue(this.getSelected().concat([optionValue]));
        } else {
            this.setValue(optionValue);
        }
    }

    deselectOption(optionValue) {
        if (this.hasAttribute("dd-multiple")) {
            let selected = this.getSelected();
            DD_Components.removeItemFromArrayIfCondition(selected, (entry) => entry == optionValue);
            this.setValue(selected);
        } else {
            this.setValue("");
        }
    }
    
    getSelected() {
        if (this.hasAttribute("dd-multiple")) {
            if (this.getAttribute("dd-selected")) {
                return this.getAttribute("dd-selected").split(";");
            } else {
                return [];
            }
        } else {
            return this.getAttribute("dd-selected");
        }
    }

    isSelected(optionValue) {
        const selected = this.getSelected();
        return selected.includes(optionValue);
    }
    
    updateText() {
        let selected = this.getAttribute("dd-selected");
        let text = this.getOptionText(selected);

        if (text == null) {
            if (this.hasAttribute("dd-placeholder")) {
                text = this.getAttribute("dd-placeholder");
                this.anchor.classList.add("dd-show-placeholder");
            } else {
                text = "";
                this.anchor.classList.remove("dd-show-placeholder");
            }
        } else {
            this.anchor.classList.remove("dd-show-placeholder");
        }
        
        if (this.anchorType != "INPUT") {
            this.anchor.innerHTML = text;
        } else if (this.hasAttribute("dd-multiple")) {
            if (!selected) {
                this.anchor.setAttribute("placeholder", text);
            } else {
                this.anchor.setAttribute("placeholder", "");
            }
        } else {
            this.anchor.setAttribute("placeholder", text);
        }
    }
    
    getOption(value) {
        let options = this.querySelectorAll("dd-option");
        for (let opt of options) {
            if (opt.getAttribute("value") == value) {
                return opt;
            }
        }
        return null;
    }
    
    getOptionText(value) {
        let option = this.getOption(value);
        if (option) {
            return option.getAttribute("dd-text");
        } else {
            return null;
        }
    }
}

class DD_FileUploader extends DD_Button {
    constructor() {
        super();
        this.input = document.createElement("input");
        this.input.setAttribute("type", "file");
        this.input.setAttribute("multiple", "");
        this.input.style.display = "none";
        this.continueSpinner = false;
        this.defaultInnerHTML = "";
        this.files = [];

        let thisElem = this;

        this.input.addEventListener('change', function() {
            thisElem.processFiles(this.files);
        });

        this.addEventListener("click", function(event) {
            if (!this.hasAttribute("disabled")) {
                this.input.click();
            }
        });

        ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
            thisElem.addEventListener(eventName, e => e.preventDefault());
            thisElem.addEventListener(eventName, e => e.stopPropagation());
        });

        // Visual feedback
        this.addEventListener('dragover', () => this.classList.add('dragover'));
        this.addEventListener('dragleave', () => this.classList.remove('dragover'));
        this.addEventListener('drop', () => this.classList.remove('dragover'));

        // Handle drop
        this.addEventListener('drop', async (event) => {
            const files = event.dataTransfer.files;
            this.processFiles(files);
        });
    }

    connectedCallback() {
        if (this.hasAttribute("dd-reconnecting")) {
            this.removeAttribute("dd-reconnecting");
            return;
        }

        this.initializeButton();
        if (this.hasAttribute("dd-accept")) this.input.setAttribute("accept", this.getAttribute("dd-accept"));
        this.appendChild(this.input);
    }

    isFileSupported(file) {
        if (this.hasAttribute("dd-accept")) {
            const accept = this.getAttribute("dd-accept");
            const acceptEntries = accept.split(",");
            for (const entry of acceptEntries) {
                if (file.type == entry.trim() || file.name.endsWith(entry.trim())) {
                    return true;
                }
            }
            return false;
        } else {
            return true;
        }
    }

    async readFilesAsArrayBuffers(fileList) {
        const thisElem = this;
        const promises = Array.from(fileList).map(file => {
            return new Promise((resolve, reject) => {
                file.supported = thisElem.isFileSupported(file);
                if (file.supported) {
                    const reader = new FileReader();
                    reader.onload = () => {
                        file.arrayBuffer = reader.result;
                        resolve(file);
                    }
                    reader.onerror = reject;
                    reader.readAsArrayBuffer(file);
                } else {
                    resolve(file);
                }
            });
        });

        return Promise.all(promises);
    }

    startSpinner() {
        if (!this.defaultInnerHTML) {
            this.defaultInnerHTML = this.innerHTML;
        }
        this.innerHTML = `<span class="dd-spinner"></span>`;
        this.setAttribute("disabled", "");
    }

    stopSpinner() {
        this.innerHTML = this.defaultInnerHTML;
        this.removeAttribute("disabled");
    }

    callChangeEventHandler(oldValue, newValue) {
        if (this.hasAttribute("dd-onchange")) {
            DD_Components.executeFunctionByName(this.getAttribute("dd-onchange"), window, this, oldValue, newValue);
            if (!this.continueSpinner) {
                this.stopSpinner();
            }
            this.continueSpinner = false;
        }
    }

    async processFiles(files) {
        this.startSpinner();
        const oldValue = this.files;
        const newValue = await this.readFilesAsArrayBuffers(files);
        this.files = newValue;
        this.callChangeEventHandler(oldValue, newValue);
    }

    get value() {
        return this.files;
    }

    set value(newValue) {
        const oldValue = this.files;
        this.files = newValue;
        this.callChangeEventHandler(oldValue, newValue);
    }
}

class DD_Option extends DD_Button {
    constructor() {
        super();

        this.addEventListener("mousedown", function(event) {
            // NOTE: Prevent blur on anchor if it is dd-input
            let menu = this.getSelectMenu();
            if (menu.anchorType == "INPUT") {
                event.preventDefault();
            }
        });
        
        this.addEventListener("click", function(event) {
            let value = this.getAttribute("value");
            if (value) {
                let menu = this.getSelectMenu();
                menu.selectOption(value);
                if (menu.anchorType != "INPUT") {
                    menu.anchor.focus();
                } else {
                    menu.anchor.input.focus();
                }
                menu.hide();
            }
        });
    }
    
    connectedCallback() {
        this.initializeButton();
        
        if (!this.hasAttribute("dd-text")) {
            if (this.hasAttribute("value")) {
                this.setAttribute("dd-text", this.getAttribute("value"));
            } else {
                this.setAttribute("dd-text", "");
            }
        }
            
        this.innerHTML = this.getAttribute("dd-text");
        
        let selectMenu = this.getSelectMenu();
        
        if (!this.hasAttribute("value")) {
            this.setAttribute("value", "");
        }
        
        if (selectMenu) {
            if (this.hasAttribute("value")) {
                if (selectMenu.hasAttribute("dd-selected")) {
                    let value = this.getAttribute("value");
                    let selected = selectMenu.getAttribute("dd-selected");
                    
                    if (selected == "" || selected == undefined) {
                        let defaultValue = selectMenu.getAttribute("dd-default");
                        
                        if (defaultValue == value) {
                            selectMenu.setValue(defaultValue);
                        }
                    } else if (selected == value) {
                        selectMenu.updateText();
                    }
                }
            }
        }
    }
    
    get value() {
        return this.getAttribute("value");
    }
    
    set value(v) {
        this.setAttribute("value", v);
    }
    
    getSelectMenu() {
        let el = this.parentElement;
        while (el) {
            if (el.tagName == "DD-SELECT") {
                return el;
            }
            el = el.parentElement;
        }
        return null;
    }
}

/* DATA */
/* DATA */
/* DATA */

class DD_Data extends HTMLElement {
    constructor() {
        super();
    }
 
    connectedCallback() {
        if (!this.hasAttribute("dd-data")) this.setAttribute("dd-data", "");
        
        this.updateDataPath();
        this.update();
        
        DD_Data.all.push(this);
        this.addToUpdateQueue();
        DD_Data.scheduleUpdateIfNeeded();
    }
    
    disconnectedCallback() {
        DD_Components.removeItemFromArrayIfCondition(DD_Data.all, (entry) => entry == this);
    }
 
    updateDataPath() {
        let dataPath = this.getAttribute("dd-data");
        let relativeDataPath = this.getAttribute("dd-relative-path");
        
        if (dataPath.startsWith("[*]")) {
            relativeDataPath = dataPath.substring(3);
            this.setAttribute("dd-relative-path", relativeDataPath);
        } else if (dataPath.startsWith(".") || dataPath.startsWith("[")) {
            this.setAttribute("dd-root", "");
           
            let el = this;
            while (el && !el.hasAttribute("dd-data-root")) {
                el = el.parentElement;
            }
           
            if (el) {
                let dataRoot = el.getAttribute("dd-data-root");
                this.setAttribute("dd-root", dataRoot);
                this.setAttribute("dd-data", dataRoot + dataPath);
            }
        }
    }
 
    update() {
        let pathContext = this.getAssociatedData();
        let displayedData = undefined;
        
        if (pathContext) {
            displayedData = DD_Components.getDataByPath(this.getAttribute("dd-relative-path"), pathContext);
        } else {
            displayedData = DD_Components.getDataByPath(this.getAttribute("dd-data"), window);
        }
        
        if (displayedData == undefined) {
            if (this.hasAttribute("dd-placeholder")) {
                displayedData = this.getAttribute("dd-placeholder");
            } else {
                displayedData = "";
            }
        }
        
        this.innerHTML = displayedData;
    }
    
    addToUpdateQueue() {
        DD_Data.updateQueue.push(this);
        this.needsUpdate = true;
    }
    
    getAssociatedData() {
        if (this.getAttribute("dd-data").startsWith("[*]")) {
            let el = this;
            
            while (el && !("associatedData" in el)) {
                el = el.parentElement;
            }
            
            if (el) return el.associatedData;
        } else {
            return undefined;
        }
    }
    
    static updateQueuedElements() {
        for (let dataElem of DD_Data.updateQueue) {
            dataElem.update();
            dataElem.needsUpdate = false;
        }
        
        DD_Data.updateQueue = [];
        DD_Data.updateScheduled = false;
    }
    
    static update(pathBegining) {
        if (pathBegining == undefined) pathBegining = "";
        
        for (let dataElem of DD_Data.all) {
            if (dataElem.getAttribute("dd-data").startsWith(pathBegining) && !dataElem.needsUpdate) {
                dataElem.addToUpdateQueue();
            }
        }
        
        if (DD_Data.updateQueue.length) {
            DD_Data.scheduleUpdateIfNeeded();
        }
    }
    
    static scheduleUpdateIfNeeded() {
        if (!DD_Data.updateScheduled) {
            requestAnimationFrame(DD_Data.updateQueuedElements);
            DD_Data.updateScheduled = true;
        }
    }
}

DD_Data.all = [];
DD_Data.updateQueue = [];
DD_Data.updateScheduled = false;
    
/* ARRAY */
/* ARRAY */
/* ARRAY */

class DD_Array extends HTMLElement {
    connectedCallback() {
        this.sorters = [];
        this.generatedItems = [];
        DD_Array.all.push(this);
        
        this.updateDefaultSorter();
    }
    
    disconnectedCallback() {
        this.generatedItems = [];
        DD_Components.removeItemFromArrayIfCondition(DD_Array.all, (entry) => entry == this);
    }
    
    updateDefaultSorter() {
        let sorter = DD_Components.getSorter(this);
        if (sorter) {
            this.addSorter(sorter);
        }
    }
    
    addSorter(options) {
        if (!("sorter" in options)) return;
        
        this.sorters.push(options);
    }
    
    generateItems() {
        if (this.hasAttribute("dd-array")) {
            let arrayPath = this.getAttribute("dd-array");
            let obj = DD_Components.getDataByPath(arrayPath, window);
            if (obj) {
                let frag = document.createDocumentFragment();
                
                if (Array.isArray(obj)) {
                    for (let i = 0; i < obj.length; ++i) {
                        if (!this.applyFilter(obj[i])) continue;
                        
                        let html = "";
                        let templateHTML = this.querySelector("template").innerHTML;
                        
                        let idx = 0;
                        let placeholderStart = templateHTML.indexOf("[*]");
                        while (placeholderStart >= 0) {
                            let resolveAndReplace = false;
                            let path = "[*]";
                            
                            if (templateHTML[placeholderStart-1] == '$') {
                                resolveAndReplace = true;
                                path = "";
                                html += templateHTML.substring(idx, placeholderStart-1);
                            } else {
                                html += templateHTML.substring(idx, placeholderStart);
                            }
                            
                            idx = placeholderStart + 3;
                            
                            while (idx < templateHTML.length && DD_Components.isValidDataPathCharacter(templateHTML[idx])) {
                                path += templateHTML[idx];
                                ++idx;
                            }
                            
                            if (resolveAndReplace) {
                                let d = DD_Components.getDataByPath(path, obj[i]);
                                html += d;
                            } else {
                                html += `<dd-data dd-data="${path}"></dd-data>`;
                            }
                            
                            placeholderStart = templateHTML.indexOf("[*]", idx);
                        }
                        
                        html += templateHTML.substring(idx);
                        
                        let tpl = document.createElement("template");
                        tpl.innerHTML = html.trim();
                        for (let node of tpl.content.childNodes) {
                            node.associatedData = obj[i];
                        }
                        frag.append(...tpl.content.childNodes);
                    }
                }
                
                DD_Components.concatenateArrays(this.generatedItems, frag.childNodes);
                this.parentElement.append(...frag.childNodes);
            } else {
                console.error(`DD_Array Error: ${arrayPath} is not an object of 'window'`);
            }
        }
    }
    
    applyFilter(item) {
        if (this.hasAttribute("dd-filter")) {
            let result = DD_Components.executeFunctionByName(this.getAttribute("dd-filter"), window, item);
            return result.returned;
        } else {
            return true;
        }
    }
    
    isEmpty() {
        return this.generatedItems.length == 0;
    }
    
    getDataType(dataPath) {
        if (!this.isEmpty()) {
            let item = this.generatedItems[0];
            let obj = DD_Components.getAssociatedData(item);
            let something = DD_Components.getSomethingByPath(dataPath, obj);
            if (something) {
                return typeof something.something;
            } else {
                return undefined;
            }
        } else {
            return undefined;
        }
    }
    
    removeGeneratedItems() {
        let fragment = document.createDocumentFragment(); 
        fragment.textContent = ' ';
        
        if (this.generatedItems.length) {
            fragment.firstChild.replaceWith(...this.generatedItems);
        }
    
        this.generatedItems = [];
    }
    
    update() {
        this.removeGeneratedItems();
        this.generateItems();
        this.updateDefaultSorter();
        this.applySorters();
    }
    
    updateItem(matchFunction) {
        for (let item of this.generatedItems) {
            if (matchFunction(item.associatedData)) {
                let dataElements = item.querySelectorAll("dd-data");
                
                for (let el of dataElements) el.update();
                if (item.tagName == "DD-DATA") item.update();
            }
        }
    }
    
    applySorters() {
        let items = this.generatedItems;
        
        if (items.length && this.sorters.length) {
            let referenceNodeForReinsertion = items[items.length-1].nextSibling;
            let parentElem = items[items.length-1].parentElement;
            
            for (let sorter of this.sorters) {
                items.sort((item1, item2) => sorter.sorter(item1.associatedData, item2.associatedData));
            }
            
            let fragment = document.createDocumentFragment(); 
            fragment.textContent = ' ';
            fragment.firstChild.replaceWith(...items);
            
            parentElem.insertBefore(fragment, referenceNodeForReinsertion);
        }
    }
    
    removeItem(matchFunction) {
        for (let i = 0; i < this.generatedItems.length; /* */) {
            let item = this.generatedItems[i];
            if (matchFunction(item.associatedData)) {
                item.remove();
                DD_Components.removeIndexFromArray(this.generatedItems, i);
            } else {
                ++i;
            }
        }
    }
    
    static getArraysByPath(path) {
        let result = [];
        for (let arr of DD_Array.all) {
            if (arr.getAttribute("dd-array") == path) {
                result.push(arr);
            }
        }
        return result;
    }
    
    static update(path) {
        let arrays = DD_Array.getArraysByPath(path);
        for (let arr of arrays) {
            arr.update();
        }
    }
    
    static updateItem(path, matchFunction) {
        let arrays = DD_Array.getArraysByPath(path);
        for (let arr of arrays) {
            arr.updateItem(matchFunction);
        }
    }
}

DD_Array.all = [];

/* SLIDER */
/* SLIDER */
/* SLIDER */

class DD_Slider extends HTMLElement {
    constructor() {
        super();

        this.leftBar = document.createElement("div");
        this.leftBar.classList.add("dd-left-bar");

        this.rightBar = document.createElement("div");
        this.rightBar.classList.add("dd-right-bar");

        this.sliding = false;

        const thisElem = this;

        function onMouseDown(event) {
            if ("touches" in event) {
                event.clientX = event.touches[0].clientX;
                event.clientY = event.touches[0].clientY;
            }

            const rect = this.getBoundingClientRect();
            const x = event.clientX - rect.x;
            this.setValue(x / rect.width, true);
            this.sliding = true;
        }

        function onMouseMove(event) {
            if ("touches" in event) {
                event.clientX = event.touches[0].clientX;
                event.clientY = event.touches[0].clientY;
            }

            if (thisElem.sliding) {
                const rect = thisElem.getBoundingClientRect();
                if (event.clientX < rect.x) {
                    thisElem.setValue(0.0, true);
                } else if (event.clientX > rect.x + rect.width) {
                    thisElem.setValue(1.0, true);
                } else {
                    const x = event.clientX - rect.x;
                    thisElem.setValue(x / rect.width, true);
                }
            }
        }

        function onMouseUp(event) {
            thisElem.sliding = false;
        }

        this.addEventListener("mousedown", onMouseDown);
        this.addEventListener("touchstart", onMouseDown);

        window.addEventListener("mousemove", onMouseMove);
        window.addEventListener("touchmove", onMouseMove);

        window.addEventListener("mouseup", onMouseUp);
        window.addEventListener("touchend", onMouseUp);
    }

    connectedCallback() {
        this.appendChild(this.leftBar);
        this.appendChild(this.rightBar);
        this.setValue(0.0, false);
    }

    updateVisual() {
        this.leftBar.style.width = `${this._value*100}%`;
    }

    setValue(newValue, notify) {
        const oldValue = this._value;
        this._value = newValue;
        this.updateVisual();
        if (notify && this.hasAttribute("dd-onchange") && oldValue != newValue) {
            DD_Components.executeFunctionByName(this.getAttribute("dd-onchange"), window, this, oldValue, newValue);
        }
    }

    get value() {
        return this._value;
    }

    set value(v) {
        this.setValue(v, false);
    }
}

/* HR */
/* HR */
/* HR */

class DD_HorizontalRule extends HTMLElement {}

/* SPINNER */
/* SPINNER */
/* SPINNER */

class DD_Spinner extends HTMLElement {
    connectedCallback() {
        this.classList.add("dd-spinner");
    }
}

/* LANGUAGE */
/* LANGUAGE */
/* LANGUAGE */

class DD_L extends HTMLElement {
    connectedCallback() {
        let elem = this;
        requestAnimationFrame(() => {
            elem.update();
        });

        DD_L.all.push(this);
    }

    disconnectedCallback() {
        DD_Components.removeItemFromArrayIfCondition(DD_L.all, (entry) => entry == this);
    }

    update() {
        let textId = this.getAttribute("dd-text-id");
        if (!textId) {
            textId = this.innerHTML;
            this.setAttribute("dd-text-id", textId);
        }

        this.innerHTML = this.getText();
    }

    static get(textId) {
        if (textId in DD_L.texts) {
            if (DD_L.lang in DD_L.texts[textId]) {
                return DD_L.texts[textId][DD_L.lang];
            } else {
                console.warn(`DD_L: could not find text '${textId}' in '${DD_L.lang}'`);
                return textId;
            }
        } else {
            console.error(`DD_L: could not find text '${textId}' in '${DD_L.lang}'`);
            return textId;
        }
    }

    getText() {
        let textId = this.getAttribute("dd-text-id");
        return DD_L.get(textId);
    }

    static getSupportedLanguages() {
        let result = [];
        let keys = Object.keys(DD_L.texts);
        for (let key of keys) {
            let entry = DD_L.texts[key];
            let entryKeys = Object.keys(entry);

            for (let lang of entryKeys) {
                if (!result.includes(lang)) {
                    result.push(lang);
                }
            }
        }
        return result;
    }

    static setLang(lang) {
        if (DD_L.lang == lang) return;
        DD_L.lang = lang;

        for (let el of DD_L.all) {
            el.update();
        }

        let elWithPlaceholders = document.querySelectorAll("[dd-translate-placeholder]");
        for (let el of elWithPlaceholders) {
            if (!el.hasAttribute("dd-placeholder-text-id")) {
                el.setAttribute("dd-placeholder-text-id", el.getAttribute("placeholder"));
            }

            let textId = el.getAttribute("dd-placeholder-text-id");
            el.setAttribute("placeholder", DD_L.get(textId));
        }

        setTimeout(() => {
            let bodyClasses = Array.from(document.body.classList);
            for (let cl of bodyClasses) {
                if (cl.startsWith("lang_")) document.body.classList.remove(cl);
            }

            let langs = DD_L.getSupportedLanguages();

            if (langs.includes(lang)) {
                document.body.classList.add(`lang_${DD_L.lang}`);
            } else if (langs.length) {
                DD_L.setLang(langs[0]);
            }
        });
    }
}

DD_L.all = [];
DD_L.texts = {};

window.addEventListener("DOMContentLoaded", function() {
    if (!DD_L.lang) {
        DD_L.setLang(DD_Components.getPreferredLang());
    }
});

// Init
(function() {
    let style = document.createElement("style");

style.textContent = `
    /* BORDER BOX */
    /* BORDER BOX */
    /* BORDER BOX */
    
    .dd-button, .dd-button::before, .dd-button::after,
    dd-button, dd-button::before, dd-button::after,
    dd-tag, dd-tag::before, dd-tag::after,
    dd-label, dd-label::before, dd-label::after,
    dd-header, dd-header::before, dd-header::after,
    dd-checkbox, dd-checkbox::before, dd-checkbox::after,
    dd-toggle, dd-toggle .toggle-track, dd-toggle::before,
    dd-radio, dd-radio::before, dd-radio::after,
    dd-tooltip, dd-tooltip::before, dd-tooltip::after,
    dd-option, dd-option::before, dd-option::after,
    dd-select, dd-select::before, dd-select::after,
    dd-menu, dd-menu::before, dd-menu::after,
    dd-input, dd-input::before, dd-input::after,
    dd-table, dd-table::before, dd-table::after,
    dd-data, dd-data::before, dd-data::after,
    dd-tab, dd-tab::before, dd-tab::after,
    dd-area, dd-area::before, dd-area::after
        { box-sizing: border-box }
    
    /* USING MOUSE */
    /* USING MOUSE */
    /* USING MOUSE */
    
    body:not(.dd-using-mouse) *:focus {
        box-shadow: 0px 0px 0px 4px var(--dd-focus-outline-color, blue);
        transition: box-shadow 200ms ease;
    }
    
    /* BUTTON AND SIMILAR */
    /* BUTTON AND SIMILAR */
    /* BUTTON AND SIMILAR */
    
    dd-button, .dd-button, dd-tag, dd-label, dd-header, dd-checkbox, dd-toggle, dd-radio, dd-option, dd-tab, dd-file-uploader, a.dd-button:visited
    {
        --border-color: var(--button-border-color, transparent);
        --border-style: var(--button-border-style, solid);
        --border-width: var(--button-border-width, 0);
        
        --border-top: var(--button-border-top, var(--border-width) var(--border-color));
        --border-right: var(--button-border-right, var(--border-width) var(--border-color));
        --border-bottom: var(--button-border-bottom, var(--border-width) var(--border-color));
        --border-left: var(--button-border-left, var(--border-width) var(--border-color));
        
        --button-margin: var(--button-margin-top, 0) var(--button-margin-right, 0) var(--button-margin-bottom, 0) var(--button-margin-left, 0);
        
        display: var(--button-display, flex);
        position: var(--button-position, initial);
        gap: var(--button-gap, 0.3em);
        align-items: center;
        justify-content: var(--button-justify-content, center);
        
        background-color: var(--button-bg, transparent);
        background-clip: padding-box;
        color: var(--button-color, inherit);
        padding: var(--button-padding, 0);
        margin: var(--button-margin, 0);
        overflow: var(--button-overflow);
        
        border-top: var(--border-style) var(--border-top);
        border-right: var(--border-style) var(--border-right);
        border-bottom: var(--border-style) var(--border-bottom);
        border-left: var(--border-style) var(--border-left);
        
        border-radius: var(--button-border-radius, 0);
        font-size: var(--button-font-size, inherit);
        font-family: var(--button-font-family, inherit);
        line-height: var(--button-line-height, 1);
        font-weight: var(--button-font-weight, inherit);
        cursor: var(--button-cursor, pointer);
        text-transform: var(--button-text-transform, none);
        width: var(--button-width, initial);
        height: var(--button-height, initial);
        flex-shrink: var(--button-flex-shrink, initial);
        white-space: var(--button-white-space, normal);
        
        opacity: var(--button-opacity, 1);
        
        transition: var(--button-transition, none);

        /* Disable text selection */
        user-select: none;
        -webkit-user-select: none; /* Safari */
        -moz-user-select: none;    /* Firefox */
        -ms-user-select: none;     /* IE10+/Edge */
    }
    
    dd-button::after, .dd-button::after {
        content: var(--button-after-content, initial);
        font-size: var(--button-after-font-size, inherit);
        font-weight: var(--button-after-font-weight, inherit);
        transform: var(--button-after-transform, none);
        padding: var(--button-after-padding, 0);
    }
    
    dd-button::before, .dd-button::before, dd-option::before {
        content: var(--button-before-content, initial);
        font-family: var(--button-before-font-family, inherit);
        font-size: var(--button-before-font-size, inherit);
        font-weight: var(--button-before-font-weight, inherit);
        transform: var(--button-before-transform, none);
        padding: var(--button-before-padding, 0);
    }
    
    dd-tag, dd-header { cursor: var(--button-cursor, default) }
    dd-header { cursor: auto }
    
    dd-button:hover, .dd-button:hover, dd-tag:hover, dd-label:hover, dd-header:hover, dd-checkbox:hover, dd-radio:hover, dd-option:hover, dd-tab:hover, a.dd-button:hover
    {
        color: var(--button-hover-color, var(--button-color, inherit));
        background-color: var(--button-hover-bg, var(--button-bg, transparent));
        border-color: var(--button-hover-border-color, var(--button-border-color, var(--border-color)));
        opacity: var(--button-hover-opacity, 1);
    }

    dd-button[dd-pressed], .dd-button[dd-pressed] {
        color: var(--button-pressed-color, var(--button-color, inherit));
        background-color: var(--button-pressed-bg, var(--button-bg, inherit));
        border-color: var(--button-pressed-border-color, var(--button-border-color, transparent));
        font-weight: var(--button-pressed-font-weight, var(--button-font-weight, inherit));
    }

    dd-button.dd-active, .dd-button.dd-active, dd-tab.dd-active, a.dd-button.dd-active:hover {
        color: var(--button-active-color, var(--button-color, inherit));
        background-color: var(--button-active-bg, var(--button-bg, inherit));
        border-color: var(--button-active-border-color, var(--button-border-color, transparent));
        font-weight: var(--button-active-font-weight, var(--button-font-weight, inherit));
        opacity: var(--button-active-opacity, var(--button-opacity, inherit));
        border-left: var(--button-active-border-left, var(--border-style) var(--button-border-left, var(--button-border-width) var(--button-active-border-color, var(--button-border-color))));
        border-bottom: var(--button-active-border-bottom, var(--border-style) var(--button-border-bottom, var(--button-border-width) var(--button-active-border-color, var(--button-border-color))));
    }
    
    dd-button .fa, .dd-button .fa, dd-option .fa, dd-header .fa, dd-tab .fa {
        font-size: 1.2em;
    }

    dd-option.dd-hide {
        display: none;
    }

    /* TOGGLE */
    /* TOGGLE */
    /* TOGGLE */

    dd-toggle .toggle-track {
        display: var(--toggle-display, inline-block);
        width: var(--toggle-size, 35px);
        height: calc(var(--toggle-size, 35px) * 0.54);
        border-radius: 100vw;
        border: 0px solid var(--toggle-bg, #ffffff);
        background: var(--toggle-thumb-bg, #bbb);
        padding: 2px 3px;
        transition: color 200ms ease-out, padding 200ms ease-out;
    }

    dd-toggle .toggle-track::before {
        content: '';
        display: block;
        height: 100%;
        aspect-ratio: 1 / 1;
        border-radius: 100%;
        background: var(--toggle-bg, #ffffff);
    }

    dd-toggle[checked] .toggle-track {
        background: var(--toggle-checked-bg, #4b92e7);
        padding-left: calc(var(--toggle-size, 35px) * 0.5 - 2px);
    }

    /* AREA */
    /* AREA */
    /* AREA */
    
    dd-area {
        display: block;
        transition-duration: var(--fade-speed, 300ms);
        transition-property: opacity;
        transition-timing-function: ease;
        opacity: 1;
    }
    
    dd-area.dd-hiding, dd-area.dd-unhiding {
        opacity: 0 !important;
    }
    
    dd-area.dd-hidden {
        display: none !important;
        opacity: 0;
    }
    
    /* CHECKBOX AND RADIO */
    /* CHECKBOX AND RADIO */
    /* CHECKBOX AND RADIO */
    
    dd-checkbox::before, dd-radio::before {
        content: '';
        display: var(--checkbox-display, inline-block);
        width: var(--checkbox-size, 13px);
        height: var(--checkbox-size, 13px);
        border-radius: 3px;
        border: 2px solid var(--checkbox-bg, #ffffff);
        background: var(--checkbox-bg, #ffffff);
        box-shadow: 0 0 1px 1px #cccccc;
    }

    dd-checkbox[checked]::before, dd-radio[checked]::before {
        background: var(--checkbox-checked-bg, #4b92e7);
    }

    dd-checkbox[checked], dd-radio[checked] {
        color: var(--checkbox-checked-color, var(--button-color, inherit));
        font-weight: var(--checkbox-checked-font-weight, var(--button-font-weight, inherit));
        border-color: var(--checkbox-checked-border-color, var(--border-color));
    }

    dd-checkbox[checked]:hover, dd-radio[checked]:hover {
        color: var(--button-hover-color, var(--checkbox-checked-color, inherit));
    }

    dd-radio::before {
        border-radius: 100%;
    }
    
    /* INPUT */
    /* INPUT */
    /* INPUT */

    dd-input .dd-input-container {
        display: flex;
        flex-wrap: wrap;
        flex-grow: 1;
    }

    dd-input .dd-clear-button {
        visibility: hidden;
        transform: scale(1.8);
        font-weight: bold;
        cursor: pointer;
    }

    dd-input .dd-clear-button.dd-show {
        visibility: visible;
    }

    dd-input input, .dd-input {
        --border-radius:
            var(--input-border-top-left-radius, 0)
            var(--input-border-top-right-radius, 0)
            var(--input-border-bottom-right-radius, 0)
            var(--input-border-bottom-left-radius, 0);
        
        --border-color: var(--input-border-color, transparent);
        --border-style: var(--input-border-style, solid);
        --border-width: var(--input-border-width, 0);
        
        --border-top: var(--input-border-top, var(--border-width) var(--border-color));
        --border-right: var(--input-border-right, var(--border-width) var(--border-color));
        --border-bottom: var(--input-border-bottom, var(--border-width) var(--border-color));
        --border-left: var(--input-border-left, var(--border-width) var(--border-color));
        
        flex-grow: 1;
        height: 100%;
        outline: var(--input-outline, none);
        
        border-top: var(--border-style) var(--border-top);
        border-right: var(--border-style) var(--border-right);
        border-bottom: var(--border-style) var(--border-bottom);
        border-left: var(--border-style) var(--border-left);
        
        border-radius: var(--input-border-radius, var(--border-radius, 0));

        background-color: var(--input-bg, #ffffff);
        padding: var(--input-padding, 0);
        font-size: var(--input-font-size, 1rem);
        line-height: var(--input-line-height, 1.1);
        font-family: inherit;
        filter: none;
    }
    
    dd-input, .dd-input {
        display: var(--input-display, flex);
        align-items: center;
        max-width: var(--input-max-width, none);
        min-width: var(--input-min-width, none);
        width: var(--input-width, 100%);
        background-color: var(--input-bg, #ffffff);
        cursor: text;
    }

    dd-input input, .dd-input {
        width: max(var(--input-width, 100%), 120px);
    }

    dd-input input:focus, .dd-input:focus {
        border-color: var(--input-focus-border-color, transparent);
        outline: var(--input-focus-outline, none);
    }
    
    dd-input input:autofill, .dd-input:autofill {
        background-color: var(--input-bg, #ffffff);
    }

    dd-input.dd-show-arrow::after, .dd-input.dd-show-arrow::after {
        content: var(--input-after-content, '\u02c5');
        font-size: var(--input-after-font-size, 1.2em);
        font-weight: var(--input-after-font-weight, inherit);
        transform: var(--input-after-transform, scale(1.8, 1));
        padding: var(--input-after-padding, 0 .3em);
        line-height: var(--input-after-line-height, 0);
        margin-left: var(--input-after-margin-left, 1em);
    }

    dd-input.dd-hide-arrow::after {
        content: initial;
    }

    dd-input dd-selected-option {
        --button-display: inline-block;
        --button-width: max-content;
    }

    /* TOOLTIP */
    /* TOOLTIP */
    /* TOOLTIP */
    
    dd-tooltip {
        background-color: var(--tooltip-bg, black);
        color: var(--tooltip-color, white);
        max-width: var(--tooltip-max-width, 150px);
        white-space: var(--tooltip-white-space, normal);
        font-family: var(--tooltip-font-family, sans-serif);
        font-size: var(--tooltip-font-size, .9rem);
        font-weight: var(--tooltip-font-weight, normal);
        padding: var(--tooltip-padding, .4em .2em);
        border-radius: var(--tooltip-border-radius, .2em);
        text-transform: var(--tooltip-text-transform, none);
        text-align: var(--tooltip-text-align, center);
        transition: opacity 150ms ease var(--tooltip-delay, 0ms);
        position: absolute;
        z-index: 9999999;
        opacity: 1;
    }
    
    dd-tooltip::before {
        content: var(--tooltip-before-content, none);
    }
    
    dd-tooltip:not(.dd-showing) {
        display: none !important;
    }
    
    dd-tooltip.dd-transparent {
        opacity: 0;
    }
    
    /* TABLE */
    /* TABLE */
    /* TABLE */
    
    dd-table { display: grid }
    dd-table[dd-cols="2"] { grid-template-columns: var(--c1, var(--c, auto)) var(--c2, var(--c, auto)) }
    dd-table[dd-cols="3"] { grid-template-columns: var(--c1, var(--c, auto)) var(--c2, var(--c, auto)) var(--c3, var(--c, auto)) }
    dd-table[dd-cols="4"] { grid-template-columns: var(--c1, var(--c, auto)) var(--c2, var(--c, auto)) var(--c3, var(--c, auto)) var(--c4, var(--c, auto)) }
    dd-table[dd-cols="5"] { grid-template-columns: var(--c1, var(--c, auto)) var(--c2, var(--c, auto)) var(--c3, var(--c, auto)) var(--c4, var(--c, auto)) var(--c5, var(--c, auto)) }
    
    dd-table[dd-cols="2"] .dd-filtered-placeholder, dd-table[dd-cols="2"] .dd-empty-placeholder { grid-column: span 2 }
    dd-table[dd-cols="3"] .dd-filtered-placeholder, dd-table[dd-cols="3"] .dd-empty-placeholder { grid-column: span 3 }
    dd-table[dd-cols="4"] .dd-filtered-placeholder, dd-table[dd-cols="4"] .dd-empty-placeholder { grid-column: span 4 }
    dd-table[dd-cols="5"] .dd-filtered-placeholder, dd-table[dd-cols="5"] .dd-empty-placeholder { grid-column: span 5 }
    
    dd-table .dd-filtered-placeholder.dd-hidden, dd-table .dd-empty-placeholder.dd-hidden { display: none !important; }

    dd-row, dd-table-header {
        display: contents;
        background-color: inherit;
        color: inherit;
        cursor: var(--row-cursor, auto);
    }

    dd-row:hover {
        background-color: var(--row-hover-bg, inherit);
    }
    
    dd-row.dd-filtered {
        display: none !important;
    }

    dd-table-body {
        display: contents;
        background-color: var(--body-bg, inherit);
        color: var(--body-color, inherit);
    }

    dd-cell {
        display: var(--cell-display, block);
        padding: var(--cell-padding, 0);
        background-color: var(--cell-bg, inherit);
        color: var(--cell-color, inherit);
        align-items: var(--cell-align-items, initial);
        justify-content: var(--cell-justify-content, initial);
        cursor: var(--cell-cursor, auto);
    }
    
    dd-table dd-row:not(:last-of-type) > dd-cell {
        border-bottom: var(--cell-border-bottom, none);
    }
    
    dd-cell:hover {
        background-color: var(--cell-hover-bg, var(--cell-bg, inherit));
    }
    
    dd-table-header > dd-cell {
        position: var(--header-position, static);
        top: var(--header-top, 0);
        padding: var(--header-padding, var(--cell-padding, 0));
        background-color: var(--header-bg, inherit);
        color: var(--header-color, inherit);
        font-weight: var(--header-font-weight, inherit);
        font-size: var(--header-font-size, inherit);
        text-transform: var(--header-text-transform, none);
        cursor: var(--header-cell-cursor, var(--cell-cursor, auto));
    }
    
    dd-table-header > dd-cell[dd-sorter] {
        cursor: var(--header-cell-cursor, var(--cell-cursor, pointer));
    }
    
    dd-table-header > dd-cell[dd-sorter] .dd-sort-icon {
        font-size: 1em;
        margin-left: .8em;
        line-height: inherit;
    }

    dd-table-header dd-cell:hover {
        background-color: var(--header-cell-hover-bg, var(--header-bg, inherit));
    }
    
    /* COLUMN STYLING */
    /* COLUMN STYLING */
    /* COLUMN STYLING */
    
    /* C1 */
    /* C1 */
    /* C1 */
    
    dd-table[dd-cols="2"] > dd-cell:nth-child(2n+1),
    dd-table[dd-cols="2"] > dd-row > dd-cell:nth-child(2n+1),
    dd-table[dd-cols="2"] > dd-table-body > dd-cell:nth-child(2n+1),
    dd-table[dd-cols="2"] > dd-table-body > dd-row > dd-cell:nth-child(2n+1),
    
    dd-table[dd-cols="3"] > dd-cell:nth-child(3n+1),
    dd-table[dd-cols="3"] > dd-row > dd-cell:nth-child(3n+1),
    dd-table[dd-cols="3"] > dd-table-body > dd-cell:nth-child(3n+1),
    dd-table[dd-cols="3"] > dd-table-body > dd-row > dd-cell:nth-child(3n+1),
    
    dd-table[dd-cols="4"] > dd-cell:nth-child(4n+1),
    dd-table[dd-cols="4"] > dd-row > dd-cell:nth-child(4n+1),
    dd-table[dd-cols="4"] > dd-table-body > dd-cell:nth-child(4n+1),
    dd-table[dd-cols="4"] > dd-table-body > dd-row > dd-cell:nth-child(4n+1),
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+1),
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+1),
    dd-table[dd-cols="5"] > dd-table-body > dd-cell:nth-child(5n+1)
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+1)
    
    {
        display: var(--c1-display, var(--cell-display, block));
        padding: var(--c1-padding, var(--cell-padding, block));
        color: var(--c1-color, var(--cell-color, inherit));
        cursor: var(--c1-cursor, var(--cell-cursor, auto));
        background-color: var(--c1-bg, var(--cell-bg, inherit));
        font-weight: var(--c1-font-weight, var(--cell-font-weight, inherit));
        font-size: var(--c1-font-size, inherit);
        text-transform: var(--c1-text-transform, inherit);
        text-align: var(--c1-text-align, inherit);
        line-height: var(--c1-line-height, inherit);
        justify-self: var(--c1-justify-self, var(--cell-justify-content, initial));
    }
    
    /* C1 HOVERED */
    /* C1 HOVERED */
    /* C1 HOVERED */
    
    dd-table[dd-cols="2"] > dd-cell:nth-child(2n+1):hover,
    dd-table[dd-cols="2"] > dd-row > dd-cell:nth-child(2n+1):hover,
    dd-table[dd-cols="2"] > dd-table-body > dd-cell:nth-child(2n+1):hover,
    dd-table[dd-cols="2"] > dd-table-body > dd-row > dd-cell:nth-child(2n+1):hover,
    
    dd-table[dd-cols="3"] > dd-cell:nth-child(3n+1):hover,
    dd-table[dd-cols="3"] > dd-row > dd-cell:nth-child(3n+1):hover,
    dd-table[dd-cols="3"] > dd-table-body > dd-cell:nth-child(3n+1):hover,
    dd-table[dd-cols="3"] > dd-table-body > dd-row > dd-cell:nth-child(3n+1):hover,
    
    dd-table[dd-cols="4"] > dd-cell:nth-child(4n+1):hover,
    dd-table[dd-cols="4"] > dd-row > dd-cell:nth-child(4n+1):hover,
    dd-table[dd-cols="4"] > dd-table-body > dd-cell:nth-child(4n+1):hover,
    dd-table[dd-cols="4"] > dd-table-body > dd-row > dd-cell:nth-child(4n+1):hover,
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+1):hover,
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+1):hover,
    dd-table[dd-cols="5"] > dd-table-body > dd-cell:nth-child(5n+1):hover
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+1):hover
    
    {
        display: var(--c1-hover-display, var(--cell-display, block));
        color: var(--c1-hover-color, var(--c1-color, var(--cell-color, inherit)));
        background-color: var(--c1-hover-bg, var(--c1-bg, var(--cell-bg, inherit)));
    }
    
    /* C2 */
    /* C2 */
    /* C2 */
    
    dd-table[dd-cols="2"] > dd-cell:nth-child(2n+2),
    dd-table[dd-cols="2"] > dd-row > dd-cell:nth-child(2n+2),
    dd-table[dd-cols="2"] > dd-table-body > dd-cell:nth-child(2n+2),
    dd-table[dd-cols="2"] > dd-table-body > dd-row > dd-cell:nth-child(2n+2),
    
    dd-table[dd-cols="3"] > dd-cell:nth-child(3n+2),
    dd-table[dd-cols="3"] > dd-row > dd-cell:nth-child(3n+2),
    dd-table[dd-cols="3"] > dd-table-body > dd-cell:nth-child(3n+2),
    dd-table[dd-cols="3"] > dd-table-body > dd-row > dd-cell:nth-child(3n+2),
    
    dd-table[dd-cols="4"] > dd-cell:nth-child(4n+2),
    dd-table[dd-cols="4"] > dd-row > dd-cell:nth-child(4n+2),
    dd-table[dd-cols="4"] > dd-table-body > dd-cell:nth-child(4n+2),
    dd-table[dd-cols="4"] > dd-table-body > dd-row > dd-cell:nth-child(4n+2),
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+2),
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+2),
    dd-table[dd-cols="5"] > dd-table-body > dd-cell:nth-child(5n+2)
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+2)
    
    {
        display: var(--c2-display, var(--cell-display, block));
        padding: var(--c2-padding, var(--cell-padding, block));
        color: var(--c2-color, var(--cell-color, inherit));
        cursor: var(--c2-cursor, var(--cell-cursor, auto));
        background-color: var(--c2-bg, var(--cell-bg, inherit));
        font-weight: var(--c2-font-weight, var(--cell-font-weight, inherit));
        font-size: var(--c2-font-size, inherit);
        text-transform: var(--c2-text-transform, inherit);
        text-align: var(--c2-text-align, inherit);
        line-height: var(--c2-line-height, inherit);
        justify-self: var(--c2-justify-self, var(--cell-justify-content, initial))
    }
    
    /* C2 HOVERED */
    /* C2 HOVERED */
    /* C2 HOVERED */
    
    dd-table[dd-cols="2"] > dd-cell:nth-child(2n+2):hover,
    dd-table[dd-cols="2"] > dd-row > dd-cell:nth-child(2n+2):hover,
    dd-table[dd-cols="2"] > dd-table-body > dd-row > dd-cell:nth-child(2n+2):hover,
    
    dd-table[dd-cols="3"] > dd-cell:nth-child(3n+2):hover,
    dd-table[dd-cols="3"] > dd-row > dd-cell:nth-child(3n+2):hover,
    dd-table[dd-cols="3"] > dd-table-body > dd-row > dd-cell:nth-child(3n+2):hover,
    
    dd-table[dd-cols="4"] > dd-cell:nth-child(4n+2):hover,
    dd-table[dd-cols="4"] > dd-row > dd-cell:nth-child(4n+2):hover,
    dd-table[dd-cols="4"] > dd-table-body > dd-row > dd-cell:nth-child(4n+2):hover,
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+2):hover,
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+2):hover,
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+2):hover
    
    {
        display: var(--c2-hover-display, var(--c2-display, var(--cell-display, block)));
        color: var(--c2-hover-color, var(--c2-color, var(--cell-color, inherit)));
        background-color: var(--c2-hover-bg, var(--cell-bg, inherit));
    }
    
    /* C3 */
    /* C3 */
    /* C3 */
    
    dd-table[dd-cols="3"] > dd-cell:nth-child(3n+3),
    dd-table[dd-cols="3"] > dd-row > dd-cell:nth-child(3n+3),
    dd-table[dd-cols="3"] > dd-table-body > dd-row > dd-cell:nth-child(3n+3),
    
    dd-table[dd-cols="4"] > dd-cell:nth-child(4n+3),
    dd-table[dd-cols="4"] > dd-row > dd-cell:nth-child(4n+3),
    dd-table[dd-cols="4"] > dd-table-body > dd-row > dd-cell:nth-child(4n+3),
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+3),
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+3),
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+3)
    
    {
        display: var(--c3-display, var(--cell-display, block));
        padding: var(--c3-padding, var(--cell-padding, block));
        color: var(--c3-color, var(--cell-color, inherit));
        cursor: var(--c3-cursor, var(--cell-cursor, auto));
        background-color: var(--c3-bg, var(--cell-bg, inherit));
        font-weight: var(--c3-font-weight, var(--cell-font-weight, inherit));
        font-size: var(--c3-font-size, inherit);
        text-transform: var(--c3-text-transform, inherit);
        text-align: var(--c3-text-align, inherit);
        line-height: var(--c3-line-height, inherit);
        justify-self: var(--c3-justify-self, var(--cell-justify-content, initial))
    }
    
    /* C3 HOVERED */
    /* C3 HOVERED */
    /* C3 HOVERED */
    
    dd-table[dd-cols="3"] > dd-cell:nth-child(3n+3):hover,
    dd-table[dd-cols="3"] > dd-row > dd-cell:nth-child(3n+3):hover,
    dd-table[dd-cols="3"] > dd-table-body > dd-row > dd-cell:nth-child(3n+3):hover,
    
    dd-table[dd-cols="4"] > dd-cell:nth-child(4n+3):hover,
    dd-table[dd-cols="4"] > dd-row > dd-cell:nth-child(4n+3):hover,
    dd-table[dd-cols="4"] > dd-table-body > dd-row > dd-cell:nth-child(4n+3):hover,
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+3):hover,
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+3):hover,
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+3):hover
    
    {
        display: var(--c3-hover-display, var(--cell-display, block));
        color: var(--c3-hover-color, var(--c3-color, var(--cell-color, inherit)));
        background-color: var(--c3-hover-bg, var(--cell-bg, inherit));
    }
    
    /* C4 */
    /* C4 */
    /* C4 */
    
    dd-table[dd-cols="4"] > dd-cell:nth-child(4n+4),
    dd-table[dd-cols="4"] > dd-row > dd-cell:nth-child(4n+4),
    dd-table[dd-cols="4"] > dd-table-body > dd-row > dd-cell:nth-child(4n+4),
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+4),
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+4),
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+4)
    
    {
        display: var(--c4-display, var(--cell-display, block));
        padding: var(--c4-padding, var(--cell-padding, block));
        color: var(--c4-color, var(--cell-color, inherit));
        cursor: var(--c4-cursor, var(--cell-cursor, auto));
        background-color: var(--c4-bg, var(--cell-bg, inherit));
        font-weight: var(--c4-font-weight, var(--cell-font-weight, inherit));
        font-size: var(--c4-font-size, inherit);
        text-transform: var(--c4-text-transform, inherit);
        text-align: var(--c4-text-align, inherit);
        line-height: var(--c4-line-height, inherit);
        justify-self: var(--c4-justify-self, var(--cell-justify-content, initial))
    }
    
    /* C4 HOVERED */
    /* C4 HOVERED */
    /* C4 HOVERED */
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+5):hover,
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+5):hover,
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+5):hover
    
    {
        display: var(--c4-hover-display, var(--cell-display, block));
        color: var(--c4-hover-color, var(--c4-color, var(--cell-color, inherit)));
        background-color: var(--c4-hover-bg, var(--cell-bg, inherit));
    }
    
    /* C5 */
    /* C5 */
    /* C5 */
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+5),
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+5),
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+5)
    
    {
        display: var(--c5-display, var(--cell-display, block));
        padding: var(--c5-padding, var(--cell-padding, block));
        color: var(--c5-color, var(--cell-color, inherit));
        cursor: var(--c5-cursor, var(--cell-cursor, auto));
        background-color: var(--c5-bg, var(--cell-bg, inherit));
        font-weight: var(--c5-font-weight, var(--cell-font-weight, inherit));
        font-size: var(--c5-font-size, inherit);
        text-transform: var(--c5-text-transform, inherit);
        text-align: var(--c5-text-align, inherit);
        line-height: var(--c5-line-height, inherit);
        justify-self: var(--c5-justify-self, var(--cell-justify-content, initial))
    }
    
    /* C5 HOVERED */
    /* C5 HOVERED */
    /* C5 HOVERED */
    
    dd-table[dd-cols="5"] > dd-cell:nth-child(5n+5):hover,
    dd-table[dd-cols="5"] > dd-row > dd-cell:nth-child(5n+5):hover,
    dd-table[dd-cols="5"] > dd-table-body > dd-row > dd-cell:nth-child(5n+5):hover
    
    {
        display: var(--c5-hover-display, var(--cell-display, block));
        color: var(--c5-hover-color, var(--c5-color, var(--cell-color, inherit)));
        background-color: var(--c5-hover-bg, var(--cell-bg, inherit));
    }
    
    /* POPUP */
    /* POPUP */
    /* POPUP */
    
    dd-popup-container {
        position: absolute;
        width: 100%;
        height: 100%;
        top: 0;
        left: 0;
        background: rgba(0,0,0,.5);
        display: flex;
        align-items: center;
        justify-content: center;
        z-index: 999999999;
    }
    
    dd-popup {
        display: block;
        width: 100%;
        position: relative;
    }
    
    dd-popup[dd-top] {
        align-self: flex-start;
        margin-top: 4em;
    }
    
    dd-x {
        box-sizing: border-box;
        display: block;
        position: absolute;
        top: .3em;
        right: .3em;
        font-weight: bold;
        color: #000;
        font-family: sans-serif;
        font-size: 2rem;
        line-height: 1.5rem;
        opacity: .4;
        border-radius: .5rem;
        cursor: pointer;
    }

    dd-x::before {
        box-sizing: border-box;
        content: "\\00D7";
    }

    dd-x:hover {
        background: rgba(0,0,0,.1);
        opacity: 1;
    }
    
    /* MENU */
    /* MENU */
    /* MENU */
    
    dd-menu, dd-select {
        --button-justify-content: flex-start;
        
        display: none;
        position: absolute;
        flex-direction: column;
        width: max-content;
        white-space: nowrap;
        overflow: auto;
    }
    
    dd-menu.dd-showing, dd-select.dd-showing {
        display: flex;
    }
    
    /* ARRAY */
    /* ARRAY */
    /* ARRAY */
    
    dd-array {
        display: none !important;
    }
    
    /* HR */
    /* HR */
    /* HR */
    
    dd-hr {
        display: block;
        width: 100%;
        background: var(--hr-color, #999);
        height: var(--hr-width, 1px);
        margin: var(--hr-margin, 0) 0;
    }
    
    /* DD-LINK */
    /* DD-LINK */
    /* DD-LINK */
    
    *[dd-link] {
        padding: 0;
    }
    
    *[dd-link] > a {
        display: inherit;
        width: 100%;
        height: 100%;
        gap: inherit;
        justify-content: inherit;
        align-items: inherit;
    }
    
    *[dd-link]:hover > a, *[dd-link].dd-active > a {
        color: inherit;
        font-size: inherit;
        font-weight: inherit;
    }
    
    dd-button[dd-link] > a, dd-tab[dd-link] > a, dd-checkbox[dd-link] > a, dd-radio[dd-link] > a, dd-option[dd-link] > a {
        padding: var(--button-padding, 0);
    }
    
    dd-cell[dd-link] > a {
        padding: var(--cell-padding, 0);
    }

    /* DD-FILE-UPLOADER */

    dd-file-uploader.dragover {
        outline: var(--dragover-outline, none);
        outline-offset: var(--dragover-outline-offset, none);
    }

    /* DD-SLIDER */

    dd-slider {
        display: flex;
        align-items: center;
        width: 100%;
        height: var(--slider-height, 30px);
        padding: 0 calc(0.5 * var(--slider-height, 30px));
    }

    dd-slider .dd-left-bar {
        position: relative;
        width: 0%;
        background: #00b6f0;
        height: calc(0.5 * var(--slider-height, 30px));
        border-radius: 100vw;
    }

    dd-slider .dd-left-bar::after {
        content: '';
        cursor: pointer;
        position: absolute;
        left: 100%;
        top: 50%;
        transform: translateX(-50%) translateY(-50%);
        height: 200%;
        aspect-ratio: 1 / 1;
        border-radius: 100px;
        background: black;
    }

    dd-slider .dd-right-bar {
        background: #bbb;
        height: calc(0.5 * var(--slider-height, 30px));
        flex-grow: 1;
        border-radius: 100vw;
    }

    /*
     * Single element CSS spinner
     * Source: https://projects.lukehaas.me/css-loaders/
     */
    .dd-spinner,
    .dd-spinner:before,
    .dd-spinner:after {
        border-radius: 50%;
        width: 2.5em;
        height: 2.5em;
        -webkit-animation-fill-mode: both;
        animation-fill-mode: both;
        -webkit-animation: load7 1s infinite ease-in-out;
        animation: load7 1s infinite ease-in-out;
    }
    .dd-spinner {
        color: var(--spinner-color, #999);
        font-size: var(--spinner-size, inherit);
        position: relative;
        text-indent: -9999em;
        -webkit-transform: translateZ(0);
        -ms-transform: translateZ(0);
        transform: translateZ(0) translateY(-100%);
        -webkit-animation-delay: -0.16s;
        animation-delay: -0.16s;
    }
    .dd-spinner:before,
    .dd-spinner:after {
        content: '';
        position: absolute;
        top: 0;
    }
    .dd-spinner:before {
        left: -3.5em;
        -webkit-animation-delay: -0.32s;
        animation-delay: -0.32s;
    }
    .dd-spinner:after {
        left: 3.5em;
    }
    @-webkit-keyframes load7 {
        0%,
    80%,
    100% {
        box-shadow: 0 2.5em 0 -1.3em;
    }
    40% {
        box-shadow: 0 2.5em 0 0;
    }
    }
    @keyframes load7 {
        0%,
    80%,
    100% {
        box-shadow: 0 2.5em 0 -1.3em;
    }
    40% {
        box-shadow: 0 2.5em 0 0;
    }
    }
`;
    
    document.head.append(style);
    window.customElements.define("dd-button", DD_Button);
    window.customElements.define("dd-tag", DD_Tag);
    window.customElements.define("dd-header", DD_Header);
    window.customElements.define("dd-label", DD_Label);
    window.customElements.define("dd-checkbox", DD_Checkbox);
    window.customElements.define("dd-toggle", DD_Toggle);
    window.customElements.define("dd-select", DD_Select);
    window.customElements.define("dd-option", DD_Option);
    window.customElements.define("dd-input", DD_Input);
    window.customElements.define("dd-table", DD_Table);
    window.customElements.define("dd-table-header", DD_TableHeader);
    window.customElements.define("dd-table-body", DD_TableBody);
    window.customElements.define("dd-row", DD_Row);
    window.customElements.define("dd-cell", DD_Cell);
    window.customElements.define("dd-tooltip", DD_Tooltip);
    window.customElements.define("dd-menu", DD_Menu);
    window.customElements.define("dd-data", DD_Data);
    window.customElements.define("dd-array", DD_Array);
    window.customElements.define("dd-tab", DD_Tab);
    window.customElements.define("dd-area", DD_Area);
    window.customElements.define("dd-radio", DD_Radio);
    window.customElements.define("dd-hr", DD_HorizontalRule);
    window.customElements.define("dd-l", DD_L);
    window.customElements.define("dd-file-uploader", DD_FileUploader);
    window.customElements.define("dd-slider", DD_Slider);
    window.customElements.define("dd-spinner", DD_Spinner);
    window.customElements.define("dd-selected-option", DD_SelectedOption);

    window.addEventListener("DOMContentLoaded", function() {
        document.body.classList.add('dd-using-mouse');
        
        // Let the document know when the mouse is being used
        document.body.addEventListener('mousedown', function(event) {
            document.body.classList.add('dd-using-mouse');
        });
        
        // Re-enable focus styling when Tab is pressed
        document.body.addEventListener('keydown', function(event) {
            if (DD_Components.isKeyTab(event.keyCode)) {
                document.body.classList.remove('dd-using-mouse');
            } else if (!DD_Components.isKeyCtrl(event.keyCode) 
                    && !DD_Components.isKeyShift(event.keyCode)
                    && !DD_Components.isKeyEnter(event.keyCode)
                    && !DD_Components.isKeySpace(event.keyCode)) {
                document.body.classList.add('dd-using-mouse');
            }
        });
        
        // Hide popups
        DD_Popup.closeAll();
    });
})();
