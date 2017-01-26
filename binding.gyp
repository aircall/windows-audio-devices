{
  "targets": [{
    "target_name": "audiodevices",
    "sources": ["lib/audioDevices.cc"],
    "include_dirs": [
      "<!(node -e \"require('nan')\")"
    ]
  }]
}
