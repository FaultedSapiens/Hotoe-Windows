"use strict";

const transport =
    window.chrome && window.chrome.webview
        ? window.chrome.webview
        : null;

let nextRequestId = 1;

const pendingRequests =
    new Map();

function normalizeInputRegionSugar()
{
    document
        .querySelectorAll("[SIR]")
        .forEach((node) => {
            node.classList.add(
                "hotoe-input-region-regulator-box"
            );
        });
}

function receive(
    event
)
{
    let message =
        event.data || {};

    if (typeof message === "string")
    {
        try
        {
            message =
                JSON.parse(message);
        }
        catch
        {
            message = {};
        }
    }

    const pending =
        pendingRequests.get(
            message.id
        );

    if (!pending)
        return;

    pendingRequests.delete(
        message.id
    );

    if (message.ok === false)
    {
        pending.reject(
            message.error
        );

        return;
    }

    pending.resolve(
        message.result
    );
}

if (transport)
{
    transport.addEventListener(
        "message",
        receive
    );
}

function invoke(
    method,
    params = {}
)
{
    if (!transport)
    {
        return Promise.reject(
            new Error("Fluent-X native transport is unavailable")
        );
    }

    const id =
        nextRequestId++;

    const request =
        {
            id,
            method,
            params
        };

    const promise =
        new Promise((resolve, reject) => {
            pendingRequests.set(
                id,
                {
                    resolve,
                    reject
                }
            );
        });

    transport.postMessage(request);

    return promise;
}

window.fx =
    window.fx || {};

fx.transport = transport;
fx.invoke = invoke;

fx.ping = function ()
{
    return invoke(
        "fx.ping"
    );
};

fx.version = function ()
{
    return invoke(
        "fx.version"
    );
};

fx.closeApplication = function ()
{
    return invoke(
        "fx.closeApplication"
    );
};

fx.close = fx.closeApplication;

fx.recalculateInputRegions = function ()
{
    normalizeInputRegionSugar();

    return invoke(
        "fx.recalculateInputRegions"
    );
};

fx.pushString = function (
    data
)
{
    let text;

    if (typeof data === "string")
    {
        text = data;
    }
    else
    {
        try
        {
            text =
                JSON.stringify(data);
        }
        catch
        {
            text =
                String(data);
        }
    }

    if (text === undefined)
    {
        text =
            String(data);
    }

    return invoke(
        "fx.pushString",
        {
            data: text
        }
    );
};

window.invoke = invoke;

window.CLOSE = function ()
{
    return fx.closeApplication();
};

window.SIRs = function ()
{
    return fx.recalculateInputRegions();
};

window.push = function (
    data
)
{
    return fx.pushString(data);
};
