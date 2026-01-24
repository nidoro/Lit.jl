
class MG_Icon extends HTMLElement {
    constructor() {
        super();
    }

    connectedCallback() {
        const iconId = this.getAttribute("mg-icon");
        const iconName = iconId.split("/")[1];
        if (iconName in g.materialIcons) {
            this.innerHTML = `&#x${g.materialIcons[iconName]};`;
        }
    }
}

var g = {
    ws: null,
    devMode: false,
    nextRequestId: 1,
    lastValidRerunResponse: null,
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
    if (events.length) {
        const fragmentId = events[0].fragment_id;
        const fragChildren = document.querySelectorAll(`.mg_fragment_container[data-mg-fragment-id="${fragmentId}"] > *`);
        for (const child of fragChildren) {
            child.style.setProperty("--opacity", 0.5);
            child.style.setProperty("--transition-duration", "0.8s");
        }
    }

    wsSendObj({
        type: "request_rerun",
        location: getLocation(),
        request_id: g.nextRequestId++,
        events,
    });
}

function ackInvalidState() {
    wsSendObj({
        type: "ack_invalid_state",
    });
}

function btnClick(event) {
    requestUpdate([{
        type: "click",
        widget_id: event.currentTarget.getAttribute("data-mg-id"),
        fragment_id: event.currentTarget.getAttribute("data-mg-fragment-id"),
    }]);
}

function mslChange(oldValue, newValue, elem) {
    if (Array.isArray(newValue) && newValue.length == 0) {
        newValue = null;
    } else if (newValue == "") {
        newValue = null;
    }

    requestUpdate([{
        type: "change",
        widget_id: elem.getAttribute("data-mg-id"),
        fragment_id: elem.getAttribute("data-mg-fragment-id"),
        old_value: oldValue,
        new_value: newValue,
    }]);
}

function cbxAnyChange(groupName) {
    const checked = DD_Checkbox.getCheckedInGroup(groupName);
    const newValue = [];
    for (const c of checked) {
        newValue.push(c.value);
    }

    const group = DD_Checkbox.getGroup(groupName);
    const fragmentId = group.checkboxes[0].getAttribute("data-mg-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: groupName,
        fragment_id: fragmentId,
        new_value: newValue,
    }]);
}

function radChange(groupName) {
    const group = DD_Radio.getGroup(groupName);
    const fragmentId = group.checkboxes[0].getAttribute("data-mg-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: groupName,
        fragment_id: fragmentId,
        new_value: DD_Radio.getGroupValue(groupName),
    }]);
}

function isNumber(value) {
    return !isNaN(value);
}

function sendDFChanges(table) {
    if (table.mg_queued_changes.length == 0) return;

    const tableElem = table.element;

    requestUpdate([{
        type: "change",
        widget_id: tableElem.getAttribute("data-mg-id"),
        fragment_id: tableElem.getAttribute("data-mg-fragment-id"),
        changes: table.mg_queued_changes,
    }]);

    table.mg_queued_changes = [];
}

function dfChange(cell) {
    if ("mgIgnoreNextChange" in cell && cell.mgIgnoreNextChange) {
        cell.mgIgnoreNextChange = false;
        return;
    }

    const table = cell.getTable();

    const oldValue = cell.getOldValue();
    let newValue = cell.getValue();

    const rowData = cell.getRow().getData();
    const rowIndex = rowData.mg_original_index;
    const columnName = cell.getField();
    const tableElem = table.element;

    const columnConfig = table.mg_column_config[columnName];
    const columnType = columnConfig.type;
    const juliaType = columnConfig.julia_type;

    if (oldValue == newValue) return;

    let ignore_changes = false;

    if (columnType == "Number") {
        if (["", undefined, null].includes(newValue)) {
            if (columnConfig.required) {
                cell.mgIgnoreNextChange = true;
                cell.setValue(oldValue);
                ignore_changes = true;
                newValue = oldValue;
            } else {
                newValue = null;
            }
        } else if (isNumber(newValue)) {
            newValue = Number(newValue);
        } else if (columnConfig.required) {
            cell.mgIgnoreNextChange = true;
            cell.setValue(oldValue);
            ignore_changes = true;
            newValue = oldValue;
        } else {
            cell.setValue(null);
            newValue = null;
        }
    } else if (columnType == "String") {
        if ([undefined, null].includes(newValue)) {
            if (columnConfig.required) {
                cell.mgIgnoreNextChange = true;
                cell.setValue(oldValue);
                ignore_changes = true;
                newValue = oldValue;
            } else {
                newValue = null;
            }
        }
    }

    if (!ignore_changes) {
        table.mg_queued_changes.push({
            row_index: rowIndex,
            column_name: columnName,
            new_value: newValue,
        });

        requestAnimationFrame(() => sendDFChanges(table));
    }
}

