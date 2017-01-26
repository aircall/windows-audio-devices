const addon = require('bindings')('audioDevices')

module.exports = {
  getAudioEndpoint: addon.getAudioEndpoint
}
