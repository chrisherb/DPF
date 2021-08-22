/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_UI_PRIVATE_DATA_HPP_INCLUDED
#define DISTRHO_UI_PRIVATE_DATA_HPP_INCLUDED

#include "../DistrhoUI.hpp"

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# include "../extra/Sleep.hpp"
#else
# include "../../dgl/Application.hpp"
# include "../../dgl/src/WindowPrivateData.hpp"
# include "../../dgl/src/pugl.hpp"
#endif

#if defined(DISTRHO_PLUGIN_TARGET_JACK) || defined(DISTRHO_PLUGIN_TARGET_DSSI)
# define DISTRHO_UI_IS_STANDALONE 1
#else
# define DISTRHO_UI_IS_STANDALONE 0
#endif

#if defined(DISTRHO_PLUGIN_TARGET_VST2) || defined(DISTRHO_PLUGIN_TARGET_VST3)
# undef DISTRHO_UI_USER_RESIZABLE
# define DISTRHO_UI_USER_RESIZABLE 0
#endif

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
START_NAMESPACE_DISTRHO
#else
START_NAMESPACE_DGL
#endif

// -----------------------------------------------------------------------
// Plugin Application, will set class name based on plugin details

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
struct PluginApplication
{
    IdleCallback* idleCallback;
    UI* ui;

    explicit PluginApplication()
        : idleCallback(nullptr),
          ui(nullptr) {}

    void addIdleCallback(IdleCallback* const cb)
    {
        DISTRHO_SAFE_ASSERT_RETURN(cb != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(idleCallback == nullptr,);

        idleCallback = cb;
    }

    bool isQuiting() const noexcept
    {
        return ui->isQuiting();
    }

    bool isStandalone() const noexcept
    {
        return DISTRHO_UI_IS_STANDALONE;
    }

    void exec()
    {
        while (ui->isRunning())
        {
            d_msleep(30);
            idleCallback->idleCallback();
        }
    }

    // these are not needed
    void idle() {}
    void quit() {}

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginApplication)
};
#else
class PluginApplication : public Application
{
public:
    explicit PluginApplication()
        : Application(DISTRHO_UI_IS_STANDALONE)
    {
        const char* const className = (
#ifdef DISTRHO_PLUGIN_BRAND
            DISTRHO_PLUGIN_BRAND
#else
            DISTRHO_MACRO_AS_STRING(DISTRHO_NAMESPACE)
#endif
            "-" DISTRHO_PLUGIN_NAME
        );
        setClassName(className);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginApplication)
};
#endif

// -----------------------------------------------------------------------
// Plugin Window, will pass some Window events to UI

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
class PluginWindow
{
    UI* const ui;

public:
    explicit PluginWindow(UI* const uiPtr, PluginApplication& app)
        : ui(uiPtr)
    {
        app.ui = ui;
    }

    // fetch cached data
    uint getWidth() const noexcept { return ui->pData.width; }
    uint getHeight() const noexcept { return ui->pData.height; }
    double getScaleFactor() const noexcept { return ui->pData.scaleFactor; }
    uintptr_t getNativeWindowHandle() const noexcept { return ui->pData.parentWindowHandle; }

    // direct mappings
    bool isVisible() const noexcept { return ui->isVisible(); }
    void focus() { ui->focus(); }
    void show() { ui->show(); }
    void setTitle(const char* const title) { ui->setTitle(title); }
    void setVisible(const bool visible) { ui->setVisible(visible); }

    // custom
    void close()
    {
        ui->hide();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
#else // DISTRHO_PLUGIN_HAS_EXTERNAL_UI
class PluginWindow : public Window
{
    DISTRHO_NAMESPACE::UI* const ui;

public:
    explicit PluginWindow(DISTRHO_NAMESPACE::UI* const uiPtr,
                          PluginApplication& app,
                          const uintptr_t parentWindowHandle,
                          const uint width,
                          const uint height,
                          const double scaleFactor)
        : Window(app, parentWindowHandle, width, height, scaleFactor, DISTRHO_UI_USER_RESIZABLE),
          ui(uiPtr)
    {
        puglBackendEnter(pData->view);
    }

    void leaveContext()
    {
        puglBackendLeave(pData->view);
    }

protected:
    void onFocus(const bool focus, const DGL_NAMESPACE::CrossingMode mode) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->uiFocus(focus, mode);
    }

    void onReshape(const uint width, const uint height) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->uiReshape(width, height);
    }

    void onScaleFactorChanged(const double scaleFactor) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->uiScaleFactorChanged(scaleFactor);
    }

# ifndef DGL_FILE_BROWSER_DISABLED
    void onFileSelected(const char* filename) override;
# endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
#endif // DISTRHO_PLUGIN_HAS_EXTERNAL_UI

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
END_NAMESPACE_DISTRHO
#else
END_NAMESPACE_DGL
#endif

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
using DGL_NAMESPACE::PluginApplication;
using DGL_NAMESPACE::PluginWindow;
#endif

// -----------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc)   (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)    (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)    (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)    (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*setSizeFunc)     (void* ptr, uint width, uint height);
typedef bool (*fileRequestFunc) (void* ptr, const char* key);

