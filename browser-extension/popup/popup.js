// LusaKey AutoFill — popup script

(function () {
    "use strict";

    const $ = (id) => document.getElementById(id);

    const lockedView = $("locked-view");
    const unlockedView = $("unlocked-view");
    const errorView = $("error-view");
    const statusBadge = $("status-badge");
    const passwordInput = $("password-input");
    const unlockBtn = $("unlock-btn");
    const unlockError = $("unlock-error");
    const lockBtn = $("lock-btn");
    const searchInput = $("search-input");
    const entryList = $("entry-list");
    const allEntryList = $("all-entry-list");
    const noMatches = $("no-matches");
    const noEntries = $("no-entries");
    const allEntriesSection = $("all-entries");
    const matchInfo = $("match-info");

    let currentTabUrl = "";
    let allCredentials = [];

    function getCurrentTabUrl() {
        return new Promise((resolve) => {
            chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
                resolve(tabs[0] ? tabs[0].url : "");
            });
        });
    }

    function sendRequest(action, params = {}) {
        return new Promise((resolve, reject) => {
            chrome.runtime.sendMessage(
                { type: "lusakey-request", action, params },
                (response) => {
                    if (chrome.runtime.lastError) {
                        reject(new Error(chrome.runtime.lastError.message));
                    } else {
                        resolve(response);
                    }
                }
            );
        });
    }

    async function checkConnection() {
        try {
            const resp = await sendRequest("ping");
            return resp && resp.ok;
        } catch {
            return false;
        }
    }

    async function unlock() {
        const pw = passwordInput.value;
        if (!pw) return;
        unlockBtn.disabled = true;
        unlockError.classList.add("hidden");
        try {
            const resp = await sendRequest("unlock", { password: pw });
            if (resp && resp.ok) {
                passwordInput.value = "";
                await showUnlocked();
            } else {
                unlockError.textContent = resp?.error || "Ошибка разблокировки";
                unlockError.classList.remove("hidden");
            }
        } catch (e) {
            unlockError.textContent = "Не удалось подключиться";
            unlockError.classList.remove("hidden");
        }
        unlockBtn.disabled = false;
    }

    async function lock() {
        await sendRequest("lock");
        showLocked();
    }

    function showLocked() {
        lockedView.classList.remove("hidden");
        unlockedView.classList.add("hidden");
        errorView.classList.add("hidden");
        statusBadge.textContent = "закрыт";
        statusBadge.className = "badge badge-locked";
    }

    function showUnlocked() {
        lockedView.classList.add("hidden");
        unlockedView.classList.remove("hidden");
        errorView.classList.add("hidden");
        statusBadge.textContent = "открыт";
        statusBadge.className = "badge badge-unlocked";
        loadEntries();
    }

    function showError() {
        lockedView.classList.add("hidden");
        unlockedView.classList.add("hidden");
        errorView.classList.remove("hidden");
        statusBadge.textContent = "ошибка";
        statusBadge.className = "badge badge-locked";
    }

    function renderEntry(cred, container) {
        const item = document.createElement("div");
        item.className = "entry-item";
        item.addEventListener("click", () => fillCredentials(cred));

        const title = document.createElement("div");
        title.className = "entry-title";
        title.textContent = cred.title || cred.username || "(без названия)";
        item.appendChild(title);

        if (cred.username) {
            const user = document.createElement("div");
            user.className = "entry-username";
            user.textContent = cred.username;
            item.appendChild(user);
        }

        if (cred.url) {
            const urlEl = document.createElement("div");
            urlEl.className = "entry-url";
            try {
                urlEl.textContent = new URL(cred.url).hostname;
            } catch {
                urlEl.textContent = cred.url;
            }
            item.appendChild(urlEl);
        }

        container.appendChild(item);
    }

    function fillCredentials(cred) {
        chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
            if (tabs[0]) {
                chrome.tabs.sendMessage(tabs[0].id, {
                    type: "lusakey-fill",
                    username: cred.username || "",
                    password: cred.password || ""
                });
            }
        });
        window.close();
    }

    async function loadEntries() {
        entryList.innerHTML = "";
        allEntryList.innerHTML = "";
        noMatches.classList.add("hidden");
        noEntries.classList.add("hidden");
        allEntriesSection.classList.add("hidden");
        matchInfo.classList.add("hidden");

        try {
            const resp = await sendRequest("listEntries");
            if (!resp || !resp.ok || !resp.result) return;
            allCredentials = resp.result;

            if (allCredentials.length === 0) {
                noEntries.classList.remove("hidden");
                return;
            }

            // Get full entries to check URLs
            let matching = [];
            let nonMatching = [];

            for (const summary of allCredentials) {
                const full = await sendRequest("getEntry", { id: summary.id });
                if (full && full.ok && full.result) {
                    const entry = full.result;
                    const entryUrl = entry.url || "";
                    const match = currentTabUrl && entryUrl && (
                        currentTabUrl.includes(entryUrl) || entryUrl.includes(currentTabUrl)
                    );
                    if (match && entry.username && entry.password) {
                        matching.push(entry);
                    } else {
                        nonMatching.push({ ...summary, url: entryUrl });
                    }
                } else {
                    nonMatching.push(summary);
                }
            }

            if (matching.length > 0) {
                matchInfo.classList.remove("hidden");
                matching.forEach((c) => renderEntry(c, entryList));
            } else {
                noMatches.classList.remove("hidden");
            }

            if (nonMatching.length > 0) {
                allEntriesSection.classList.remove("hidden");
                nonMatching.forEach((c) => renderEntry(c, allEntryList));
            }
        } catch {
            // ignore
        }
    }

    async function init() {
        const connected = await checkConnection();
        if (!connected) {
            showError();
            return;
        }

        currentTabUrl = await getCurrentTabUrl();
        // We first try unlock with empty password to check if already unlocked
        // by checking if listEntries works without unlocking
        try {
            const testResp = await sendRequest("listEntries");
            if (testResp && testResp.ok) {
                await showUnlocked();
                return;
            }
        } catch {
            // not unlocked, show locked view
        }
        showLocked();
    }

    unlockBtn.addEventListener("click", unlock);
    passwordInput.addEventListener("keydown", (e) => {
        if (e.key === "Enter") unlock();
    });
    lockBtn.addEventListener("click", lock);

    searchInput.addEventListener("input", () => {
        const q = searchInput.value.toLowerCase();
        document.querySelectorAll(".entry-item").forEach((item) => {
            const text = item.textContent.toLowerCase();
            item.style.display = text.includes(q) ? "" : "none";
        });
    });

    init();
})();