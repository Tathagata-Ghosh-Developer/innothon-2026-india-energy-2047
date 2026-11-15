#!/usr/bin/env node
// Static mock of the NPP endpoints in api/npp.postman_collection.json, serving the recorded
// responses in api/fixtures/ so the collection can run without network access.
// Node.js 18 or later, no dependencies.
//
// Usage: node api/mock/server.js [--port 3647] [--fixtures <dir>]
//
// Routing (the query string is ignored except for the Merit date):
//   GET|HEAD /dashBoard/demandmet{1,2}chartdata?date=D -> fixtures/dashBoard/<name>/<D>.json,
//            or [] when no fixture exists for that day (the live API answers [] for days without data)
//   GET|HEAD /<path>                                   -> fixtures/<path>.json
//   HEAD     /<path>.xls                               -> status and headers from fixtures/<path>.xls.headers.json
//   anything else                                      -> 404
"use strict";
const http = require("http");
const fs = require("fs");
const path = require("path");

const args = process.argv.slice(2);
const opt = (name, fallback) => {
    const i = args.indexOf(name);
    return i >= 0 && i + 1 < args.length ? args[i + 1] : fallback;
};
const port = Number(opt("--port", process.env.NPP_MOCK_PORT || 3647));
const root = path.resolve(opt("--fixtures", path.join(__dirname, "..", "fixtures")));

function fixturePath(rel) {
    const p = path.resolve(root, "." + rel);
    return p.startsWith(root + path.sep) ? p : null;  // stay inside the fixtures folder
}

function send(res, method, status, headers, body) {
    res.writeHead(status, headers);
    res.end(method === "HEAD" ? undefined : body);
}

const server = http.createServer((req, res) => {
    const url = new URL(req.url, "http://localhost");
    const method = req.method;
    const pathname = decodeURIComponent(url.pathname);
    let status = 404;

    if (method !== "GET" && method !== "HEAD") {
        status = 405;
        send(res, method, status, { "Content-Type": "text/plain" }, "method not allowed\n");
    } else if (/\.xls$/i.test(pathname)) {
        const p = fixturePath(pathname + ".headers.json");
        if (method === "HEAD" && p && fs.existsSync(p)) {
            const h = JSON.parse(fs.readFileSync(p, "utf8"));
            status = h.status;
            send(res, method, status, h.headers);
        } else {
            send(res, method, status, { "Content-Type": "text/plain" }, "not recorded (the mock answers HEAD only)\n");
        }
    } else {
        const json = { "Content-Type": "application/json" };
        const merit = /^\/dashBoard\/demandmet[12]chartdata$/.test(pathname);
        const day = url.searchParams.get("date") || "";
        const p = merit ? fixturePath(pathname + "/" + day + ".json") : fixturePath(pathname + ".json");
        if (p && fs.existsSync(p)) {
            status = 200;
            send(res, method, status, json, fs.readFileSync(p));
        } else if (merit) {
            status = 200;
            send(res, method, status, json, "[]");
        } else {
            send(res, method, status, { "Content-Type": "text/plain" }, "no fixture for " + pathname + "\n");
        }
    }
    console.log(`${method} ${req.url} -> ${status}`);
});

server.listen(port, "127.0.0.1", () => {
    console.log(`NPP mock on http://127.0.0.1:${port} serving ${root}`);
});
