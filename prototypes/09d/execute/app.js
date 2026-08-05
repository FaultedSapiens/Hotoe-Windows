"use strict";

const capabilities = [
    {
        method: "fx.ping",
        js: "fx.ping()",
        params: {},
        implementation: "Ping returns pong from a native capability."
    },
    {
        method: "fx.version",
        js: "fx.version()",
        params: {},
        implementation: "Version returns the runtime version string."
    },
    {
        method: "fx.closeApplication",
        js: "CLOSE()",
        params: {},
        implementation: "Close asks RuntimeHost to terminate the application."
    },
    {
        method: "fx.recalculateInputRegions",
        js: "SIRs()",
        params: {},
        implementation: "SIRs asks native to rebuild interactive input regions."
    },
    {
        method: "fx.pushString",
        js: "push(\"Hello\")",
        params: { data: "Hello" },
        implementation: "IPC pushes a string into the platform backend."
    }
];

const graphNodes = [
    ["Application", "intent", 88, 78],
    ["Fluent-X JS", "fx.*", 300, 78],
    ["JSON RPC", "request", 520, 78],
    ["Dispatcher", "lookup", 740, 78],
    ["Capability", "native API", 950, 78],
    ["Backend", "platform", 950, 266],
    ["Response", "json", 740, 266],
    ["Promise", "resolve", 520, 266]
];

const graphEdges = [
    [0, 1],
    [1, 2],
    [2, 3],
    [3, 4],
    [4, 5],
    [5, 6],
    [6, 7],
    [7, 1]
];

const state = {
    rows: new Map(),
    particles: [],
    activeNodes: new Map(),
    selectedCapability: capabilities[0]
};

const svg = document.getElementById("runtimeGraph");
const edgeLayer = document.getElementById("edges");
const nodeLayer = document.getElementById("nodes");
const particleLayer = document.getElementById("particles");
const rows = document.getElementById("rpcRows");

function nodePoint(index) {
    const node = graphNodes[index];
    return {
        x: node[2],
        y: node[3]
    };
}

function pathBetween(a, b) {
    const start = nodePoint(a);
    const end = nodePoint(b);
    const dx = (end.x - start.x) * .42;
    return `M${start.x},${start.y} C${start.x + dx},${start.y} ${end.x - dx},${end.y} ${end.x},${end.y}`;
}

function makeSvg(tag, attributes = {}) {
    const node = document.createElementNS("http://www.w3.org/2000/svg", tag);

    Object.entries(attributes).forEach(([key, value]) => {
        node.setAttribute(key, value);
    });

    return node;
}

function renderGraph() {
    graphEdges.forEach(([from, to]) => {
        edgeLayer.appendChild(
            makeSvg("path", {
                class: "runtime-edge",
                d: pathBetween(from, to)
            })
        );
    });

    graphNodes.forEach(([title, subtitle, x, y], index) => {
        const group = makeSvg("g", {
            transform: `translate(${x - 70}, ${y - 32})`,
            "data-node": String(index)
        });

        group.appendChild(makeSvg("rect", {
            class: "node-shell",
            width: "140",
            height: "64",
            rx: "6"
        }));

        const titleNode = makeSvg("text", {
            class: "node-title",
            x: "16",
            y: "28"
        });
        titleNode.textContent = title;

        const subtitleNode = makeSvg("text", {
            class: "node-subtitle",
            x: "16",
            y: "47"
        });
        subtitleNode.textContent = subtitle;

        group.append(titleNode, subtitleNode);
        nodeLayer.appendChild(group);
    });
}

function capabilityFor(method) {
    return capabilities.find((capability) => capability.method === method) || {
        method,
        js: `invoke("${method}")`,
        params: {},
        implementation: "Dispatcher returns an unknown capability error."
    };
}

function renderCapabilityList() {
    const list = document.getElementById("capabilityList");
    list.textContent = "";

    capabilities.forEach((capability) => {
        const button = document.createElement("button");
        button.textContent = capability.method;
        button.className =
            capability === state.selectedCapability
                ? "active"
                : "";

        button.addEventListener("click", () => {
            state.selectedCapability = capability;
            renderCapabilityList();
            renderCapabilityDetails();
        });

        list.appendChild(button);
    });
}

function renderCapabilityDetails() {
    const capability = state.selectedCapability;
    document.getElementById("capabilityName").textContent =
        capability.method;

    const payload = {
        id: "n",
        method: capability.method,
        params: capability.params
    };

    const steps = [
        ["JS API", capability.js],
        ["RPC payload", JSON.stringify(payload)],
        ["Dispatcher lookup", `FindCapability("${capability.method}")`],
        ["Native implementation", capability.implementation],
        ["Response", "RuntimeResponse resolves the pending Promise"]
    ];

    const list = document.getElementById("capabilitySteps");
    list.textContent = "";

    steps.forEach(([label, value]) => {
        const item = document.createElement("li");
        item.innerHTML = `${label}<br><code></code>`;
        item.querySelector("code").textContent = value;
        list.appendChild(item);
    });
}

