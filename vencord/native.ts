/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import { IpcMainInvokeEvent } from "electron";
import net from "net";

let server: net.Server | null = null;
let clients: net.Socket[] = [];
let currentState: any = { self: { muted: false, deafened: false }, voiceChannel: null };
let pendingCommand: "mute" | "deafen" | null = null;

export function updateState(_: IpcMainInvokeEvent, state: any) {
    currentState = state;
    const payload = JSON.stringify(currentState) + "\n";
    clients = clients.filter((client) => {
        if (client.destroyed) return false;
        try {
            client.write(payload);
            return true;
        } catch (e) {
            return false;
        }
    });
}

export function consumeCommand(_: IpcMainInvokeEvent) {
    if (pendingCommand) {
        const cmd = pendingCommand;
        pendingCommand = null;
        return cmd;
    }
    return null;
}

export function startServer(_: IpcMainInvokeEvent) {
    if (server) return;

    server = net.createServer((socket) => {
        clients.push(socket);
        socket.write(JSON.stringify(currentState) + "\n");

        socket.on("data", (data) => {
            const command = data.toString().trim();
            if (command === "mute") pendingCommand = "mute";
            else if (command === "deafen") pendingCommand = "deafen";
        });

        socket.on("error", () => {
            clients = clients.filter((c) => c !== socket);
        });
    });

    server.listen(3845, "127.0.0.1", () => {
        console.log("[DeskMute] TCP Server running on 127.0.0.1:3845");
    });
}

export function stopServer(_: IpcMainInvokeEvent) {
    if (server) {
        clients.forEach((c) => c.destroy());
        clients = [];
        server.close();
        server = null;
    }
}
