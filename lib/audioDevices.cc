#include <node.h>
#include <v8.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <atlstr.h>
#endif

// Lange: Copied from https://chromium.googlesource.com/external/webrtc/+/branch-heads/43/talk/media/devices/win32devicemanager.cc#40
// There's some conflict I don't understand,
// the hack is to just define this one symbol manually.
// See: https://code.google.com/p/webrtc/issues/detail?id=3996
EXTERN_C const PROPERTYKEY PKEY_AudioEndpoint_GUID = { {
  0x1da5d803, 0xd492, 0x4edd, {
    0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e
  } }, 4
};

using namespace v8;

#ifdef _WIN32
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

#define EXIT_ON_ERROR(hresult) \
  if (FAILED(hresult)) { goto _handleError; }
#endif


#ifdef _WIN32
boolean ExtractDataFromCollection(
  Isolate *isolate,
  IMMDeviceCollection *collection, UINT dataLength, char* kind,
  IMMDevice *pDefaultDevice,
  Local<Array> &results, UINT &accessor
) {
  HRESULT hr;
  IMMDevice *pEndpoint = NULL;
  IPropertyStore *pProps = NULL;

  LPWSTR deviceId = NULL;
  // Extract id from default device to set a boolean further on
  hr = pDefaultDevice->GetId(&deviceId);
  if (FAILED(hr))
    return false;

  // Cross collection and serialize each item into V8::Object pushed to results
  for (UINT i = 0 ; i < dataLength ; i++) {
    Local<Object> obj = Object::New(isolate);
    LPWSTR itemId = NULL;

    hr = collection->Item(i, &pEndpoint);
      if (FAILED(hr))
        return false;
    hr = pEndpoint->GetId(&itemId);
    obj->Set(String::NewFromUtf8(isolate, "default"), Boolean::New(isolate, wcscmp(itemId, deviceId) == 0));
    obj->Set(String::NewFromUtf8(isolate, "kind"), String::NewFromUtf8(isolate, kind));

    hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
    if (FAILED(hr))
      return false;

    PROPVARIANT deviceGuid;
    PropVariantInit(&deviceGuid);
    hr = pProps->GetValue(PKEY_AudioEndpoint_GUID, &deviceGuid);
    if (FAILED(hr))
      return false;
    obj->Set(String::NewFromUtf8(isolate, "guid"), String::NewFromUtf8(isolate, CW2A(deviceGuid.pwszVal)));

    PROPVARIANT deviceName;
    PropVariantInit(&deviceName);
    hr = pProps->GetValue(PKEY_Device_FriendlyName, &deviceName);
    if (FAILED(hr))
      return false;
    obj->Set(String::NewFromUtf8(isolate, "device"), String::NewFromUtf8(isolate, CW2A(deviceName.pwszVal)));

    PROPVARIANT friendlyName;
    PropVariantInit(&friendlyName);
    hr = pProps->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyName);
    if (FAILED(hr))
      return false;
    obj->Set(String::NewFromUtf8(isolate, "friendly"), String::NewFromUtf8(isolate, CW2A(friendlyName.pwszVal)));

    results->Set(accessor, obj);
    accessor++;
  }
  return true;

}
#endif

void Method(const v8::FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();
  HandleScope scope(isolate);

  #ifdef _WIN32
  HRESULT hr;
  IMMDeviceEnumerator *pEnumerator = NULL;
  IMMDeviceCollection *pCaptureCollection = NULL;
  IMMDeviceCollection *pRenderCollection = NULL;

  CoInitialize(nullptr);
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL,
                        CLSCTX_ALL, IID_IMMDeviceEnumerator,
                        (void**)&pEnumerator
                        );
  EXIT_ON_ERROR(hr)

  HRESULT hrCapture;
  HRESULT hrRender;
  UINT countRender = 0;
  UINT countCapture = 0;

  // Count output devices
  hrCapture = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCaptureCollection);
  if (!FAILED(hrCapture)) {
    hr = pCaptureCollection->GetCount(&countCapture);
    if (FAILED(hr))
    countCapture = 0;
  }

  // Count input devices
  hrRender = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pRenderCollection);
  if (!FAILED(hrRender)) {
    hr = pRenderCollection->GetCount(&countRender);
    if (FAILED(hr))
    countRender = 0;
  }

  UINT totalCount = countCapture + countRender;
  // If there's no element, jump to error handling to return empty array
  if (totalCount == 0)
    goto _handleError;

  UINT accessor = 0;
  Local<Array> resultsArray = Array::New(isolate, totalCount);

  // Microphones
  IMMDevice *pDefaultCaptureDevice = NULL;
  hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDefaultCaptureDevice);
  if (!FAILED(hr) && !FAILED(hrCapture))
    if (ExtractDataFromCollection(isolate,
                                  pCaptureCollection, countCapture, "audioinput",
                                  pDefaultCaptureDevice,
                                  resultsArray, accessor
                                  ) == false)
      goto _handleError;

  // Headsets
  IMMDevice *pDefaultRenderDevice = NULL;
  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultRenderDevice);
  if (!FAILED(hr) && !FAILED(hrRender))
    if (ExtractDataFromCollection(isolate,
                                  pRenderCollection, countRender, "audiooutput",
                                  pDefaultRenderDevice,
                                  resultsArray, accessor
                                  ) == false)
      goto _handleError;


  args.GetReturnValue().Set(resultsArray);
  return;

  #else
  goto _handleError;

  #endif


// Error subfunction called from EXIT_ON_ERROR macro
_handleError:
  args.GetReturnValue().Set(Array::New(isolate)); // Return empty array on error
  return;
}

void init(Local<Object> exports) {
  Isolate *isolate = Isolate::GetCurrent();
  Local<Context> context = isolate->GetCurrentContext();

  exports->Set(String::NewFromUtf8(isolate, "getAudioDevices"),
               FunctionTemplate::New(isolate, Method)->GetFunction(context).ToLocalChecked());
}

NODE_MODULE(test, init)
