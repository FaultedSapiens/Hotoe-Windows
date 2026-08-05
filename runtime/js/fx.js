  "use strict";

const logBox =
    document.getElementById("log");

const reqCount =
    document.getElementById("reqCount");

const resCount =
    document.getElementById("resCount");

const pendingCount =
    document.getElementById("pendingCount");

const errCount =
    document.getElementById("errCount");

let requests = 0;
let responses = 0;
let pending = 0;
let errors = 0;

function updateStats(){

    reqCount.textContent =
        requests;

    resCount.textContent =
        responses;

    pendingCount.textContent =
        pending;

    errCount.textContent =
        errors;

}

const rpcBody =
    document.getElementById("rpcBody");

function addRow(

    direction,

    message

){

    const row =
        document.createElement("tr");

    row.innerHTML =

        `
        <td>${message.id ?? "-"}</td>
        <td class="${direction==="TX"?"tx":"rx"}">
            ${direction}
        </td>
        <td>${message.method ?? "-"}</td>
        <td class="${message.ok===false?"err":""}">
            ${
                message.ok===false
                ? "ERROR"
                : "OK"
            }
        </td>
        `;

    rpcBody.appendChild(
        row
    );

    rpcBody.parentElement.scrollTop =
        rpcBody.parentElement.scrollHeight;

}

async function animatePipeline(){

    const browser =
        document.getElementById("browserNode");

    const native =
        document.getElementById("nativeNode");

    const dispatcher =
        document.getElementById("dispatcherNode");

    const packet1 =
        document.getElementById("packet1");

    const packet2 =
        document.getElementById("packet2");

    browser.classList.add("active");

    packet1.animate(

        [

            {

                transform:
                    "translateX(0)"

            },

            {

                transform:
                    "translateX(calc(100vw/5 - 40px))"

            }

        ],

        {

            duration:180,

            easing:"linear"

        }

    );

    await new Promise(

        r=>setTimeout(r,180)

    );

    browser.classList.remove(
        "active"
    );

    native.classList.add(
        "active"
    );

    packet2.animate(

        [

            {

                transform:
                    "translateX(0)"

            },

            {

                transform:
                    "translateX(calc(100vw/5 - 40px))"

            }

        ],

        {

            duration:180,

            easing:"linear"

        }

    );

    await new Promise(

        r=>setTimeout(r,180)

    );

    native.classList.remove(
        "active"
    );

    dispatcher.classList.add(
        "active"
    );

    await new Promise(

        r=>setTimeout(r,120)

    );

    dispatcher.classList.remove(
        "active"

    );

}

let nextId = 1;
function send(message){

    requests++;

    pending++;

    updateStats();

    message.id =
        nextId++;

    addRow(
        "TX",
        message
    );

    chrome.webview.postMessage(message);

}
function invoke(
    method,
    params = {}
)
{
    send({

        method,

        params

    });
}

chrome.webview.addEventListener(

    "message",

    event=>{

        responses++;

        pending--;

        updateStats();

        addRow(

            "RX",

            event.data

        );

        if(

            event.data &&
            event.data.ok === false

        ){

            errors++;

            updateStats();

        }

    }

);

function sendPing()
{
    fx.ping();
}

function sendEcho(){

    send({

    id:2,

    method:"echo",

    params:{

        text:"FallenBrainLiquidEyes"

    }

});

}

function sendUnknown(){

    send({

    id:3,

    method:"unknown",

    params:{}

});

}

window.addEventListener(

    "DOMContentLoaded",

    ()=>{

        updateStats();

        sendPing();

        setTimeout(

            sendPing,

            500

        );

        setTimeout(

            sendPing,

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

window.fx = {};

fx.ping = function ()
{
    invoke(
        "fx.ping"
    );
};

fx.version = function ()
{
    send({

        method: "fx.version",

        params: {}

    });
};

fx.close = function ()
{
    send({

        method: "fx.close",

        params: {}

    });
};