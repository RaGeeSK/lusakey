// LusaKey AutoFill — content script
// Detects password/username fields on every page, shows inline autofill
// suggestions, and fills credentials on click.

(function () {
    "use strict";

    let credentials = [];
    let autofillBar = null;
    let currentUrl = window.location.href;

    function findFormFields() {
        const passwordFields = document.querySelectorAll('input[type="password"]');
        const usernameFields = document.querySelectorAll(
            'input[type="text"][autocomplete="username"], '
            + 'input[type="email"][autocomplete="username"], '
            + 'input:not([type="password"]):not([type="hidden"])'
        );
        return { passwordFields, usernameFields };
    }

    function getBestFieldPair() {
        const { passwordFields, usernameFields } = findFormFields();
        if (passwordFields.length === 0) return null;
        const primaryPw = passwordFields[0];
        const form = primaryPw.closest("form");
        let primaryUser = null;
        if (form) {
            primaryUser = form.querySelector(
                'input[type="text"], input[type="email"], input:not([type="password"]):not([type="hidden"])'
            );
        }
        return {
            username: primaryUser || usernameFields[0] || null,
            password: primaryPw
        };
    }

    function removeAutofillBar() {
        if (autofillBar) {
            autofillBar.remove();
            autofillBar = null;
        }
    }

    function showAutofillBar(field) {
        removeAutofillBar();
        if (credentials.length === 0) return;

        autofillBar = document.createElement("div");
        autofillBar.className = "lusakey-autofill-bar";
        autofillBar.style.cssText = `
            position: absolute;
            z-index: 2147483647;
            background: #F4F3EE;
            border: 1px solid #B1ADA1;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.15);
            padding: 4px;
            min-width: 200px;
            font-family: Inter, -apple-system, sans-serif;
            font-size: 13px;
        `;

        const title = document.createElement("div");
        title.textContent = "LusaKey — подстановка пароля";
        title.style.cssText = `
            padding: 6px 8px 4px;
            font-size: 11px;
            color: #B1ADA1;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        `;
        autofillBar.appendChild(title);

        credentials.forEach((cred) => {
            const item = document.createElement("div");
            item.style.cssText = `
                padding: 8px;
                cursor: pointer;
                border-radius: 4px;
                display: flex;
                flex-direction: column;
                gap: 2px;
            `;
            item.addEventListener("mouseenter", () => {
                item.style.background = "#E8E6E1";
            });
            item.addEventListener("mouseleave", () => {
                item.style.background = "transparent";
            });
            item.addEventListener("click", (e) => {
                e.stopPropagation();
                fillCredentials(cred);
            });

            const titleSpan = document.createElement("span");
            titleSpan.textContent = cred.title || cred.username || "(без названия)";
            titleSpan.style.cssText = "font-weight: 600; color: #3D3D3D;";
            item.appendChild(titleSpan);

            if (cred.username) {
                const userSpan = document.createElement("span");
                userSpan.textContent = cred.username;
                userSpan.style.cssText = "font-size: 12px; color: #6B6B6B;";
                item.appendChild(userSpan);
            }

            autofillBar.appendChild(item);
        });

        const rect = field.getBoundingClientRect();
        autofillBar.style.left = Math.max(4, Math.min(rect.left, window.innerWidth - 220)) + "px";
        autofillBar.style.top = (rect.bottom + window.scrollY + 4) + "px";

        document.body.appendChild(autofillBar);

        const clickHandler = (e) => {
            if (autofillBar && !autofillBar.contains(e.target)) {
                removeAutofillBar();
                document.removeEventListener("click", clickHandler);
            }
        };
        document.addEventListener("click", clickHandler);
    }

    function fillCredentials(cred) {
        const pair = getBestFieldPair();
        if (!pair) return;
        if (pair.username && cred.username) {
            pair.username.value = cred.username;
            pair.username.dispatchEvent(new Event("input", { bubbles: true }));
            pair.username.dispatchEvent(new Event("change", { bubbles: true }));
        }
        if (pair.password && cred.password) {
            pair.password.value = cred.password;
            pair.password.dispatchEvent(new Event("input", { bubbles: true }));
            pair.password.dispatchEvent(new Event("change", { bubbles: true }));
        }
        removeAutofillBar();
    }

    chrome.runtime.onMessage.addListener((message) => {
        if (message.type === "lusakey-fill") {
            fillCredentials({ username: message.username, password: message.password });
        }
    });

    function requestCredentials(url) {
        chrome.runtime.sendMessage(
            { type: "lusakey-request", action: "getCredentialsForUrl", params: { url } },
            (response) => {
                if (response && response.ok && response.result) {
                    credentials = response.result;
                    const pair = getBestFieldPair();
                    if (pair && credentials.length > 0) {
                        pair.password.focus();
                        showAutofillBar(pair.password);
                    }
                }
            }
        );
    }

    let debounceTimer = null;
    function onPageChange() {
        if (currentUrl !== window.location.href) {
            currentUrl = window.location.href;
            removeAutofillBar();
            credentials = [];
        }
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
            const pair = getBestFieldPair();
            if (pair) {
                requestCredentials(currentUrl);
            }
        }, 500);
    }

    const observer = new MutationObserver(onPageChange);
    observer.observe(document.body || document.documentElement, {
        childList: true,
        subtree: true,
    });

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", onPageChange);
    } else {
        onPageChange();
    }
})();