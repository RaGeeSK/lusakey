// Browser popup. Authorization is approved in the desktop app; the master
// password is never accepted, stored, or transmitted by this extension.
(function () {
    "use strict";

    const $ = (id) => document.getElementById(id);
    const lockedView = $("locked-view");
    const unlockedView = $("unlocked-view");
    const errorView = $("error-view");
    const statusBadge = $("status-badge");
    const appLoginBtn = $("app-login-btn");
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

    function sendRequest(action, params = {}) {
        return new Promise((resolve, reject) => chrome.runtime.sendMessage(
            { type: "lusakey-request", action, params },
            (response) => chrome.runtime.lastError ? reject(new Error(chrome.runtime.lastError.message)) : resolve(response)
        ));
    }

    function showLocked(message = "") {
        lockedView.classList.remove("hidden"); unlockedView.classList.add("hidden"); errorView.classList.add("hidden");
        statusBadge.textContent = "закрыт"; statusBadge.className = "badge badge-locked";
        unlockError.textContent = message; unlockError.classList.toggle("hidden", !message);
    }
    function showUnlocked() {
        lockedView.classList.add("hidden"); unlockedView.classList.remove("hidden"); errorView.classList.add("hidden");
        statusBadge.textContent = "открыт"; statusBadge.className = "badge badge-unlocked"; loadEntries();
    }
    function showError() {
        lockedView.classList.add("hidden"); unlockedView.classList.add("hidden"); errorView.classList.remove("hidden");
        statusBadge.textContent = "ошибка"; statusBadge.className = "badge badge-locked";
    }
    function getCurrentTabUrl() {
        return new Promise((resolve) => chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => resolve(tabs[0]?.url || "")));
    }
    function fillEntry(cred) {
        chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
            if (tabs[0]) chrome.tabs.sendMessage(tabs[0].id, { type: "lusakey-fill", username: cred.username || "", password: cred.password || "" });
        });
        window.close();
    }
    function renderEntry(cred, container) {
        const item = document.createElement("div"); item.className = "entry-item"; item.addEventListener("click", () => fillEntry(cred));
        const title = document.createElement("div"); title.className = "entry-title"; title.textContent = cred.title || cred.username || "(без названия)"; item.appendChild(title);
        if (cred.username) { const username = document.createElement("div"); username.className = "entry-username"; username.textContent = cred.username; item.appendChild(username); }
        container.appendChild(item);
    }
    async function loadEntries() {
        entryList.innerHTML = ""; allEntryList.innerHTML = ""; noMatches.classList.add("hidden"); noEntries.classList.add("hidden"); allEntriesSection.classList.add("hidden"); matchInfo.classList.add("hidden");
        const list = await sendRequest("listEntries");
        if (!list?.ok) return;
        if (list.result.length === 0) { noEntries.classList.remove("hidden"); return; }
        const matching = []; const other = [];
        for (const summary of list.result) {
            const full = await sendRequest("getEntry", { id: summary.id }); const entry = full?.ok ? full.result : summary;
            const matches = entry.url && currentTabUrl && (currentTabUrl.includes(entry.url) || entry.url.includes(currentTabUrl));
            (matches ? matching : other).push(entry);
        }
        if (matching.length) { matchInfo.classList.remove("hidden"); matching.forEach((entry) => renderEntry(entry, entryList)); } else noMatches.classList.remove("hidden");
        if (other.length) { allEntriesSection.classList.remove("hidden"); other.forEach((entry) => renderEntry(entry, allEntryList)); }
    }
    async function beginAppLogin() {
        appLoginBtn.disabled = true;
        try {
            const response = await sendRequest("requestAppLogin");
            if (!response?.ok) throw new Error(response?.error || "Не удалось открыть приложение");
            showLocked("Подтвердите вход в открывшемся приложении…"); pollAppLogin(response.result.requestId);
        } catch (error) { showLocked(error.message); appLoginBtn.disabled = false; }
    }
    async function pollAppLogin(requestId) {
        try {
            const response = await sendRequest("getAppLoginStatus", { requestId }); const status = response?.result?.status;
            if (response?.ok && status === "approved") { showUnlocked(); return; }
            if (response?.ok && status === "pending") { setTimeout(() => pollAppLogin(requestId), 700); return; }
            showLocked(response?.result?.error || "Вход не подтверждён");
        } catch (error) { showLocked(error.message); }
        appLoginBtn.disabled = false;
    }
    async function init() {
        currentTabUrl = await getCurrentTabUrl();
        const { pendingAppLoginRequestId } = await chrome.storage.session.get("pendingAppLoginRequestId");
        if (pendingAppLoginRequestId) {
            appLoginBtn.disabled = true;
            showLocked("Подтвердите вход в открывшемся приложении…");
            pollAppLogin(pendingAppLoginRequestId);
            return;
        }
        try { const response = await sendRequest("listEntries"); response?.ok ? showUnlocked() : showLocked(); } catch { showError(); }
    }
    appLoginBtn.addEventListener("click", beginAppLogin);
    lockBtn.addEventListener("click", async () => { await sendRequest("lock"); showLocked(); });
    searchInput.addEventListener("input", () => { const q = searchInput.value.toLowerCase(); document.querySelectorAll(".entry-item").forEach((item) => item.style.display = item.textContent.toLowerCase().includes(q) ? "" : "none"); });
    init();
})();
