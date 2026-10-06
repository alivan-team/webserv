#!/usr/bin/env node

let body = "";

process.stdin.setEncoding("utf8");

process.stdin.on("data", (chunk) => {
    body += chunk;
});

process.stdin.on("end", () => {
    console.log("Content-Type: text/plain");
    console.log("");

    console.log("JavaScript CGI works!");
    console.log(`REQUEST_METHOD=${process.env.REQUEST_METHOD || ""}`);
    console.log(`QUERY_STRING=${process.env.QUERY_STRING || ""}`);
    console.log(`CONTENT_LENGTH=${process.env.CONTENT_LENGTH || ""}`);
    console.log(`CONTENT_TYPE=${process.env.CONTENT_TYPE || ""}`);
    console.log(`SCRIPT_NAME=${process.env.SCRIPT_NAME || ""}`);
    console.log(`SERVER_NAME=${process.env.SERVER_NAME || ""}`);
    console.log(`SERVER_PORT=${process.env.SERVER_PORT || ""}`);

    if (body.length > 0) {
        console.log("");
        console.log("BODY:");
        process.stdout.write(body);

        if (!body.endsWith("\n")) {
            console.log("");
        }
    }
});

process.stdin.resume();