"use strict";

window.addEventListener(
    "busMessage",
    function (event) {
        document.getElementById("message").textContent =
            event.detail;
    }
);

window.addEventListener(
    "DOMContentLoaded",
    function () {
        SIRs()
            .catch(() => {});
    }
);
