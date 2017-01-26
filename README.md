## windows-audio-devices
On Windows 7 and superior, get a native alternative to `navigator.mediaDevices.enumerateDevices()` to get audio capture and render devices.
```
const { getAudioDevices } = require('windows-audio-devices')
const audioDevicesArray = getAudioDevices()
```
Returns an array of objects structured as below:
```
{
  "kind": "'audioinput' or 'audioutput'"
  "device": "Complete device name",
  "friendly": "Shortened friendly name for display",
  "default": "Boolean representing default system choice"
}
```

#### License
MIT, please see LICENSE for details. Copyright (c) [Aircall.io](http://www.aircall.io)
