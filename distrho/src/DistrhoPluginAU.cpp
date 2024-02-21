/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// TODO
// - g_nextBundlePath vs d_nextBundlePath cleanup
// - scale points to kAudioUnitParameterFlag_ValuesHaveStrings

#include "DistrhoPluginInternal.hpp"
#include "../DistrhoPluginUtils.hpp"

#include <AudioUnit/AudioUnit.h>

#define TRACE d_stderr("////////--------------------------------------------------------------- %s %d", __PRETTY_FUNCTION__, __LINE__);

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

static const char* AudioUnitPropertyID2Str(const AudioUnitPropertyID prop) noexcept
{
    switch (prop)
    {
    #define PROP(s) case s: return #s;
    PROP(kAudioUnitProperty_ClassInfo)
    PROP(kAudioUnitProperty_MakeConnection)
    PROP(kAudioUnitProperty_SampleRate)
    PROP(kAudioUnitProperty_ParameterList)
    PROP(kAudioUnitProperty_ParameterInfo)
   #if !TARGET_OS_IPHONE
    PROP(kAudioUnitProperty_FastDispatch)
   #endif
    PROP(kAudioUnitProperty_CPULoad)
    PROP(kAudioUnitProperty_StreamFormat)
    PROP(kAudioUnitProperty_ElementCount)
    PROP(kAudioUnitProperty_Latency)
    PROP(kAudioUnitProperty_SupportedNumChannels)
    PROP(kAudioUnitProperty_MaximumFramesPerSlice)
    PROP(kAudioUnitProperty_ParameterValueStrings)
    PROP(kAudioUnitProperty_AudioChannelLayout)
    PROP(kAudioUnitProperty_TailTime)
    PROP(kAudioUnitProperty_BypassEffect)
    PROP(kAudioUnitProperty_LastRenderError)
    PROP(kAudioUnitProperty_SetRenderCallback)
    PROP(kAudioUnitProperty_FactoryPresets)
    PROP(kAudioUnitProperty_RenderQuality)
    PROP(kAudioUnitProperty_HostCallbacks)
    PROP(kAudioUnitProperty_InPlaceProcessing)
    PROP(kAudioUnitProperty_ElementName)
    PROP(kAudioUnitProperty_SupportedChannelLayoutTags)
    PROP(kAudioUnitProperty_PresentPreset)
    PROP(kAudioUnitProperty_DependentParameters)
    PROP(kAudioUnitProperty_InputSamplesInOutput)
    PROP(kAudioUnitProperty_ShouldAllocateBuffer)
    PROP(kAudioUnitProperty_FrequencyResponse)
    PROP(kAudioUnitProperty_ParameterHistoryInfo)
    PROP(kAudioUnitProperty_NickName)
    PROP(kAudioUnitProperty_OfflineRender)
    PROP(kAudioUnitProperty_ParameterIDName)
    PROP(kAudioUnitProperty_ParameterStringFromValue)
    PROP(kAudioUnitProperty_ParameterClumpName)
    PROP(kAudioUnitProperty_ParameterValueFromString)
    PROP(kAudioUnitProperty_PresentationLatency)
    PROP(kAudioUnitProperty_ClassInfoFromDocument)
    PROP(kAudioUnitProperty_RequestViewController)
    PROP(kAudioUnitProperty_ParametersForOverview)
    PROP(kAudioUnitProperty_SupportsMPE)
    PROP(kAudioUnitProperty_RenderContextObserver)
    PROP(kAudioUnitProperty_LastRenderSampleTime)
    PROP(kAudioUnitProperty_LoadedOutOfProcess)
   #if !TARGET_OS_IPHONE
    PROP(kAudioUnitProperty_SetExternalBuffer)
    PROP(kAudioUnitProperty_GetUIComponentList)
    PROP(kAudioUnitProperty_CocoaUI)
    PROP(kAudioUnitProperty_IconLocation)
    PROP(kAudioUnitProperty_AUHostIdentifier)
   #endif
    PROP(kAudioUnitProperty_MIDIOutputCallbackInfo)
    PROP(kAudioUnitProperty_MIDIOutputCallback)
    PROP(kAudioUnitProperty_MIDIOutputEventListCallback)
    PROP(kAudioUnitProperty_AudioUnitMIDIProtocol)
    PROP(kAudioUnitProperty_HostMIDIProtocol)
    PROP(kAudioUnitProperty_MIDIOutputBufferSizeHint)
    #undef PROP
    }
    return "[unknown]";
}