function inpInput(event) {
    const newValue = event.currentTarget.value;
    const id = event.currentTarget.parentElement.parentElement.getAttribute("data-mg-id");
    const fragmentId = event.currentTarget.parentElement.parentElement.getAttribute("data-mg-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: id,
        fragment_id: fragmentId,
        new_value: newValue,
    }]);
}

function inpChange(event) {
    const newValue = event.currentTarget.value;
    const id = event.currentTarget.parentElement.parentElement.getAttribute("data-mg-id");
    const fragmentId = event.currentTarget.parentElement.parentElement.getAttribute("data-mg-fragment-id");

    requestUpdate([{
        type: "change",
        widget_id: id,
        fragment_id: fragmentId,
        new_value: newValue == "" ? null : newValue,
    }]);
}

function clrChange(event) {
    const newValue = event.currentTarget.value;
    const id = event.currentTarget.getAttribute("data-mg-id");
    const fragmentId = event.currentTarget.getAttribute("data-mg-fragment-id");

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
        if (key.startsWith("--")) {
            elem.style.setProperty(key, value);
        } else {
            elem.style[key] = value;
        }
    }
}

function applyAttributes(elem, attributes) {
    for (const [key, value] of Object.entries(attributes)) {
        elem.setAttribute(key, value);
    }
}

function coalesce(...args) {
    for (const arg of args) {
        if (arg !== null && arg !== undefined) {
            return arg;
        }
    }
    return null;
}

function getTabulatorColumnNumber(table, columnName) {
    const columns = table.getColumns();
    for (const [index, column] of columns.entries()) {
        if (column.getField() == columnName) {
            return index;
        }
    }
    return null;
}

function getTabulatorColumn(table, columnName) {
    const columns = table.getColumns();
    const index = getTabulatorColumnNumber(table, columnName);
    if (index != null) {
        return columns[index];
    }
    return null;
}

