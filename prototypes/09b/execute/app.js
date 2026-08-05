"use strict";

function log(
    message
)
{
    document.getElementById("log").textContent +=
        "\n" + message;
}

function sendPing()
{
    return fx.ping()
        .then(log)
        .catch(log);
}

function sendEcho()
{
    return invoke(
        "echo",
        {
            text: "FallenBrainLiquidEyes"
        }
    ).catch(() => {});
}

function sendUnknown()
{
    return invoke(
        "unknown",
        {}
    ).catch(() => {});
}

window.log = log;
window.sendPing = sendPing;
window.sendEcho = sendEcho;
window.sendUnknown = sendUnknown;

window.addEventListener(
    "focusEvent",
    function (event) {
        const focus =
            event.detail;

        document.getElementById("focusState").textContent =
            "focusEvent: " + focus;
    }
);

window.addEventListener(
    "DOMContentLoaded",
    function () {
        SIRs()
            .catch(log);

        setTimeout(
            () => sendPing(),
            500
        );

        setTimeout(
            () => sendPing(),
            1000
        );

        setTimeout(
            sendEcho,
            1500
        );

        setTimeout(
            sendUnknown,
            2000
        );
    }
);
