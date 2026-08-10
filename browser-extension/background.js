// LusaKey AutoFill — background service worker
// Maintains a persistent connection to the native-messaging host (lusakey-nmhost)
// and relays requests from content scripts and popup.

const NM_HOST = "com.lusakey.nmhost";
let port = null;
let pendingRequests = new Map();
let nextRequestId = 1;

function connect() {
    if (port) return;
    try {
        port = chrome.runtime.connectNative(NM_HOST);
        port.onMessage.addListener((msg) => {
            for (const [id, {resolve}] of pendingRequests) {
                resolve(msg);
                pendingRequests.delete(id);
            }
        });
        port.onDisconnect.addListener(() => {
            port = null;
            for (const [, {reject}] of pendingRequests) {
                reject(new Error("native host disconnected"));
            }
            pendingRequests.clear();
        });
    } catch (e) {
        console.warn("LusaKey: native messaging connect failed", e);
    }
}

function sendRequest(action, params = {}) {
    return new Promise((resolve, reject) => {
        connect();
        if (!port) {
            reject(new Error("native host not available"));
            return;
        }
        const id = nextRequestId++;
        const request = { action, ...params };
        pendingRequests.set(id, { resolve, reject });
        try {
            port.postMessage(request);
        } catch (e) {
            pendingRequests.delete(id);
            reject(e);
        }
    });
}

function disconnect() {
    if (port) {
        port.disconnect();
        port = null;
    }
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.type === "lusakey-request") {
        sendRequest(message.action, message.params)
            .then(sendResponse)
            .catch((err) => sendResponse({ ok: false, error: err.message }));
        return true;
    }
    if (message.type === "lusakey-disconnect") {
        disconnect();
        sendResponse({ ok: true });
    }
});