static const char* AudioUnitScope2Str(const AudioUnitScope scope) noexcept
{
    switch (scope)
    {
    #define SCOPE(s) case s: return #s;
    SCOPE(kAudioUnitScope_Global)
    SCOPE(kAudioUnitScope_Input)
    SCOPE(kAudioUnitScope_Output)
    SCOPE(kAudioUnitScope_Group)
    SCOPE(kAudioUnitScope_Part)
    SCOPE(kAudioUnitScope_Note)
    SCOPE(kAudioUnitScope_Layer)
    SCOPE(kAudioUnitScope_LayerItem)
    #undef SCOPE
    }
    return "[unknown]";
}

static const char* AudioUnitSelector2Str(const SInt16 selector) noexcept
{
    switch (selector)
    {
    #define SEL(s) case s: return #s;
    SEL(kAudioUnitInitializeSelect)
    SEL(kAudioUnitUninitializeSelect)
    SEL(kAudioUnitGetPropertyInfoSelect)
    SEL(kAudioUnitGetPropertySelect)
    SEL(kAudioUnitSetPropertySelect)
    SEL(kAudioUnitAddPropertyListenerSelect)
    SEL(kAudioUnitRemovePropertyListenerSelect)
    SEL(kAudioUnitRemovePropertyListenerWithUserDataSelect)
    SEL(kAudioUnitAddRenderNotifySelect)
    SEL(kAudioUnitRemoveRenderNotifySelect)
    SEL(kAudioUnitGetParameterSelect)
    SEL(kAudioUnitSetParameterSelect)
    SEL(kAudioUnitScheduleParametersSelect)
    SEL(kAudioUnitRenderSelect)
    SEL(kAudioUnitResetSelect)
    SEL(kAudioUnitComplexRenderSelect)
    SEL(kAudioUnitProcessSelect)
    SEL(kAudioUnitProcessMultipleSelect)
    SEL(kMusicDeviceMIDIEventSelect)
    SEL(kMusicDeviceSysExSelect)
    SEL(kMusicDevicePrepareInstrumentSelect)
    SEL(kMusicDeviceReleaseInstrumentSelect)
    SEL(kMusicDeviceStartNoteSelect)
    SEL(kMusicDeviceStopNoteSelect)
    SEL(kMusicDeviceMIDIEventListSelect)
    SEL(kAudioOutputUnitStartSelect)
    SEL(kAudioOutputUnitStopSelect)
    #undef SEL
    }
    return "[unknown]";
}

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static constexpr const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static constexpr const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const updateStateValueFunc updateStateValueCallback = nullptr;
#endif

