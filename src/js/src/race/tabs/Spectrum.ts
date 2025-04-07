import { SimpleFpvTimer, Mode, Page, RssiEvent, ConfigGameMode, Config } from "../../SimpleFpvTimer.js"
import van from "../../lib/van-1.5.2.js"
import "../../lib/bootstrap.bundle.js"
import { RaceMode } from "../../race/RaceMode.js"
import { Notifications } from "../../Notifications.js"
import { TimeSync } from "../../TimeSync.js"
import { CaptureTheFlagMode } from "../../ctf/CaptureTheFlagMode.js"
import { TestuPlot } from "../../race/tabs/TestuPlot.js"
import { SpectrumMode } from "../../spectrum/SpectrumMode.js"

const {button, div, pre,h3} = van.tags


export class SpectrumPage extends Page {
    root: HTMLElement;
    _cfg: HTMLElement;
    _rssi: HTMLElement;
    _equalizer: HTMLElement;

    get cfg(): HTMLElement {
        if (!this._cfg) {
            this._cfg = div("CONFIG:");
        }
        return this._cfg;
    }

    get rssi(): HTMLElement {
        return this._rssi ? this._rssi : (this._rssi = div("RSSI"));
    }

    get equalizer(): HTMLElement {
        if (!this._equalizer) {
            const frequencyToChannel: { [key: number]: number } = {
                5658: 1,
                5695: 2,
                5732: 3,
                5769: 4,
                5806: 5,
                5843: 6,
                5880: 7,
                5917: 8
            };
    
            this._equalizer = div({
                id: "equalizer",
                style: `
                    display: flex;
                    gap: 10px;
                    height: 200px;
                    background: black;
                    align-items: flex-end;
                    justify-content: center;
                    padding: 10px;
                `
            });
    
            // Create a bar for each frequency
            Object.entries(frequencyToChannel).forEach(([freq, channel]) => {
                const barContainer = div({
                    id: `bar-container-${channel}`, // Unique ID for each bar container
                    style: `
                        display: flex;
                        flex-direction: column;
                        align-items: center;
                        justify-content: flex-end;
                        width: 50px;
                        height: 150px;
                        margin: 0 5px;
                    `
                });
    
                // Create the bar
                const bar = div({
                    id: `bar-${channel}`, // Unique ID for each bar
                    style: `
                        width: 100%;
                        height: 0%; /* Start with 0 height */
                        background: white;
                        display: block;
                    `,
                    title: `Channel: ${channel}, Frequency: ${freq} MHz`
                });
    
                // Create the label for the channel number
                const label = div({
                    style: `
                        margin-top: 5px;
                        color: white;
                        font-size: 12px;
                        text-align: center;
                    `
                }, `${channel}`); // Display the channel number
    
                // Append the bar and label to the container
                barContainer.appendChild(bar);
                barContainer.appendChild(label);
    
                // Append the container to the equalizer
                this._equalizer.appendChild(barContainer);
            });
        }
        return this._equalizer;
    }
    
    getDom(): HTMLElement {
        if (!this.root) {
            this.root = div(
                this.cfg,
                this.rssi,
                this.equalizer // Add the equalizer div here
            );
        }
        return this.root;
    }

    constructor() {
        super("Spectrum");
        this.getDom();

        document.addEventListener("SFT_RSSI", (e: CustomEventInit<RssiEvent>) => {
            const freq = e.detail?.freq; // Frequency from the event
            const s = e.detail?.data[0]?.s; // RSSI value for the frequency
            if (freq !== undefined && s !== undefined) {
                this.renderEqualizer(freq, s); // Update the specific bar
            }
        
            this.rssi.replaceChildren(
                h3("Last RSSI message:" + Date.now()),
                pre(JSON.stringify(e.detail, undefined, 2))
            );
        });

        document.addEventListener("SFT_CONFIG_UPDATE", (e: CustomEventInit<Config>) => {
            this.cfg.replaceChildren(
                h3("Config"),
                pre(e.detail.toJsonString(2))
            );
        });
    }

    private renderEqualizer(freq: number, s: number) {
        const frequencyToChannel: { [key: number]: number } = {
            5658: 1,
            5695: 2,
            5732: 3,
            5769: 4,
            5806: 5,
            5843: 6,
            5880: 7,
            5917: 8
        };
    
        const channel = frequencyToChannel[freq];
        if (!channel) {
            console.warn(`Frequency ${freq} is not mapped to a channel.`);
            return;
        }
    
        // Find the bar for the given channel
        const bar = document.getElementById(`bar-${channel}`);
        if (!bar) {
            console.warn(`Bar for channel ${channel} not found.`);
            return;
        }
    
        // Scale the bar height
        const maxS = 2000; // Adjusted maximum RSSI value for scaling
        const barHeight = Math.min((s / maxS) * 100, 100); // Scale height as a percentage, capped at 100%
        bar.style.height = `${barHeight}%`;
    
        // Update the tooltip
        bar.title = `Channel: ${channel}, Frequency: ${freq} MHz, RSSI: ${s}`;
    }
}
