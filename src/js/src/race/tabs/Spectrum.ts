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
    _ctf: HTMLElement;

    get cfg(): HTMLElement {
        if (!this._cfg) {
            this._cfg = div("CONFIG:");
        }
        return this._cfg;
    }

    get rssi(): HTMLElement {
        return this._rssi ? this._rssi : (this._rssi = div("RSSI"));
    }
    get ctf(): HTMLElement {
        return this._ctf ? this._ctf : (this._ctf = div("ctf"));
    }

    getDom(): HTMLElement {
        if (!this.root) {
            this.root = div(
                this.cfg,
                this.rssi,
            );
        }
        return this.root;
    }

    constructor() {
        super("Spectrum");
        this.getDom();

        document.addEventListener("SFT_RSSI", (e: CustomEventInit<RssiEvent>) => {
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
}