class PluginAU
{
public:
    PluginAU(const AudioUnit component)
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback, updateStateValueCallback),
          fParameterCount(fPlugin.getParameterCount()),
          fCachedParameterValues(nullptr)
    {
        TRACE

	    if (fParameterCount != 0)
        {
            fCachedParameterValues = new float[fParameterCount];
            std::memset(fCachedParameterValues, 0, sizeof(float) * fParameterCount);

            for (uint32_t i=0; i<fParameterCount; ++i)
                fCachedParameterValues[i] = fPlugin.getParameterValue(i);
        }
    }

    ~PluginAU()
    {
        TRACE
        delete[] fCachedParameterValues;
    }

    OSStatus auInitialize()
    {
        return noErr;
    }

    OSStatus auUninitialize()
    {
        return noErr;
    }

    OSStatus auGetPropertyInfo(const AudioUnitPropertyID prop,
                               const AudioUnitScope inScope,
                               const AudioUnitElement inElement,
                               UInt32& outDataSize,
                               Boolean& outWritable)
    {
        switch (prop)
        {
        case kAudioUnitProperty_ClassInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            /* TODO used for storing plugin state
            outDataSize = sizeof(CFPropertyListRef);
            outWritable = true;
            */
            break;
        case kAudioUnitProperty_MakeConnection:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global || inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(AudioUnitConnection);
            outWritable = true;
            return noErr;
        case kAudioUnitProperty_SampleRate:
            break;
        case kAudioUnitProperty_ParameterList:
            outDataSize = inScope == kAudioUnitScope_Global ? sizeof(AudioUnitParameterID) * fParameterCount : 0;
            outWritable = false;
            return noErr;
        case kAudioUnitProperty_ParameterInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(AudioUnitParameterInfo);
            outWritable = false;
            return noErr;
        case kAudioUnitProperty_StreamFormat:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(AudioStreamBasicDescription);
            outWritable = true;
            return noErr;
        case kAudioUnitProperty_ElementCount:
            outDataSize = sizeof(UInt32);
            outWritable = false;
            return noErr;
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kAudioUnitProperty_Latency:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(Float64);
            outWritable = false;
            return noErr;
       #endif
        case kAudioUnitProperty_SupportedNumChannels:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(AUChannelInfo);
            outWritable = false;
            return noErr;
        case kAudioUnitProperty_MaximumFramesPerSlice:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(UInt32);
            outWritable = true;
            return noErr;
        case kAudioUnitProperty_SetRenderCallback:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(AURenderCallbackStruct);
            outWritable = true;
            return noErr;
       #if DISTRHO_PLUGIN_HAS_UI
        case kAudioUnitProperty_CocoaUI:
            outDataSize = sizeof(AudioUnitCocoaViewInfo);
            outWritable = false;
            return noErr;
       #endif
        }

        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auGetProperty(const AudioUnitPropertyID prop,
                           const AudioUnitScope inScope,
                           const AudioUnitElement inElement,
                           void* const outData)
    {
        switch (prop)
        {
        case kAudioUnitProperty_ClassInfo:
            /* TODO used for storing plugin state
            *static_cast<CFPropertyListRef*>(outData) = nullptr;
            */
            break;
        case kAudioUnitProperty_SampleRate:
            break;
        case kAudioUnitProperty_ParameterList:
            {
                AudioUnitParameterID* const paramList = static_cast<AudioUnitParameterID*>(outData);

                for (uint32_t i=0; i<fParameterCount; ++i)
                    paramList[i] = i;
            }
            return noErr;
        case kAudioUnitProperty_ParameterInfo:
	        DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            {
                AudioUnitParameterInfo* const info = static_cast<AudioUnitParameterInfo*>(outData);
                std::memset(info, 0, sizeof(*info));

                const ParameterRanges& ranges(fPlugin.getParameterRanges(inElement));

                info->flags = kAudioUnitParameterFlag_IsHighResolution
                                    | kAudioUnitParameterFlag_IsReadable
                                    | kAudioUnitParameterFlag_HasCFNameString;

                if (fPlugin.getParameterDesignation(inElement) == kParameterDesignationBypass)
                {
                    info->flags |= kAudioUnitParameterFlag_IsWritable|kAudioUnitParameterFlag_NonRealTime;
                    info->unit = kAudioUnitParameterUnit_Generic;

                    d_strncpy(info->name, "Bypass", sizeof(info->name));
                    info->cfNameString = CFSTR("Bypass");
                }
                else
                {
                    const uint32_t hints = fPlugin.getParameterHints(inElement);

                    info->flags |= kAudioUnitParameterFlag_CFNameRelease;

                    if (hints & kParameterIsOutput)
                    {
                        info->flags |= kAudioUnitParameterFlag_MeterReadOnly;
                    }
                    else
                    {
                        info->flags |= kAudioUnitParameterFlag_IsWritable;

                        if ((hints & kParameterIsAutomatable) == 0x0)
                            info->flags |= kAudioUnitParameterFlag_NonRealTime;
                    }

                    if (hints & kParameterIsBoolean)
                        info->unit = kAudioUnitParameterUnit_Boolean;
                    else if (hints & kParameterIsInteger)
                        info->unit = kAudioUnitParameterUnit_Indexed;
                    else
                        info->unit = kAudioUnitParameterUnit_Generic;

                    // | kAudioUnitParameterFlag_ValuesHaveStrings;

                    const String& name(fPlugin.getParameterName(inElement));
                    d_strncpy(info->name, name, sizeof(info->name));
                    info->cfNameString = static_cast<CFStringRef>([[NSString stringWithUTF8String:name] retain]);
                }

                info->minValue = ranges.min;
                info->maxValue = ranges.max;
                info->defaultValue = ranges.def;
            }
            return noErr;
        case kAudioUnitProperty_StreamFormat:
            {
                AudioStreamBasicDescription* const desc = static_cast<AudioStreamBasicDescription*>(outData);
                std::memset(desc, 0, sizeof(*desc));

                if (inElement != 0)
                    return kAudioUnitErr_InvalidElement;

                if (inScope == kAudioUnitScope_Input)
                    desc->mChannelsPerFrame = DISTRHO_PLUGIN_NUM_INPUTS;
                else if (inScope == kAudioUnitScope_Output)
                    desc->mChannelsPerFrame = DISTRHO_PLUGIN_NUM_OUTPUTS;
                else
                    return kAudioUnitErr_InvalidScope;

                desc->mFormatID         = kAudioFormatLinearPCM;
                desc->mFormatFlags      = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
                desc->mSampleRate       = fPlugin.getSampleRate();
                desc->mBitsPerChannel   = 32;
                desc->mBytesPerFrame    = sizeof(float);
                desc->mBytesPerPacket   = sizeof(float);
                desc->mFramesPerPacket  = 1;
            }
            return noErr;
        case kAudioUnitProperty_ElementCount:
            switch (inScope)
            {
            case kAudioUnitScope_Global:
                *static_cast<UInt32*>(outData) = 1;
                break;
            case kAudioUnitScope_Input:
                *static_cast<UInt32*>(outData) = DISTRHO_PLUGIN_NUM_INPUTS != 0 ? 1 : 0;
                break;
            case kAudioUnitScope_Output:
                *static_cast<UInt32*>(outData) = DISTRHO_PLUGIN_NUM_OUTPUTS != 0 ? 1 : 0;
                break;
            default:
                *static_cast<UInt32*>(outData) = 0;
                break;
            }
            return noErr;
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kAudioUnitProperty_Latency:
            *static_cast<Float64*>(outData) = static_cast<double>(fPlugin.getLatency()) / fPlugin.getSampleRate();
            return noErr;
       #endif
        case kAudioUnitProperty_SupportedNumChannels:
            *static_cast<AUChannelInfo*>(outData) = { DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS };
            return noErr;
        case kAudioUnitProperty_MaximumFramesPerSlice:
            *static_cast<UInt32*>(outData) = fPlugin.getBufferSize();
            return noErr;
        case kAudioUnitProperty_SetRenderCallback:
            // TODO
            break;
       #if DISTRHO_PLUGIN_HAS_UI
        case kAudioUnitProperty_CocoaUI:
            {
                AudioUnitCocoaViewInfo* const info = static_cast<AudioUnitCocoaViewInfo*>(outData);
                std::memset(info, 0, sizeof(*info));

                NSString* const bundlePathString = [[NSString alloc]
                    initWithBytes:d_nextBundlePath
                           length:strlen(d_nextBundlePath)
                         encoding:NSUTF8StringEncoding];

                info->mCocoaAUViewBundleLocation = static_cast<CFURLRef>([[NSURL fileURLWithPath: bundlePathString] retain]);
                info->mCocoaAUViewClass[0] = CFSTR("DPF_UI_ViewFactory");

                [bundlePathString release];
            }
            return noErr;
       #endif
        }

        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auSetProperty(const AudioUnitPropertyID prop,
                           const AudioUnitScope inScope,
                           const AudioUnitElement inElement,
                           const void* const inData,
                           const UInt32 inDataSize)
    {
        switch (prop)
        {
        case kAudioUnitProperty_ClassInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(CFPropertyListRef*), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            /* TODO used for restoring plugin state
            *static_cast<CFPropertyListRef*>(inData);
            */
            break;
        case kAudioUnitProperty_MakeConnection:
            // TODO
            return noErr;
        case kAudioUnitProperty_StreamFormat:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AudioStreamBasicDescription), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const AudioStreamBasicDescription* const desc = static_cast<const AudioStreamBasicDescription*>(inData);

                const uint flags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
                DISTRHO_SAFE_ASSERT_UINT_RETURN(desc->mFormatID == kAudioFormatLinearPCM, desc->mFormatID, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_UINT_RETURN((desc->mFormatFlags & flags) == flags, desc->mFormatFlags, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_UINT_RETURN(desc->mBitsPerChannel == 32, desc->mBitsPerChannel, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_UINT_RETURN(desc->mBytesPerFrame == sizeof(float), desc->mBytesPerFrame, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_UINT_RETURN(desc->mBytesPerPacket == sizeof(float), desc->mBytesPerPacket, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_UINT_RETURN(desc->mFramesPerPacket == 1, desc->mFramesPerPacket, kAudioUnitErr_FormatNotSupported);

                if (inScope == kAudioUnitScope_Input) {
                    DISTRHO_SAFE_ASSERT_UINT_RETURN(desc->mChannelsPerFrame == DISTRHO_PLUGIN_NUM_INPUTS, desc->mChannelsPerFrame, kAudioUnitErr_FormatNotSupported);
                }
                else {
                    DISTRHO_SAFE_ASSERT_UINT_RETURN(desc->mChannelsPerFrame == DISTRHO_PLUGIN_NUM_OUTPUTS, desc->mChannelsPerFrame, kAudioUnitErr_FormatNotSupported);
                }

                fPlugin.setSampleRate(desc->mSampleRate, true);
            }
            return noErr;
        case kAudioUnitProperty_MaximumFramesPerSlice:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(UInt32), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            fPlugin.setBufferSize(*static_cast<const UInt32*>(inData));
            return noErr;
        case kAudioUnitProperty_SetRenderCallback:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            // TODO
            return noErr;
        }

        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auAddPropertyListener(const AudioUnitPropertyID prop,
                                   const AudioUnitPropertyListenerProc proc,
                                   void* const userData)
    {
        return noErr;
    }

    OSStatus auRemovePropertyListener(const AudioUnitPropertyID prop, const AudioUnitPropertyListenerProc proc)
    {
        return noErr;
    }

    OSStatus auRemovePropertyListenerWithUserData(const AudioUnitPropertyID prop,
                                                  const AudioUnitPropertyListenerProc proc,
                                                  void* const userData)
    {
        return noErr;
    }

    OSStatus auAddRenderNotify(const AURenderCallback proc, void* const userData)
    {
        return noErr;
    }

    OSStatus auRemoveRenderNotify(const AURenderCallback proc, void* const userData)
    {
        return noErr;
    }

    OSStatus auGetParameter(const AudioUnitParameterID param,
                            const AudioUnitScope scope,
                            const AudioUnitElement elem,
                            AudioUnitParameterValue* const value)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope, kAudioUnitErr_InvalidScope);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(param < fParameterCount, param, kAudioUnitErr_InvalidElement);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(elem == 0, elem, kAudioUnitErr_InvalidElement);

        *value = fPlugin.getParameterValue(param);
        return noErr;
    }

    OSStatus auSetParameter(const AudioUnitParameterID param,
                            const AudioUnitScope scope,
                            const AudioUnitElement elem,
                            const AudioUnitParameterValue value,
                            const UInt32 bufferOffset)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope, kAudioUnitErr_InvalidScope);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(param < fParameterCount, param, kAudioUnitErr_InvalidElement);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(elem == 0, elem, kAudioUnitErr_InvalidElement);

        fPlugin.setParameterValue(param, value);
        return noErr;
    }

    OSStatus auScheduleParameters(const AudioUnitParameterEvent* const events, const UInt32 numEvents)
    {
        return noErr;
    }

    OSStatus auRender(AudioUnitRenderActionFlags& ioActionFlags,
                      const AudioTimeStamp& inTimeStamp,
                      const UInt32 inBusNumber,
                      const UInt32 inFramesToProcess,
                      AudioBufferList& ioData)
    {
        if (inFramesToProcess > fPlugin.getBufferSize())
            return kAudioUnitErr_TooManyFramesToProcess;

        return noErr;
    }

    OSStatus auReset(const AudioUnitScope scope, const AudioUnitElement elem)
    {
        return noErr;
    }

#if 0
protected:
    // ----------------------------------------------------------------------------------------------------------------
    // ComponentBase AU dispatch

    OSStatus Render(AudioUnitRenderActionFlags& ioActionFlags,
                    const AudioTimeStamp& inTimeStamp,
                    const UInt32 nFrames) override
    {
        float value;
        for (uint32_t i=0; i<fParameterCount; ++i)
        {
            if (fPlugin.isParameterOutputOrTrigger(i))
            {
                value = fPlugin.getParameterValue(i);

                if (d_isEqual(fCachedParameterValues[i], value))
                    continue;

                fCachedParameterValues[i] = value;

                if (AUElement* const elem = GlobalScope().GetElement(0))
                    elem->SetParameter(i, value);
            }
        }

        return noErr;
    }
#endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    PluginExporter fPlugin;

    const uint32_t fParameterCount;
    float* fCachedParameterValues;

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent&)
    {
        return true;
    }

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginHolder*>(ptr)->writeMidi(midiEvent);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(uint32_t, float)
    {
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return static_cast<PluginHolder*>(ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    bool updateState(const char*, const char*)
    {
        return true;
    }

    static bool updateStateValueCallback(void* const ptr, const char* const key, const char* const value)
    {
        return static_cast<PluginHolder*>(ptr)->updateState(key, value);
    }
   #endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAU)
};

// --------------------------------------------------------------------------------------------------------------------

struct AudioComponentPlugInInstance {
	AudioComponentPlugInInterface acpi;
    PluginAU* plugin;

    AudioComponentPlugInInstance() noexcept
        : acpi(),
          plugin(nullptr)
    {
        std::memset(&acpi, 0, sizeof(acpi));
        acpi.Open = Open;
		acpi.Close = Close;
		acpi.Lookup = Lookup;
		acpi.reserved = NULL;
    }

    ~AudioComponentPlugInInstance()
    {
        delete plugin;
    }

	static OSStatus Open(void* const self, const AudioUnit component)
    {
        static_cast<AudioComponentPlugInInstance*>(self)->plugin = new PluginAU(component);
        return noErr;
    }

	static OSStatus Close(void* const self)
    {
        delete static_cast<AudioComponentPlugInInstance*>(self);
        return noErr;
    }

    static AudioComponentMethod Lookup(const SInt16 selector)
    {
        d_stdout("AudioComponentPlugInInstance::Lookup(%3d:%s)", selector, AudioUnitSelector2Str(selector));

        switch (selector)
        {
        case kAudioUnitInitializeSelect:
            return reinterpret_cast<AudioComponentMethod>(Initialize);
        case kAudioUnitUninitializeSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitGetPropertyInfoSelect:
            return reinterpret_cast<AudioComponentMethod>(GetPropertyInfo);
        case kAudioUnitGetPropertySelect:
            return reinterpret_cast<AudioComponentMethod>(GetProperty);
        case kAudioUnitSetPropertySelect:
            return reinterpret_cast<AudioComponentMethod>(SetProperty);
        case kAudioUnitAddPropertyListenerSelect:
            return reinterpret_cast<AudioComponentMethod>(AddPropertyListener);
        case kAudioUnitRemovePropertyListenerSelect:
            return reinterpret_cast<AudioComponentMethod>(RemovePropertyListener);
        case kAudioUnitRemovePropertyListenerWithUserDataSelect:
            return reinterpret_cast<AudioComponentMethod>(RemovePropertyListenerWithUserData);
        case kAudioUnitAddRenderNotifySelect:
            return reinterpret_cast<AudioComponentMethod>(AddRenderNotify);
        case kAudioUnitRemoveRenderNotifySelect:
            return reinterpret_cast<AudioComponentMethod>(RemoveRenderNotify);
        case kAudioUnitGetParameterSelect:
            return reinterpret_cast<AudioComponentMethod>(GetParameter);
        case kAudioUnitSetParameterSelect:
            return reinterpret_cast<AudioComponentMethod>(SetParameter);
        case kAudioUnitScheduleParametersSelect:
            return reinterpret_cast<AudioComponentMethod>(ScheduleParameters);
        case kAudioUnitRenderSelect:
            return reinterpret_cast<AudioComponentMethod>(Render);
        /*
        case kAudioUnitComplexRenderSelect:
            return reinterpret_cast<AudioComponentMethod>(ComplexRender);
        */
        case kAudioUnitResetSelect:
            return reinterpret_cast<AudioComponentMethod>(Reset);
        /*
        case kAudioUnitProcessSelect:
            return reinterpret_cast<AudioComponentMethod>(Process);
        case kAudioUnitProcessMultipleSelect:
            return reinterpret_cast<AudioComponentMethod>(ProcessMultiple);
        */
        }

        return nullptr;
    }

    static OSStatus Initialize(AudioComponentPlugInInstance* const self)
    {
        d_stdout("AudioComponentPlugInInstance::Initialize(%p)", self);
        return self->plugin->auInitialize();
    }

    static OSStatus Uninitialize(AudioComponentPlugInInstance* const self)
    {
        d_stdout("AudioComponentPlugInInstance::Uninitialize(%p)", self);
        return self->plugin->auUninitialize();
    }

    static OSStatus GetPropertyInfo(AudioComponentPlugInInstance* const self,
                                    const AudioUnitPropertyID prop,
                                    const AudioUnitScope inScope,
                                    const AudioUnitElement inElement,
                                    UInt32* const outDataSize,
                                    Boolean* const outWritable)
    {
        d_stdout("AudioComponentPlugInInstance::GetPropertyInfo(%p, %2d:%s, %d:%s, %d, 0x%x, ...)",
                 self, prop, AudioUnitPropertyID2Str(prop), inScope, AudioUnitScope2Str(inScope), inElement);

		UInt32 dataSize = 0;
		Boolean writable = false;
        const OSStatus res = self->plugin->auGetPropertyInfo(prop, inScope, inElement, dataSize, writable);

		if (outDataSize != nullptr)
			*outDataSize = dataSize;

		if (outWritable != nullptr)
			*outWritable = writable;

        return res;
    }

    static OSStatus GetProperty(AudioComponentPlugInInstance* const self,
                                const AudioUnitPropertyID prop,
                                const AudioUnitScope inScope,
                                const AudioUnitElement inElement,
                                void* const outData,
                                UInt32* const ioDataSize)
    {
        d_stdout("AudioComponentPlugInInstance::GetProperty    (%p, %2d:%s, %d:%s, %d, 0x%x, ...)",
                 self, prop, AudioUnitPropertyID2Str(prop), inScope, AudioUnitScope2Str(inScope), inElement);
        DISTRHO_SAFE_ASSERT_RETURN(ioDataSize != nullptr, kAudio_ParamError);

        Boolean writable;
        UInt32 outDataSize = 0;
        OSStatus res;

        if (outData == nullptr)
        {
            res = self->plugin->auGetPropertyInfo(prop, inScope, inElement, outDataSize, writable);
            *ioDataSize = outDataSize;
            return res;
        }

        const UInt32 inDataSize = *ioDataSize;
        if (inDataSize == 0)
            return kAudio_ParamError;

        res = self->plugin->auGetPropertyInfo(prop, inScope, inElement, outDataSize, writable);

        if (res != noErr)
            return res;

		void* outBuffer;
        uint8_t* tmpBuffer;
        if (inDataSize < outDataSize)
		{
			tmpBuffer = new uint8_t[outDataSize];
			outBuffer = tmpBuffer;
		}
        else
        {
			tmpBuffer = nullptr;
			outBuffer = outData;
		}

        res = self->plugin->auGetProperty(prop, inScope, inElement, outBuffer);

		if (res != noErr)
        {
			*ioDataSize = 0;
            return res;
        }

        if (tmpBuffer != nullptr)
        {
            memcpy(outData, tmpBuffer, inDataSize);
            delete[] tmpBuffer;
        }
        else
        {
            *ioDataSize = outDataSize;
        }

        return noErr;
    }

    static OSStatus SetProperty(AudioComponentPlugInInstance* const self,
                                const AudioUnitPropertyID prop,
                                const AudioUnitScope inScope,
                                const AudioUnitElement inElement,
                                const void* const inData,
                                const UInt32 inDataSize)
    {
        d_stdout("AudioComponentPlugInInstance::SetProperty(%p, %d:%s, %d:%s, %d, 0x%x, %p, %u)",
                 self, prop, AudioUnitPropertyID2Str(prop), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
        return self->plugin->auSetProperty(prop, inScope, inElement, inData, inDataSize);
    }

    static OSStatus AddPropertyListener(AudioComponentPlugInInstance* const self,
                                        const AudioUnitPropertyID prop,
                                        const AudioUnitPropertyListenerProc proc,
                                        void* const userData)
    {
        d_stdout("AudioComponentPlugInInstance::AddPropertyListener(%p, %d:%s, %p, %p)",
                 self, prop, AudioUnitPropertyID2Str(prop), proc, userData);
        return self->plugin->auAddPropertyListener(prop, proc, userData);
    }

    static OSStatus RemovePropertyListener(AudioComponentPlugInInstance* const self,
                                           const AudioUnitPropertyID prop,
                                           const AudioUnitPropertyListenerProc proc)
    {
        d_stdout("AudioComponentPlugInInstance::RemovePropertyListener(%p, %d:%s, %p)",
                 self, prop, AudioUnitPropertyID2Str(prop), proc);
        return self->plugin->auRemovePropertyListener(prop, proc);
    }

    static OSStatus RemovePropertyListenerWithUserData(AudioComponentPlugInInstance* const self,
                                                       const AudioUnitPropertyID prop,
                                                       const AudioUnitPropertyListenerProc proc,
                                                       void* const userData)
    {
        d_stdout("AudioComponentPlugInInstance::RemovePropertyListenerWithUserData(%p, %d:%s, %p, %p)",
                 self, prop, AudioUnitPropertyID2Str(prop), proc, userData);
        return self->plugin->auRemovePropertyListenerWithUserData(prop, proc, userData);
    }

    static OSStatus AddRenderNotify(AudioComponentPlugInInstance* const self,
                                    const AURenderCallback proc,
                                    void* const userData)
    {
        d_stdout("AudioComponentPlugInInstance::AddRenderNotify(%p, %p, %p)",
                 self, proc, userData);
        return self->plugin->auAddRenderNotify(proc, userData);
    }

    static OSStatus RemoveRenderNotify(AudioComponentPlugInInstance* const self,
                                       const AURenderCallback proc,
                                       void* const userData)
    {
        d_stdout("AudioComponentPlugInInstance::RemoveRenderNotify(%p, %p, %p)",
                 self, proc, userData);
        return self->plugin->auRemoveRenderNotify(proc, userData);
    }

    static OSStatus GetParameter(AudioComponentPlugInInstance* const self,
                                 const AudioUnitParameterID param,
                                 const AudioUnitScope scope,
                                 const AudioUnitElement elem,
                                 AudioUnitParameterValue* const value)
    {
        d_stdout("AudioComponentPlugInInstance::GetParameter(%p, %d, %d:%s, 0x%x, %p)",
                 self, param, scope, AudioUnitScope2Str(scope), elem, value);
        return self->plugin->auGetParameter(param, scope, elem, value);
    }

    static OSStatus SetParameter(AudioComponentPlugInInstance* const self,
                                 const AudioUnitParameterID param,
                                 const AudioUnitScope scope,
                                 const AudioUnitElement elem,
                                 const AudioUnitParameterValue value,
                                 const UInt32 bufferOffset)
    {
        d_stdout("AudioComponentPlugInInstance::SetParameter(%p, %d %d:%s, 0x%x, %f, %u)",
                 self, param, scope, AudioUnitScope2Str(scope), elem, value, bufferOffset);
        return self->plugin->auSetParameter(param, scope, elem, value, bufferOffset);
    }

    static OSStatus ScheduleParameters(AudioComponentPlugInInstance* const self,
                                       const AudioUnitParameterEvent* const events,
                                       const UInt32 numEvents)
    {
        d_stdout("AudioComponentPlugInInstance::ScheduleParameters(%p, %p, %u)",
                 self, events, numEvents);
        return self->plugin->auScheduleParameters(events, numEvents);
    }

    static OSStatus Reset(AudioComponentPlugInInstance* const self,
                          const AudioUnitScope scope,
                          const AudioUnitElement elem)
    {
        d_stdout("AudioComponentPlugInInstance::Reset(%p, %d:%s, 0x%x)",
                 self, scope, AudioUnitScope2Str(scope), elem);
        return self->plugin->auReset(scope, elem);
    }

    static OSStatus Render(AudioComponentPlugInInstance* const self,
                           AudioUnitRenderActionFlags* ioActionFlags,
                           const AudioTimeStamp* const inTimeStamp,
                           const UInt32 inOutputBusNumber,
                           const UInt32 inNumberFrames,
                           AudioBufferList* const ioData)
    {
        d_stdout("AudioComponentPlugInInstance::Render(%p, %p, %p, %u, %u, %p)",
                 self, ioActionFlags, inTimeStamp, inOutputBusNumber, inNumberFrames, ioData);
        DISTRHO_SAFE_ASSERT_RETURN(inTimeStamp != nullptr, kAudio_ParamError);
        DISTRHO_SAFE_ASSERT_RETURN(ioData != nullptr, kAudio_ParamError);

        AudioUnitRenderActionFlags tmpFlags;

        if (ioActionFlags == nullptr)
        {
            tmpFlags = 0;
            ioActionFlags = &tmpFlags;
        }

        return self->plugin->auRender(*ioActionFlags, *inTimeStamp, inOutputBusNumber, inNumberFrames, *ioData);
    }

    DISTRHO_DECLARE_NON_COPYABLE(AudioComponentPlugInInstance)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
void* PluginAUFactory(const AudioComponentDescription*)
{
    USE_NAMESPACE_DISTRHO
    TRACE

    if (d_nextBufferSize == 0)
        d_nextBufferSize = 1156;

    if (d_isZero(d_nextSampleRate))
        d_nextSampleRate = 48000.0;

    if (d_nextBundlePath == nullptr)
    {
        static String bundlePath;

        String tmpPath(getBinaryFilename());
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));

        if (tmpPath.endsWith(DISTRHO_OS_SEP_STR "Contents"))
        {
            tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
            bundlePath = tmpPath;
        }
        else
        {
            bundlePath = "error";
        }

        d_nextBundlePath = bundlePath.buffer();
    }

    d_nextCanRequestParameterValueChanges = true;

    return new AudioComponentPlugInInstance();
}

// --------------------------------------------------------------------------------------------------------------------
