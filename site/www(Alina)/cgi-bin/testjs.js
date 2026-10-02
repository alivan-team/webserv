#!/usr/bin/env node

const method = process.env.REQUEST_METHOD || "";
const query = process.env.QUERY_STRING || "";

let body = "";

process.stdin.setEncoding("utf8");

process.stdin.on("data", chunk => {
    body += chunk;
});

process.stdin.on("end", () => {
    console.log("Content-Type: text/plain\r");
    console.log("\r");

    console.log("JS CGI EXECUTED");
    console.log("method=" + method);
    console.log("query=" + query);
    console.log("body=" + body);
});