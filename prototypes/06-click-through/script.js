const focused = document.getElementById("focused");
const active = document.getElementById("active");
const foreground = document.getElementById("foreground");
const minimized = document.getElementById("minimized");
const maximized = document.getElementById("maximized");
const size = document.getElementById("size");

const mousePosition = document.getElementById("mouse-position");
const mouseInside = document.getElementById("mouse-inside");
const mouseLeft = document.getElementById("mouse-left");
const mouseMiddle = document.getElementById("mouse-middle");
const mouseRight = document.getElementById("mouse-right");

const lastKey = document.getElementById("last-key");
const ctrl = document.getElementById("ctrl");
const shift = document.getElementById("shift");
const alt = document.getElementById("alt");

const nativeState = document.getElementById("native-state");
const webviewState = document.getElementById("webview-state");

const mouseMoves = document.getElementById("mouse-moves");
const clicks = document.getElementById("clicks");
const keyDowns = document.getElementById("key-downs");
const activates = document.getElementById("activates");
const focusChanges = document.getElementById("focus-changes");

const eventLog = document.getElementById("event-log");

// P6 hit-test inspector
const insideRegion = document.getElementById("inside-region");
const expectedHitTest = document.getElementById("expected-hit-test");
const regionPointer = document.getElementById("region-pointer");


// Native interactive RECT:
//
// left   = 100
// top    = 100
// right  = 500
// bottom = 350

const TEST_REGION = {
    left: 100,
    top: 100,
    right: 500,
    bottom: 350
};


function isInsideTestRegion(x, y)
{
    return (
        x >= TEST_REGION.left &&
        x < TEST_REGION.right &&
        y >= TEST_REGION.top &&
        y < TEST_REGION.bottom
    );
}


function updateHitTestInspector(x, y)
{
    const inside = isInsideTestRegion(x, y);

    insideRegion.textContent = inside ? "true" : "false";

    expectedHitTest.textContent =
        inside ? "HTCLIENT" : "HTTRANSPARENT";

    regionPointer.textContent =
        inside ? "captured" : "pass-through";
}


// Initial state

nativeState.textContent = "Waiting for native data...";
webviewState.textContent = "Ready";

focused.textContent = "false";
active.textContent = "false";
foreground.textContent = "false";
minimized.textContent = "false";
maximized.textContent = "false";
size.textContent = "0 × 0";

mousePosition.textContent = "0, 0";
mouseInside.textContent = "false";
mouseLeft.textContent = "false";
mouseMiddle.textContent = "false";
mouseRight.textContent = "false";

lastKey.textContent = "0";
ctrl.textContent = "false";
shift.textContent = "false";
alt.textContent = "false";

mouseMoves.textContent = "0";
clicks.textContent = "0";
keyDowns.textContent = "0";
activates.textContent = "0";
focusChanges.textContent = "0";

insideRegion.textContent = "false";
expectedHitTest.textContent = "HTTRANSPARENT";
regionPointer.textContent = "waiting";


// Event log placeholders

eventLog.innerHTML = "";

for (let i = 0; i < 8; i++)
{
    const line = document.createElement("div");

    line.className = "event";
    line.textContent = "Waiting for Win32...";

    eventLog.appendChild(line);
}