
class LT_Icon extends HTMLElement {
    constructor() {
        super();
    }

    connectedCallback() {
        const iconId = this.getAttribute("lt-icon");
        const iconName = iconId.split("/")[1];
        if (iconName in g.materialIcons) {
            this.innerHTML = `&#x${g.materialIcons[iconName]};`;
        }
    }
}

var g = {
    ws: null,
};

function getLocation() {
    return {
        href: location.href,
        pathname: location.pathname,
        host: location.host,
        hostname: location.hostname,
        search: location.search,
    }
}

function requestUpdate(events) {
    const fragmentId = events[0].fragment_id;
    const fragChildren = document.querySelectorAll(`.lt_fragment_container[data-lt-fragment-id="${fragmentId}"] > *`);
    for (const child of fragChildren) {
        child.style.setProperty("--opacity", 0.5);
        child.style.setProperty("--transition-duration", "0.8s");
    }

    wsSendObj({
        type: "update",
        location: getLocation(),
        events,
    });
}

function btnClick(event) {
    requestUpdate([{
        type: "click",
        widget_id: event.currentTarget.getAttribute("data-lt-id"),
        fragment_id: event.currentTarget.getAttribute("data-lt-fragment-id"),
    }]);
}

function mslChange(oldValue, newValue, elem) {
    requestUpdate([{
        type: "change",
        widget_id: elem.getAttribute("data-lt-id"),
        fragment_id: elem.getAttribute("data-lt-fragment-id"),
        old_value: oldValue,
        new_value: newValue ? newValue : null,
    }]);
}

function cbxAnyChange(groupName) {
    const checked = DD_Checkbox.getCheckedInGroup(groupName);
    const newValue = [];
    for (const c of checked) {
        newValue.push(c.value);
    }

    const group = DD_Checkbox.getGroup(groupName);
    const fragmentId = group.checkboxes[0].getAttribute("data-lt-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: groupName,
        fragment_id: fragmentId,
        new_value: newValue,
    }]);
}

function radChange(groupName) {
    const group = DD_Radio.getGroup(groupName);
    const fragmentId = group.checkboxes[0].getAttribute("data-lt-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: groupName,
        fragment_id: fragmentId,
        new_value: DD_Radio.getGroupValue(groupName),
    }]);
}

function dfChange(cell) {
    const rowData = cell.getRow().getData();
    const rowIndex = rowData.lt_original_index;
    const columnName = cell.getField();
    const newValue = cell.getValue();
    const tableElem = cell.getTable().element;

    requestUpdate([{
        type: "change",
        widget_id: tableElem.getAttribute("data-lt-id"),
        fragment_id: tableElem.getAttribute("data-lt-fragment-id"),

        row_index: rowIndex,
        column_name: columnName,
        new_value: newValue,
    }]);
}

function inpInput(event) {
    const newValue = event.currentTarget.value;
    const id = event.currentTarget.parentElement.parentElement.getAttribute("data-lt-id");
    const fragmentId = event.currentTarget.parentElement.parentElement.getAttribute("data-lt-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: id,
        fragment_id: fragmentId,
        new_value: newValue,
    }]);
}

function inpChange(event) {
    const newValue = event.currentTarget.value;
    const id = event.currentTarget.parentElement.parentElement.getAttribute("data-lt-id");
    const fragmentId = event.currentTarget.parentElement.parentElement.getAttribute("data-lt-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: id,
        fragment_id: fragmentId,
        new_value: newValue,
    }]);
}

function clrChange(event) {
    const newValue = event.currentTarget.value;
    const id = event.currentTarget.getAttribute("data-lt-id");
    const fragmentId = event.currentTarget.getAttribute("data-lt-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: id,
        fragment_id: fragmentId,
        new_value: newValue,
    }]);
}

function codeChange(event) {
    // TODO
}

function applyCSS(elem, css) {
    for (const [key, value] of Object.entries(css)) {
        elem.style[key] = value;
    }
}

