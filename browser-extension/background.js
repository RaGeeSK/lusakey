// Native messaging returns responses in order and carries no request ID.
// Keep one request in flight so a page script and popup cannot receive each
// other's response while sharing the same host connection.
const NM_HOST = "com.lusakey.nmhost";
let port = null;
let activeRequest = null;
const requestQueue = [];
let browserLoginPolling = false;

// A browser action popup is closed as soon as focus moves to the desktop app.
// Keep the native port and polling in the service worker so an approved login
// completes even while the popup is not visible.
async function trackAppLogin(requestId) {
    if (browserLoginPolling) return;
    browserLoginPolling = true;
    await chrome.storage.session.set({ pendingAppLoginRequestId: requestId });

    const poll = async () => {
        try {
            const response = await sendRequest("getAppLoginStatus", { requestId });
            if (response?.ok && response?.result?.status === "pending") {
                setTimeout(poll, 700);
                return;
            }
        } catch (_) {
            // The next popup opening can start a fresh confirmation request.
        }
        browserLoginPolling = false;
        await chrome.storage.session.remove("pendingAppLoginRequestId");
    };
    setTimeout(poll, 700);
}

chrome.storage.session.get("pendingAppLoginRequestId").then(({ pendingAppLoginRequestId }) => {
    if (pendingAppLoginRequestId) trackAppLogin(pendingAppLoginRequestId);
});

function connect() {
    if (port) return true;
    try {
        port = chrome.runtime.connectNative(NM_HOST);
        port.onMessage.addListener((message) => {
            if (!activeRequest) return;
            activeRequest.resolve(message);
            activeRequest = null;
            pumpQueue();
        });
        port.onDisconnect.addListener(() => {
            const error = new Error(chrome.runtime.lastError?.message || "native host disconnected");
            if (activeRequest) activeRequest.reject(error);
            activeRequest = null;
            while (requestQueue.length) requestQueue.shift().reject(error);
            port = null;
        });
        return true;
    } catch (error) {
        return false;
    }
}

function pumpQueue() {
    if (activeRequest || !requestQueue.length) return;
    if (!connect()) {
        const request = requestQueue.shift();
        request.reject(new Error("native host not available"));
        return;
    }
    activeRequest = requestQueue.shift();
    try {
        port.postMessage({ action: activeRequest.action, ...activeRequest.params });
    } catch (error) {
        activeRequest.reject(error);
        activeRequest = null;
        pumpQueue();
    }
}

function sendRequest(action, params = {}) {
    return new Promise((resolve, reject) => {
        requestQueue.push({ action, params, resolve, reject });
        pumpQueue();
    });
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.type === "lusakey-request") {
        sendRequest(message.action, message.params)
            .then((response) => {
                if (message.action === "requestAppLogin" && response?.ok && response?.result?.requestId) {
                    trackAppLogin(response.result.requestId);
                }
                sendResponse(response);
            })
            .catch((error) => sendResponse({ ok: false, error: error.message }));
        return true;
    }
    if (message.type === "lusakey-disconnect") {
        if (port) port.disconnect();
        sendResponse({ ok: true });
    }
});