function activateNode(index) {
    state.activeNodes.set(index, performance.now() + 520);
}

function spawnParticle(id, ok, reverse = false) {
    const circle = makeSvg("circle", {
        class: "particle",
        r: "6",
        fill: ok === false ? "var(--red)" : reverse ? "var(--green)" : "var(--blue)"
    });

    particleLayer.appendChild(circle);

    state.particles.push({
        id,
        element: circle,
        born: performance.now(),
        duration: reverse ? 1120 : 1380,
        reverse,
        ok
    });
}

function routeProgress(progress, reverse) {
    const route = reverse
        ? [7, 6, 5, 4, 3, 2, 1]
        : [0, 1, 2, 3, 4, 5, 6, 7];

    const scaled = progress * (route.length - 1);
    const segment = Math.min(route.length - 2, Math.floor(scaled));
    const local = scaled - segment;
    const a = nodePoint(route[segment]);
    const b = nodePoint(route[segment + 1]);

    activateNode(route[segment]);
    activateNode(route[segment + 1]);

    return {
        x: a.x + (b.x - a.x) * local,
        y: a.y + (b.y - a.y) * local
    };
}

function animate() {
    const now = performance.now();

    state.activeNodes.forEach((until, index) => {
        const node = nodeLayer.querySelector(`[data-node="${index}"]`);
        if (!node) return;

        if (until > now) {
            node.classList.add("node-active");
        } else {
            node.classList.remove("node-active");
            state.activeNodes.delete(index);
        }
    });

    state.particles = state.particles.filter((particle) => {
        const progress = (now - particle.born) / particle.duration;

        if (progress >= 1) {
            particle.element.remove();
            return false;
        }

        const point = routeProgress(
            Math.max(0, Math.min(1, progress)),
            particle.reverse
        );

        particle.element.setAttribute("cx", point.x);
        particle.element.setAttribute("cy", point.y);
        particle.element.setAttribute("opacity", String(1 - progress * .35));

        return true;
    });

    requestAnimationFrame(animate);
}

function shortJson(value) {
    const text = JSON.stringify(value);
    return text.length > 44 ? text.slice(0, 41) + "..." : text;
}

function rowFor(id) {
    if (state.rows.has(id)) {
        return state.rows.get(id);
    }

    const row = document.createElement("tr");
    row.innerHTML = "<td></td><td></td><td></td><td></td><td></td><td></td><td></td>";
    rows.prepend(row);
    state.rows.set(id, row);
    return row;
}

function updateRow(detail, status) {
    const row = rowFor(detail.id);
    const cells = row.children;
    const capability = capabilityFor(detail.method);

    row.className = status;
    cells[0].textContent = detail.id;
    cells[1].innerHTML = `<code>${detail.method}</code>`;
    cells[2].textContent = shortJson(detail.params || {});
    cells[3].textContent = capability.method;
    cells[4].textContent = detail.duration ? `${detail.duration.toFixed(1)} ms` : "pending";
    cells[5].textContent = status;
    cells[6].textContent =
        detail.error ||
        detail.result ||
        "";
}

function invokeAction(action) {
    if (action === "ping") return fx.ping();
    if (action === "version") return fx.version();
    if (action === "sirs") return SIRs();
    if (action === "push") return push("Hello");
    if (action === "close") return CLOSE();
    return Promise.resolve();
}

function wireEvents() {
    document.querySelectorAll("[data-action]").forEach((button) => {
        button.addEventListener("click", () => {
            invokeAction(button.dataset.action)
                .catch(() => {});
        });
    });

    window.addEventListener("fluentx:request", (event) => {
        updateRow(event.detail, "pending");
        spawnParticle(event.detail.id, true, false);
        state.selectedCapability = capabilityFor(event.detail.method);
        renderCapabilityList();
        renderCapabilityDetails();
    });

    window.addEventListener("fluentx:response", (event) => {
        updateRow(
            event.detail,
            event.detail.ok ? "ok" : "error"
        );
        spawnParticle(event.detail.id, event.detail.ok, true);
    });

    window.addEventListener("focusEvent", (event) => {
        document.getElementById("focusState").textContent =
            String(event.detail);
    });

    window.addEventListener("busMessage", (event) => {
        document.getElementById("busState").textContent =
            event.detail;
    });
}

renderGraph();
renderCapabilityList();
renderCapabilityDetails();
wireEvents();
requestAnimationFrame(animate);

window.addEventListener("DOMContentLoaded", () => {
    SIRs().catch(() => {});
});