function createAppElement(parent, props, fragmentId) {
    let newElements = [];

    if (props.type == "html") {
        const elem = document.createElement(props.tag);

        for (const [key, value] of Object.entries(props.attributes)) {
            elem.setAttribute(key, value);
        }

        for (const [key, value] of Object.entries(props.css)) {
            elem.style[key] = value;
        }

        elem.innerHTML = props.inner_html;

        newElements.push(elem);
    } else if (props.type == "container") {
        const elem = document.createElement("div");

        for (const [key, value] of Object.entries(props.css)) {
            elem.style[key] = value;
        }

        if (props.is_fragment_container) {
            elem.classList.add("lt_fragment_container");
            fragmentId = props.fragment_id;
        }

        newElements.push(elem);
    } else if (props.type == "button") {
        let elem = document.querySelector(`[data-lt-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("dd-button");

            let iconHTML = "";
            if (props.icon) iconHTML = `<lt-icon lt-icon="${props.icon}"></lt-icon>`;

            elem.innerHTML = `${iconHTML} ${props.label}`;
            elem.classList.add("lt-button");

            if (props.style) {
                elem.classList.add(`lt-button-style-${props.style}`);
            }

            elem.setAttribute("data-lt-container-id", props.container_id);
            elem.setAttribute("data-lt-local-id", props.local_id);
            elem.setAttribute("data-lt-id", props.id);
            elem.addEventListener("click", btnClick);
        } else {
            elem.setAttribute("dd-reconnecting", "");
            if (DD_Components.isFocused(elem)) {
                requestAnimationFrame(()=>{
                    elem.focus();
                });
            }
        }

        newElements.push(elem);
    } else if (props.type == "text") {
        const elem = document.createElement("p");
        elem.innerText = props.text;
        newElements.push(elem);
    } else if (props.type == "text_input") {
        let elem = document.querySelector(`[data-lt-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("dd-input");
            elem.classList.add("lt-text-input");

            elem.setAttribute("data-lt-id", props.id);
            elem.setAttribute("placeholder", props.placeholder);
            elem.setAttribute("value", props.value);
            //elem.setAttribute("oninput", "inpInput(event)");
            elem.setAttribute("onchange", "inpChange(event)");
        } else {
            elem.setAttribute("dd-reconnecting", "");

            if (elem.input.value != props.value) {
                elem.setAttribute("value", props.value);
            }

            if (DD_Components.isFocused(elem.input)) {
                requestAnimationFrame(()=>{
                    elem.input.focus();
                });
            }
        }

        newElements.push(elem);
    } else if (props.type == "selectbox") {
        const inpElem = document.createElement("dd-input");
        inpElem.classList.add("lt-selectbox");

        for (const [key, value] of Object.entries(props.css)) {
            inpElem.style[key] = value;
        }

        const slcElem = document.createElement("dd-select");
        slcElem.classList.add("lt-selectbox");

        if (props["multiple"]) {
            slcElem.setAttribute("dd-multiple", "");
        }

        slcElem.setAttribute("dd-onchange", "mslChange()");

        slcElem.setAttribute("dd-width", "anchor");
        slcElem.setAttribute("data-lt-container-id", props.container_id);
        slcElem.setAttribute("data-lt-local-id", props.local_id);
        slcElem.setAttribute("data-lt-id", props.id);

        for (const op of props.options) {
            const optElem = document.createElement("dd-option");
            optElem.setAttribute("value", op);
            optElem.setAttribute("dd-text", op);
            slcElem.appendChild(optElem);
        }

        newElements.push(inpElem);
        newElements.push(slcElem);

        if (props.value != null) {
            requestAnimationFrame(() => slcElem.setValue(props.value, {silent: true}));
        }
    } else if (props.type == "color_picker") {
        let elem = document.querySelector(`[data-lt-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("input");
            elem.setAttribute("type", "color");
            elem.setAttribute("onchange", "clrChange(event)");
            elem.setAttribute("data-lt-container-id", props.container_id);
            elem.setAttribute("data-lt-local-id", props.local_id);
            elem.setAttribute("data-lt-id", props.id);
            elem.setAttribute("value", props.value);

            applyCSS(elem, props.css)
        } else {
            elem.value = props.value;
        }

        newElements.push(elem);
    } else if (props.type == "checkboxes") {
        const cbxGroup = DD_Checkbox.getGroup(props.id);

        if (!cbxGroup) {
            for (const op of props.options) {
                const elem = document.createElement("dd-checkbox");
                elem.setAttribute("dd-group", props.id);
                elem.setAttribute("dd-onanychange", "cbxAnyChange()");

                elem.innerText = op;
                elem.value = op;

                if (!props.multiple) {
                    if (props.value) {
                        elem.setAttribute("checked", "");
                    }
                } else {
                    if (props.value.includes(elem.value)) {
                        elem.setAttribute("checked", "");
                    } else {
                        elem.removeAttribute("checked");
                    }
                }

                newElements.push(elem);
            }
        } else {
            newElements = cbxGroup.checkboxes;
            for (const elem of newElements) {
                elem.setAttribute("dd-reconnecting", "");
            }
        }
    } else if (props.type == "radio") {
        for (const op of props.options) {
            const elem = document.createElement("dd-radio");
            elem.setAttribute("dd-group", props.id);

            elem.innerText = op;
            elem.value = op;

            if (props.value == op) {
                elem.setAttribute("dd-onanychange", "radChange()");
            }

            newElements.push(elem);
        }

        requestAnimationFrame(() => {
            DD_Radio.selectInGroup(props.id, props.value, {silent: true});
        })
    } else if (props.type == "image") {
        let elem = document.querySelector(`[data-lt-id="${props.id}"]`);

        if (!elem) {
            // elem = document.createElement("img");
            elem = props.img;

            elem.classList.add("lt-image");
            //elem.setAttribute("src", props.uri);
            if (props.width) elem.setAttribute("width", props.width);
            if (props.height) elem.setAttribute("height", props.height);
            elem.setAttribute("data-lt-id", props.id);
            applyCSS(elem, props.css);
        }

        newElements.push(elem);
    } else if (props.type == "dataframe") {
        let elem = document.querySelector(`[data-lt-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("div");

            elem.setAttribute("data-lt-container-id", props.container_id);
            elem.setAttribute("data-lt-local-id", props.local_id);
            elem.setAttribute("data-lt-id", props.id);

            elem.style["height"] = props.height;
            elem.classList.add("lt-dataframe");

            for (const [i, row] of props.initial_value.entries()) {
                row.lt_original_index = i+1;
            }

            let columns = [];

            if (("initial_value" in props) && props.initial_value.length) {
                for (const columnName of Object.keys(props.initial_value[0])) {
                    if (columnName == "lt_original_index") continue;

                    if (columnName in props.columns) {
                        const column = props.columns[columnName];
                        columns.push({
                            field: columnName,
                            title: columnName,
                            editor: column.editable ? "input" : null,
                            cellEdited: column.editable ? dfChange : null,
                        });
                    } else {
                        columns.push({
                            field: columnName,
                            title: columnName,
                            editor: null,
                        });
                    }
                }
            }

            const table = new Tabulator(elem, {
                data: props.initial_value,
                columns,
                layout: "fitFill",
                selectableRange: true,
                selectableRangeAutoFocus: false,
                clipboard: true,
                clipboardCopyRowRange: "range",
                clipboardCopyConfig:{
                    rowHeaders: false,
                    columnHeaders: false,
                },
                height: props.height,
                columnDefaults:{
                    headerSort: false,
                    editor: null,
                    resizable: "header",
                },
            });

            setTimeout(()=>{
                document.activeElement.blur();
            }, 0);
        } else {
            // TODO: NOTHING TO DO?
        }

        newElements.push(elem);

    } else if (props.type == "code") {
        const elem = document.createElement("div");
        applyCSS(elem, props.css);

        const textarea = document.createElement("textarea");
        elem.appendChild(textarea);

        requestAnimationFrame(() => {
            const cm = CodeMirror.fromTextArea(textarea, {
                mode: "julia",
                viewportMargin: Infinity,
                readOnly: true,
                indentWithTabs: false,
                indentUnit: 4,
                extraKeys: {
                    Tab: function(cm) {
                        const spaces = Array(cm.getOption("indentUnit") + 1).join(" ");
                        cm.replaceSelection(spaces, "end");
                    }
                }
            });

            cm.on("change", codeChange);
            cm.setValue(props.initial_value);
        });

        newElements.push(elem);
        newElements.push(elem);
    } else {
        console.error(`Unknown element type '${props.type}'`);
    }

    if (newElements.length) {
        for (const elem of newElements) {
            parent.appendChild(elem);
            elem.setAttribute("data-lt-fragment-id", fragmentId);
        }

        if (props.type == "container") {
            for (const child of props.children) {
                createAppElement(newElements[0], child, fragmentId);
            }
        }

        return newElements[0];
    } else {
        return null;
    }
}

function wsSendObj(obj) {
    console.log("Sending this:");
    console.log(obj);
    g.ws.send(JSON.stringify(obj));
}

function wsOnOpen() {
    console.log("wsOnOpen");
    wsSendObj({type: "update", location: getLocation(), events: []});
}

function getImages(props) {
    if (props.type == "image") {
        return [props];
    } else if (props.type == "container") {
        let result = [];
        for (const child of props.children) {
            result = result.concat(getImages(child));
        }
        return result;
    }
    return [];
}

async function preloadImages(root) {
    const images = getImages(root);

    const tasks = images.map(image => {
        image.img = new Image();
        image.img.src = image.src;
        return image.img.decode().then(() => image.img);
    });

    return Promise.all(tasks);
}

async function wsOnMessage(event) {
    //console.log("Receiving this (raw):");
    //console.log(event.data);

    const msg = JSON.parse(event.data);
    console.log("Receiving this (parsed):");
    console.log(msg);

    if (msg.type == "new_state") {
        // Preload images
        //-------------------
        await preloadImages(msg.root);

        const fragmentId = msg.root["fragment_id"];

        const oldFragContainer = document.querySelector(`.lt_fragment_container[data-lt-fragment-id="${fragmentId}"]`);
        const computedStyle = getComputedStyle(oldFragContainer.firstElementChild);
        oldFragContainer.style.visibility = "hidden";

        const newFragWrapper = document.createDocumentFragment();

        const newFragContainer = createAppElement(newFragWrapper, msg.root, "");
        for (const child of newFragContainer.children) {
            child.style.setProperty("--opacity", computedStyle.opacity);
            child.style.setProperty("--transition-duration", "0.15s");
        }

        oldFragContainer.parentElement.insertBefore(newFragWrapper, oldFragContainer);
        oldFragContainer.remove();

        // Remove checkbox groups that cease to exist
        //-----------------------------------------------
        while (DD_Components.removeItemFromArrayIfCondition(DD_Checkbox.groups, (entry) => (entry.checkboxes.length == 0)));

        // Initialize opacity transition
        setTimeout(() => {
            for (const child of newFragContainer.children) {
                child.style.setProperty("--opacity", 1);
            }
        }, 10);
    }
}

function wsOnClose(event) {
    console.log("wsOnClose");
}

function wsOnError(err) {
    console.log("wsOnError");
}

async function loadIconMap(url) {
    const text = await fetch(url).then(r => r.text());

    const map = {};
    for (const line of text.split(/\r?\n/)) {
        if (!line.trim()) continue;
        const [name, code] = line.trim().split(/\s+/);
        map[name] = code;
    }
    return map;
}

(async function main(){
    g.materialIcons = await loadIconMap("/Lit.jl/fonts/MaterialIconsOutlined-Regular.codepoints");

    g.ws = new WebSocket(`wss://${location.host}`, ["ws"]);
    g.ws.addEventListener("open", wsOnOpen);
    g.ws.addEventListener("message", wsOnMessage);
    g.ws.addEventListener("close", wsOnClose);
    g.ws.addEventListener("error", wsOnError);

    window.customElements.define("lt-icon", LT_Icon);
})();