// -----------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DGL
    PluginApplication app;
    ScopedPointer<PluginWindow> window;

    // DSP
    double   sampleRate;
    uint32_t parameterOffset;
    void*    dspPtr;

    // UI
    uint bgColor;
    uint fgColor;
    double scaleFactor;
    uintptr_t winId;
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI && !defined(DGL_FILE_BROWSER_DISABLED)
    char* uiStateFileKeyRequest;
#endif

    // Callbacks
    void*           callbacksPtr;
    editParamFunc   editParamCallbackFunc;
    setParamFunc    setParamCallbackFunc;
    setStateFunc    setStateCallbackFunc;
    sendNoteFunc    sendNoteCallbackFunc;
    setSizeFunc     setSizeCallbackFunc;
    fileRequestFunc fileRequestCallbackFunc;

    PrivateData() noexcept
        : app(),
          window(nullptr),
          sampleRate(0),
          parameterOffset(0),
          dspPtr(nullptr),
          bgColor(0),
          fgColor(0xffffffff),
          scaleFactor(1.0),
          winId(0),
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI && !defined(DGL_FILE_BROWSER_DISABLED)
          uiStateFileKeyRequest(nullptr),
#endif
          callbacksPtr(nullptr),
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          setSizeCallbackFunc(nullptr),
          fileRequestCallbackFunc(nullptr)
    {
#if defined(DISTRHO_PLUGIN_TARGET_DSSI) || defined(DISTRHO_PLUGIN_TARGET_LV2)
        parameterOffset += DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;
# if DISTRHO_PLUGIN_WANT_LATENCY
        parameterOffset += 1;
# endif
#endif

#ifdef DISTRHO_PLUGIN_TARGET_LV2
# if (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_TIMEPOS || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
# endif
# if (DISTRHO_PLUGIN_WANT_MIDI_OUTPUT || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
# endif
#endif
    }

    ~PrivateData() noexcept
    {
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI && !defined(DGL_FILE_BROWSER_DISABLED)
        std::free(uiStateFileKeyRequest);
#endif
    }

    void editParamCallback(const uint32_t rindex, const bool started)
    {
        if (editParamCallbackFunc != nullptr)
            editParamCallbackFunc(callbacksPtr, rindex, started);
    }

    void setParamCallback(const uint32_t rindex, const float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(callbacksPtr, rindex, value);
    }

    void setStateCallback(const char* const key, const char* const value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(callbacksPtr, key, value);
    }

    void sendNoteCallback(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(callbacksPtr, channel, note, velocity);
    }

    void setSizeCallback(const uint width, const uint height)
    {
        if (setSizeCallbackFunc != nullptr)
            setSizeCallbackFunc(callbacksPtr, width, height);
    }

    // implemented below, after PluginWindow
    bool fileRequestCallback(const char* const key);

    static UI::PrivateData* s_nextPrivateData;
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    static ExternalWindow::PrivateData createNextWindow(UI* ui, uint width, uint height);
#else
    static PluginWindow& createNextWindow(UI* ui, uint width, uint height);
#endif
};

// -----------------------------------------------------------------------
// UI private data fileRequestCallback, which requires PluginWindow definitions

inline bool UI::PrivateData::fileRequestCallback(const char* const key)
{
    if (fileRequestCallbackFunc != nullptr)
        return fileRequestCallbackFunc(callbacksPtr, key);

#if DISTRHO_PLUGIN_WANT_STATEFILES && !DISTRHO_PLUGIN_HAS_EXTERNAL_UI && !defined(DGL_FILE_BROWSER_DISABLED)
    std::free(uiStateFileKeyRequest);
    uiStateFileKeyRequest = strdup(key);
    DISTRHO_SAFE_ASSERT_RETURN(uiStateFileKeyRequest != nullptr, false);

    char title[0xff];
    snprintf(title, sizeof(title)-1u, DISTRHO_PLUGIN_NAME ": %s", key);
    title[sizeof(title)-1u] = '\0';

    DGL_NAMESPACE::Window::FileBrowserOptions opts;
    opts.title = title;
    return window->openFileBrowser(opts);
#endif

    return false;
}

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// PluginWindow onFileSelected that require UI::PrivateData definitions

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI && !defined(DGL_FILE_BROWSER_DISABLED)
START_NAMESPACE_DGL

inline void PluginWindow::onFileSelected(const char* const filename)
{
    DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

# if DISTRHO_PLUGIN_WANT_STATEFILES
    if (char* const key = ui->uiData->uiStateFileKeyRequest)
    {
        ui->uiData->uiStateFileKeyRequest = nullptr;
        if (filename != nullptr)
        {
            // notify DSP
            ui->setState(key, filename);
            // notify UI
            ui->stateChanged(key, filename);
        }
        std::free(key);
        return;
    }
# endif

    ui->uiFileBrowserSelected(filename);
}

END_NAMESPACE_DGL
#endif

// -----------------------------------------------------------------------

#endif // DISTRHO_UI_PRIVATE_DATA_HPP_INCLUDED
