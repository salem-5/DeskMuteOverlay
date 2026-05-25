/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import definePlugin, { PluginNative } from "@utils/types";

const Native = VencordNative.pluginHelpers.DeskMute as PluginNative<typeof import("./native")>;

let pollInterval: ReturnType<typeof setInterval>;
let syncInterval: ReturnType<typeof setInterval>;
let lastStateStr = "";

let CachedMediaEngineStore: any = null;
let CachedToggleStore: any = null;
let CachedVoiceChannelStore: any = null;
let CachedSpeakingStore: any = null;

function getToggleStore() {
    if (!CachedToggleStore) CachedToggleStore = Vencord.Webpack.find(m => m && typeof m.toggleSelfMute === "function");
    return CachedToggleStore;
}

function getMediaEngineStore() {
    if (!CachedMediaEngineStore) CachedMediaEngineStore = Vencord.Webpack.find(m => m && typeof m.getSelfMute === "function");
    return CachedMediaEngineStore;
}

function getVoiceChannelStore() {
    if (!CachedVoiceChannelStore) CachedVoiceChannelStore = Vencord.Webpack.find(m => m && typeof m.getVoiceChannelId === "function");
    return CachedVoiceChannelStore;
}

function getSpeakingStore() {
    if (!CachedSpeakingStore) {
        CachedSpeakingStore = Vencord.Webpack.find(m => m && typeof m.isSpeaking === "function");
    }
    return CachedSpeakingStore;
}

function triggerAction(action: "mute" | "deafen") {
    const toggleStore = getToggleStore();
    if (action === "mute") toggleStore?.toggleSelfMute();
    else if (action === "deafen") toggleStore?.toggleSelfDeaf();
}

function buildOverlayState() {
    try {
        const { ChannelStore, VoiceStateStore, UserStore, GuildMemberStore } = Vencord.Webpack.Common;
        const MediaEngineStore = getMediaEngineStore();
        const SpeakingStore = getSpeakingStore();
        const selfUserId = UserStore?.getCurrentUser()?.id;

        const state = {
            self: {
                id: selfUserId,
                muted: MediaEngineStore?.getSelfMute() ?? false,
                deafened: MediaEngineStore?.getSelfDeaf() ?? false,
                serverMuted: false,
                serverDeafened: false
            },
            voiceChannel: null as any
        };

        const SelectedVoiceChannelStore = getVoiceChannelStore();
        const channelId = SelectedVoiceChannelStore?.getVoiceChannelId();

        if (channelId && VoiceStateStore && ChannelStore) {
            const channel = ChannelStore.getChannel(channelId);
            const guildId = channel?.guild_id;

            state.voiceChannel = {
                id: channelId,
                name: channel?.name || "Private Call",
                guildId: guildId || null,
                members: [] as any[]
            };

            const voiceStates = VoiceStateStore.getVoiceStatesForChannel(channelId) || {};

            for (const [userId, voiceState] of Object.entries(voiceStates) as [string, any][]) {
                const user = UserStore?.getUser(userId);
                const member = guildId && GuildMemberStore ? GuildMemberStore.getMember(guildId, userId) : null;

                if (user) {
                    const displayName = member?.nick || user.globalName || user.username;
                    const avatarUrl = typeof user.getAvatarURL === "function" ? user.getAvatarURL(guildId, 256, false) : "";

                    if (userId === selfUserId) {
                        state.self.serverMuted = voiceState.mute ?? false;
                        state.self.serverDeafened = voiceState.deaf ?? false;
                    }

                    state.voiceChannel.members.push({
                        id: userId,
                        name: user.username,
                        displayName: displayName,
                        avatarUrl: avatarUrl,
                        isMuted: voiceState.mute || voiceState.selfMute || false,
                        isDeafened: voiceState.deaf || voiceState.selfDeaf || false,
                        isServerMuted: voiceState.mute || false,
                        isServerDeafened: voiceState.deaf || false,
                        isStreaming: voiceState.selfStream || false,
                        isVideoOn: voiceState.selfVideo || false,
                        isSpeaking: SpeakingStore?.isSpeaking(userId) ?? false
                    });
                }
            }
        }
        return state;
    } catch (e) {
        console.error("[DeskMute] Builder Error:", e);
        return { self: { muted: false, deafened: false, serverMuted: false, serverDeafened: false }, voiceChannel: null };
    }
}

export default definePlugin({
    name: "DeskMute",
    description: "TCP Broadcast API for Voice Overlay",
    authors: [{ name: "swzo", id: 0n }],

    start() {
        Native.startServer();

        pollInterval = setInterval(async () => {
            const command = await Native.consumeCommand();
            if (command) triggerAction(command);
        }, 100);

        syncInterval = setInterval(() => {
            const state = buildOverlayState();
            const stateStr = JSON.stringify(state);
            if (stateStr !== lastStateStr) {
                lastStateStr = stateStr;
                Native.updateState(state);
            }
        }, 100);
    },

    stop() {
        clearInterval(pollInterval);
        clearInterval(syncInterval);
        Native.stopServer();
        CachedMediaEngineStore = null;
        CachedToggleStore = null;
        CachedVoiceChannelStore = null;
        CachedSpeakingStore = null;
    }
});