function createAppElement(parent, props, fragmentId) {
    let newElements = [];

    if (props.type == "html") {
        const elem = document.createElement(props.tag);

        applyCSS(elem, props.css);
        applyAttributes(elem, props.attributes);

        elem.innerHTML = props.inner_html;

        newElements.push(elem);
    } else if (props.type == "container") {
        const elem = document.createElement("div");

        elem.setAttribute("data-mg-id", props.id);
        applyCSS(elem, props.css);
        applyAttributes(elem, props.attributes);

        if (props.is_fragment_container) {
            elem.classList.add("mg_fragment_container");
            fragmentId = props.fragment_id;
        }

        // Sidebar
        //---------
        if (elem.classList.contains("mg-sidebar")) {
            let state = elem.classList.contains("mg-show") ? "open" : "closed";
            const oldElem = document.querySelector(`.mg-sidebar[data-mg-id="${props.id}"]`);

            if (oldElem) {
                if (oldElem.classList.contains("mg-show")) {
                    state = "open";
                } else {
                    state = "closed";
                }
            }

            MG_SetSidebarState(elem, state);
            requestAnimationFrame(() => MG_SetSidebarState(elem, state));
        }

        newElements.push(elem);
    } else if (props.type == "button") {
        let elem = document.querySelector(`[data-mg-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("dd-button");

            let iconHTML = "";
            if (props.icon) iconHTML = `<mg-icon mg-icon="${props.icon}"></mg-icon>`;

            elem.innerHTML = `${iconHTML} ${props.label}`;
            elem.classList.add("mg-button");

            if (props.style) {
                elem.classList.add(`mg-button-style-${props.style}`);
            }

            elem.setAttribute("data-mg-container-id", props.container_id);
            elem.setAttribute("data-mg-local-id", props.local_id);
            elem.setAttribute("data-mg-id", props.id);
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
        let elem = document.querySelector(`[data-mg-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("dd-input");
            elem.classList.add("mg-text-input");

            elem.setAttribute("data-mg-id", props.id);

            if (props.value != null) {
                elem.setAttribute("value", props.value);
            }

            elem.setAttribute("placeholder", props.placeholder);

            //elem.setAttribute("oninput", "inpInput(event)");
            elem.setAttribute("onchange", "inpChange(event)");
        } else {
            elem.setAttribute("dd-reconnecting", "");

            if (props.value && elem.input.value != props.value) {
                elem.setAttribute("value", props.value);
            } else if (!props.value && elem.input.value) {
                elem.setAttribute("value", "");
            }

            if (DD_Components.isFocused(elem.input)) {
                requestAnimationFrame(()=>{
                    elem.input.focus();
                });
            }
        }

        newElements.push(elem);
    } else if (props.type == "selectbox") {
        let inpElem = document.querySelector(`dd-input[data-mg-id="${props.id}"]`);
        let slcElem = document.querySelector(`dd-select[data-mg-id="${props.id}"]`);

        if (!inpElem) {
            inpElem = document.createElement("dd-input");
            inpElem.classList.add("mg-selectbox");

            for (const [key, value] of Object.entries(props.css)) {
                inpElem.style[key] = value;
            }

            slcElem = document.createElement("dd-select");
            slcElem.classList.add("mg-selectbox");

            if (props["multiple"]) {
                slcElem.setAttribute("dd-multiple", "");
            }

            slcElem.setAttribute("dd-placeholder", props["placeholder"]);
            slcElem.setAttribute("dd-onchange", "mslChange()");

            slcElem.setAttribute("dd-width", "anchor");
            slcElem.setAttribute("data-mg-container-id", props.container_id);
            slcElem.setAttribute("data-mg-local-id", props.local_id);
            slcElem.setAttribute("data-mg-id", props.id);

            for (const op of props.options) {
                const optElem = document.createElement("dd-option");
                optElem.setAttribute("value", op);
                optElem.setAttribute("dd-text", op);
                slcElem.appendChild(optElem);
            }
        }

        if (props.value != null) {
            requestAnimationFrame(() => slcElem.setValue(props.value, {silent: true}));
        }

        newElements.push(inpElem);
        newElements.push(slcElem);
    } else if (props.type == "color_picker") {
        let elem = document.querySelector(`[data-mg-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("input");
            elem.setAttribute("type", "color");
            elem.setAttribute("onchange", "clrChange(event)");
            elem.setAttribute("data-mg-container-id", props.container_id);
            elem.setAttribute("data-mg-local-id", props.local_id);
            elem.setAttribute("data-mg-id", props.id);

            let value = props.value ? props.value : "#999999";
            elem.setAttribute("value", value);

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
                elem.setAttribute("dd-silent", "");

                if (props.multiple) {
                    if (props.value.includes(elem.value)) {
                        elem.setAttribute("checked", "");
                    } else {
                        elem.removeAttribute("checked", "");
                    }
                } else {
                    if (props.value) {
                        elem.setAttribute("checked", "");
                    } else {
                        elem.removeAttribute("checked", "");
                    }
                }
            }

            requestAnimationFrame(() => {
                for (const elem of newElements) {
                    elem.removeAttribute("dd-silent");
                }
            });
        }

    } else if (props.type == "radio") {
        const group = DD_Radio.getGroup(props.id);

        if (!group) {
            for (const op of props.options) {
                const elem = document.createElement("dd-radio");
                elem.setAttribute("dd-group", props.id);

                elem.innerText = op;
                elem.value = op;

                if (op == props.options[0]) {
                    elem.setAttribute("dd-onanychange", "radChange()");
                }

                newElements.push(elem);
            }
        } else {
            const elems = document.querySelectorAll(`dd-radio[dd-group="${props.id}"]`);
            newElements = Array.from(elems);
            for (const elem of elems) {
                elem.setAttribute("dd-reconnecting", "");
            }
        }

        requestAnimationFrame(() => {
            DD_Radio.selectInGroup(props.id, props.value, {silent: true});
        })
    } else if (props.type == "image") {
        let elem = document.querySelector(`[data-mg-id="${props.id}"]`);

        if (!elem) {
            // elem = document.createElement("img");
            elem = props.img;

            elem.classList.add("mg-image");
            //elem.setAttribute("src", props.uri);
            if (props.width) elem.setAttribute("width", props.width);
            if (props.height) elem.setAttribute("height", props.height);
            elem.setAttribute("data-mg-id", props.id);
            applyCSS(elem, props.css);
        }

        newElements.push(elem);
    } else if (props.type == "dataframe") {
        let elem = document.querySelector(`[data-mg-id="${props.id}"]`);

        if (!elem) {
            elem = document.createElement("div");

            elem.setAttribute("data-mg-container-id", props.container_id);
            elem.setAttribute("data-mg-local-id", props.local_id);
            elem.setAttribute("data-mg-id", props.id);

            elem.style["height"] = props.height;
            elem.classList.add("mg-dataframe");

            const lining = document.createElement("div");
            lining.classList.add("mg-dataframe-lining");
            lining.setAttribute("data-mg-container-id", props.container_id);
            lining.setAttribute("data-mg-local-id", props.local_id);
            lining.setAttribute("data-mg-id", props.id);
            lining.setAttribute("data-mg-fragment-id", fragmentId);
            elem.appendChild(lining);

            for (const [i, row] of props.initial_value.entries()) {
                row.mg_original_index = i+1;
            }

            let columns = [];

            if (("initial_value" in props) && props.initial_value.length) {
                for (const columnName of Object.keys(props.initial_value[0])) {
                    if (columnName == "mg_original_index") continue;

                    let columnOptions = {
                        field: columnName,
                        title: columnName,
                        editor: null,
                    }

                    if (columnName in props.column_config) {
                        const config = props.column_config[columnName];
                        columnOptions = {
                            field: columnName,
                            title: columnName,
                            editor: config.editable ? (config.type == "Number" ? "number" : "input") : null,
                            cellEdited: config.editable ? dfChange : null,
                        }
                    }

                    columns.push(columnOptions);
                }
            }

            const table = new Tabulator(lining, {
                data: props.initial_value,
                columns,
                layout: "fitFill",
                selectableRange: true,
                selectableRangeAutoFocus: false,

                // NOTE: This activates Tabular.js handler for Delete/Backspace keydown
                // event, but it turns out that it is not good because it deletes
                // read-only cells too, and it also deletes the entire cell content
                // when the user is editing a specific cell, which is really bad UX.
                // selectableRangeClearCells: true,

                clipboard: true,
                clipboardCopyRowRange: "range",
                clipboardCopyConfig:{
                    rowHeaders: false,
                    columnHeaders: false,
                },
                clipboardPasteParser:"range",

                // NOTE: Unfortunately, we need to implement our own clipboard
                // paste action to prevent read-only cells from being edited.
                // TODO: Handle paste of a single value into multiple cells.
                clipboardPasteAction: function(clipboardData) {
                    // Get top-left cell of the active range
                    const range = table.getRanges()?.[0];
                    if(!range) return;

                    const startCell = range.getCells()[0][0];
                    const startRow = startCell.getRow().getPosition();
                    const rows = table.getRows();

                    clipboardData.forEach((pasteRow, rowOffset) => {
                        const row = rows[startRow + rowOffset - 1];
                        if(!row) return;

                        for (const [columnName, value] of Object.entries(pasteRow)) {
                            const column = getTabulatorColumn(table, columnName);
                            if (column.getDefinition().editor) {
                                const cell = row.getCell(column.getField());
                                if(!cell) return;
                                cell.setValue(value);
                            }

                        };
                    });
                },

                height: props.height,
                columnDefaults:{
                    headerSort: false,
                    editor: null,
                    resizable: "header",
                },
                editTriggerEvent:"dblclick"
            });

            table.mg_column_config = props.column_config;
            table.mg_queued_changes = [];

            // Handle Delete/Backspace
            //---------------------------------------
            lining.addEventListener("keydown", function(e) {
                if (document.activeElement.tagName == "INPUT") {
                    return;
                }

                if (e.key === "Delete" || e.key === "Backspace") {
                    let ranges = table.getRanges();

                    ranges.forEach(range => {
                        range.getCells().forEach(cells => {
                            cells.forEach(cell => {
                                if (cell.getColumn().getDefinition().editor) {
                                    cell.setValue(null);
                                }
                            });
                        });
                    });

                    e.preventDefault();
                }
            });

            setTimeout(()=> document.activeElement.blur(), 0);
        } else {
            const scrollY = elem.querySelector(".tabulator-tableholder").scrollTop;

            let refocus = null;
            if (elem.contains(document.activeElement)) {
                refocus = document.activeElement;
            }

            requestAnimationFrame(() => {
                elem.querySelector(".tabulator-tableholder").scrollTop = scrollY;
                if (refocus) refocus.focus();
            });
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
                lineNumbers: props.show_line_numbers,
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
            elem.setAttribute("data-mg-fragment-id", fragmentId);
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
    if (g.devMode) {
        console.log("Sending this:");
        console.log(obj);
    }
    g.ws.send(JSON.stringify(obj));
}

function wsOnOpen() {
    if (g.devMode) {
        console.log("Connected to net-layer");
    }
    requestUpdate([]);
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

async function displayRerunResponse(msg) {
    // Preload images
    //-------------------
    await preloadImages(msg.root);

    const fragmentId = msg.root["fragment_id"];

    const oldFragContainer = document.querySelector(`.mg_fragment_container[data-mg-fragment-id="${fragmentId}"]`);
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

    // Remove checkbox groups that ceased to exist
    //
    // TODO: I think this should actually be something done on the
    // disconnected event of the DD_Checkbox/DD_Radio component, when
    // the last checkbox of a group is removed.
    //-----------------------------------------------
    while (DD_Components.removeItemFromArrayIfCondition(DD_Checkbox.groups, (entry) => (entry.checkboxes.length == 0)));

    // Initialize opacity transition
    setTimeout(() => {
        for (const child of newFragContainer.children) {
            child.style.setProperty("--opacity", 1);
        }
    }, 10);

    g.lastValidRerunResponse = null;
}

async function wsOnMessage(event) {
    //console.log("Receiving this (raw):");
    //console.log(event.data);

    const msg = JSON.parse(event.data);
    g.devMode = "dev_mode" in msg ? msg["dev_mode"] : false;

    if (g.devMode) {
        console.log("Receiving this (parsed):");
        console.log(msg);
    }

    if (msg.type == "response_rerun") {
        if (msg.error == null) {
            // We only display the returned state if it is the response we are
            // *finally* waiting for. Otherwise, store this as the last valid
            // rerun response and wait for the next response.
            if (msg.request_id == g.nextRequestId-1) {
                displayRerunResponse(msg);
            } else {
                g.lastValidRerunResponse = msg;
            }
        } else if (msg.error.type == "InvalidState") {
            if (g.lastValidRerunResponse) {
                displayRerunResponse(g.lastValidRerunResponse);
            }
            ackInvalidState();
        }
    } else if (msg.type == "please_refresh") {
        location.reload();
    }
}

function wsOnClose(event) {
    if (g.devMode) {
        console.log("Disconnected from net-layer");
    }
}

function wsOnError(err) {
    console.error(err);
}

function MG_SetSidebarState(sidebarElem, state) {
    const btn = sidebarElem.querySelector(".mg-sidebar-toggle-button");

    if (state == "open") {
        sidebarElem.classList.add("mg-show");
        if (btn) {
            btn.innerHTML = sidebarElem.dataset.mgCloseLabel;
        }
    } else {
        sidebarElem.classList.remove("mg-show");
        if (btn) {
            btn.innerHTML = sidebarElem.dataset.mgOpenLabel;
        }
    }
}

function MG_ToggleSidebar(event) {
    const btn = event.currentTarget;
    const sidebarElem = btn.parentElement.parentElement;

    if (sidebarElem.classList.contains("mg-show")) {
        MG_SetSidebarState(sidebarElem, "closed");
    } else {
        MG_SetSidebarState(sidebarElem, "open");
    }
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
    g.materialIcons = await loadIconMap("/Magic.jl/fonts/MaterialIconsOutlined-Regular.codepoints");

    let wsEndpoint = `wss://${location.host}`;
    if (location.protocol == "http:") {
        wsEndpoint = `ws://${location.host}`;
    }

    g.ws = new WebSocket(wsEndpoint, ["ws"]);
    g.ws.addEventListener("open", wsOnOpen);
    g.ws.addEventListener("message", wsOnMessage);
    g.ws.addEventListener("close", wsOnClose);
    g.ws.addEventListener("error", wsOnError);

    window.customElements.define("mg-icon", MG_Icon);
})();
