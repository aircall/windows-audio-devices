const addon = require('bindings')('audioDevices')

module.exports = {
  getAudioDevices: addon.getAudioDevices
}
