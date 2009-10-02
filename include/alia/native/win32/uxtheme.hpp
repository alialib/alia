#ifndef ALIA_NATIVE_WIN32_UXTHEME_HPP
#define ALIA_NATIVE_WIN32_UXTHEME_HPP

#include <alia/native/win32/dll.hpp>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

// interface to uxtheme.dll

namespace alia { namespace native {

class theme_dll
{
 public:
    // API functions
    HTHEME (WINAPI* OpenThemeData)(
        HWND hwnd,
        LPCWSTR pszClassList);
    HRESULT (WINAPI* CloseThemeData)(
        HTHEME hTheme);
    HRESULT (WINAPI* DrawThemeBackground)(
        HTHEME hTheme,
        HDC hdc, 
        int iPartId,
        int iStateId,
        const RECT *pRect, 
        const RECT *pClipRect);
    HRESULT (WINAPI* DrawThemeText)(
        HTHEME hTheme,
        HDC hdc,
        int iPartId,
        int iStateId,
        LPCWSTR pszText,
        int iCharCount,
        DWORD dwTextFlags,
        DWORD dwTextFlags2,
        const RECT *pRect);
    HRESULT (WINAPI* GetThemeBackgroundContentRect)(
        HTHEME hTheme,
        HDC hdc,
        int iPartId,
        int iStateId, 
        const RECT *pBoundingRect, 
        RECT *pContentRect);
    HRESULT (WINAPI* GetThemePartSize)(
        HTHEME hTheme,
        HDC hdc,
        int iPartId,
        int iStateId,
        LPCRECT prc,
        THEMESIZE eSize,
        SIZE *psz);
    HRESULT (WINAPI* GetThemeColor)(
        HTHEME hTheme,
        int iPartId,
        int iStateId,
        int iPropId,
        COLORREF *pColor);
    HRESULT (WINAPI* GetThemeFont)(
        HTHEME hTheme,
        HDC hdc,
        int iPartId,
        int iStateId,
        int iPropId,
        LOGFONTW *pFont);
    COLORREF (WINAPI* GetThemeSysColor)(
        HTHEME hTheme,
        int iColorID);
    HRESULT (WINAPI* GetThemeSysFont)(
        HTHEME hTheme,
        int iFontID,
        LOGFONTW *plf);
    HRESULT (WINAPI* GetCurrentThemeName)(
        LPWSTR pszThemeFileName,
        int dwMaxNameChars,
        LPWSTR pszColorBuff,
        int cchMaxColorChars,
        LPWSTR pszSizeBuff,
        int cchMaxSizeChars);

    theme_dll();

 private:
    dll lib;
};

class theme
{
 public:
    theme(theme_dll& dll, LPCWSTR name_list) : dll(dll)
    {
        handle = dll.OpenThemeData(NULL, name_list);
        if (handle == NULL)
            throw exception("unsupported theme");
    }

    ~theme() { dll.CloseThemeData(handle); }

    operator HTHEME () const { return handle; }

 private:
    HTHEME handle;
    theme_dll& dll;
};

class theme_set
{
 public:
    theme_dll dll;

    theme button, combo_box, edit, scrollbar, header, trackbar, progress;

    theme_set()
      : button(dll, L"button")
      , combo_box(dll, L"combobox")
      , edit(dll, L"edit")
      , scrollbar(dll, L"scrollbar")
      , header(dll, L"header")
      , trackbar(dll, L"trackbar")
      , progress(dll, L"progress")
    {}
};

}}

#endif
