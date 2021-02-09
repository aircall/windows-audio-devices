declare module 'windows-audio-devices' {
    export function getAudioDevices(): AudioDeviceInfo[];

    export interface AudioDeviceInfo {
        kind: 'audioinput' | 'audiooutput';
        device: string;
        friendly: string;
        default: boolean;
    }
}