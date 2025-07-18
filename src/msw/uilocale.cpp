///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/uilocale.cpp
// Purpose:     wxUILocale implementation for MSW
// Author:      Vadim Zeitlin
// Created:     2021-07-31
// Copyright:   (c) 2021 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_INTL

#include "wx/uilocale.h"
#include "wx/private/uilocale.h"

#include "wx/msw/private/uilocale.h"
#include "wx/msw/registry.h"

#include "wx/scopedarray.h"
#include "wx/dynlib.h"
#include "wx/wxcrt.h"

#ifndef LOCALE_NAME_USER_DEFAULT
    #define LOCALE_NAME_USER_DEFAULT nullptr
#endif

#ifndef LOCALE_NAME_MAX_LENGTH
    #define LOCALE_NAME_MAX_LENGTH   85
#endif

#ifndef MUI_LANGUAGE_NAME
    #define MUI_LANGUAGE_NAME 8
#endif

#ifndef LOCALE_ICONSTRUCTEDLOCALE
#define LOCALE_ICONSTRUCTEDLOCALE 0x0000007d
#endif

#ifndef LOCALE_RETURN_NUMBER
#define LOCALE_RETURN_NUMBER 0x20000000
#endif

#ifndef LOCALE_SENGLISHDISPLAYNAME
#define LOCALE_SENGLISHDISPLAYNAME    0x00000072
#endif

#ifndef LOCALE_SNATIVEDISPLAYNAME
#define LOCALE_SNATIVEDISPLAYNAME     0x00000073
#endif

#ifndef LOCALE_SENGLISHLANGUAGENAME
#define LOCALE_SENGLISHLANGUAGENAME   0x00001001
#endif

#ifndef LOCALE_SNATIVELANGUAGENAME
#define LOCALE_SNATIVELANGUAGENAME    0x00000004
#endif

#ifndef LOCALE_SENGLISHCOUNTRYNAME
#define LOCALE_SENGLISHCOUNTRYNAME    0x00001002
#endif

#ifndef LOCALE_SNATIVECOUNTRYNAME
#define LOCALE_SNATIVECOUNTRYNAME     0x00000008
#endif

#ifndef LOCALE_IREADINGLAYOUT
#define LOCALE_IREADINGLAYOUT         0x00000070
#endif

#ifndef LOCALE_RETURN_GENITIVE_NAMES
#define LOCALE_RETURN_GENITIVE_NAMES  0x10000000
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

namespace
{
// Function to retrieve the user's preferred languages
void GetUserPreferredLanguagesFromRegistry(wxVector<wxString>& userLanguages)
{
    // Open the registry key for user preferred languages
    wxRegKey key(wxRegKey::HKCU, L"Control Panel\\International\\User Profile");
    if ( !key.Open(wxRegKey::Read) )
        return;

    // Retrieve the "Languages" value from the key
    DWORD type = REG_SZ;
    constexpr DWORD numChars = 256;
    DWORD valueSize = numChars*sizeof(WCHAR);
    wxScopedArray<WCHAR> languagesBuf(numChars + 1); // +1 for NUL at the end
    WCHAR* const languages = languagesBuf.get();
    if ( ::RegQueryValueEx(reinterpret_cast<HKEY>(key.GetHkey()),
                           L"Languages",
                           wxRESERVED_PARAM,
                           &type,
                           reinterpret_cast<LPBYTE>(languages),
                           &valueSize) == ERROR_SUCCESS )
    {
        // Ensure the buffer is NUL-terminated because this is not
        // guaranteed by RegQueryValueEx() for REG_MULTI_SZ values.
        const size_t actualLen = valueSize/sizeof(WCHAR);
        languages[actualLen] = L'\0';

        // Extract languages from multi-string value
        const WCHAR* p = languages;

        while ( *p != 0 )
        {
            const wxString language(p);
            userLanguages.push_back(language);
            p += language.length() + 1;

            if ( static_cast<size_t>(p - languages) >= actualLen )
                break;
        }
    }
}

} // anonymous namespace

LCTYPE wxGetLCTYPEFormatFromLocalInfo(wxLocaleInfo index)
{
    switch ( index )
    {
        case wxLOCALE_SHORT_DATE_FMT:
            return LOCALE_SSHORTDATE;

        case wxLOCALE_LONG_DATE_FMT:
            return LOCALE_SLONGDATE;

        case wxLOCALE_TIME_FMT:
            return LOCALE_STIMEFORMAT;

        default:
            wxFAIL_MSG( "no matching LCTYPE" );
    }

    return 0;
}

WXDLLIMPEXP_BASE wxString wxGetMSWDateTimeFormat(wxLocaleInfo index)
{
    if ( !wxUILocale::IsSet() )
    {
        // We don't want to use the date/time formats of "C" locale here
        // because this is incompatible with the behaviour in the previous
        // wxWidgets versions and inconsistent with the behaviour of
        // wxCalendarCtrl (which uses default user locale format), so let the
        // date/time controls keep using their default format.
        return wxString{};
    }

    wxString format;
    wxString localeName = wxUILocale::GetCurrent().GetName();
    if (localeName.IsSameAs("C"))
    {
        localeName = "en-US";
    }

    const wchar_t* name = localeName.wc_str();
    LCTYPE lctype = wxGetLCTYPEFormatFromLocalInfo(index);
    if (lctype != 0)
    {
        wchar_t buf[256];
        if (::GetLocaleInfoEx(name, lctype, buf, WXSIZEOF(buf)))
        {
            format = buf;
        }
        else
        {
            wxLogLastError(wxT("GetLocaleInfoEx"));
        }
    }
    return format;
}

// ----------------------------------------------------------------------------
// wxLocaleIdent::GetName() implementation for MSW
// ----------------------------------------------------------------------------

wxString wxLocaleIdent::GetName() const
{
    // Construct name in right format:
    // Windows: <language>-<script>-<REGION>

    wxString name;
    if ( !m_language.empty() )
    {
        name << m_language;

        if ( !m_script.empty() )
        {
            name << "-" << m_script;
        }

        if ( !m_region.empty() )
        {
            name << "-" << m_region;
        }

        if (!m_extension.empty())
        {
            name << "-" << m_extension;
        }

        if (!m_sortorder.empty())
        {
            name << "_" << m_sortorder;
        }
    }

    return name;
}

// ----------------------------------------------------------------------------
// Standard C wxUILocale implementation for MSW
// ----------------------------------------------------------------------------

class wxUILocaleImplStdC : public wxUILocaleImpl
{
public:

    // Create object corresponding to the given locale, return nullptr if not
    // supported.
    static wxUILocaleImplStdC* Create()
    {
        return new wxUILocaleImplStdC();
    }


    ~wxUILocaleImplStdC() override
    {
    }

    void Use() override
    {
    }

    wxString GetName() const override
    {
        return wxString("C");
    }

    wxLocaleIdent GetLocaleId() const override
    {
        return wxLocaleIdent().Language("C");
    }

    wxString GetInfo(wxLocaleInfo index, wxLocaleCategory cat) const override
    {
        return wxGetStdCLocaleInfo(index, cat);
    }

    wxString GetLocalizedName(wxLocaleName name, wxLocaleForm WXUNUSED(form)) const override
    {
        wxString str;
        switch (name)
        {
            case wxLOCALE_NAME_LOCALE:
            case wxLOCALE_NAME_LANGUAGE:
                str = wxString("English");
                break;
            case wxLOCALE_NAME_COUNTRY:
                str = wxString();
                break;
            default:
                wxFAIL_MSG("unknown wxLocaleInfo");
        }

        return str;
    }

#if wxUSE_DATETIME
    wxString GetMonthName(wxDateTime::Month month, wxDateTime::NameForm form) const override
    {
        return wxDateTime::GetEnglishMonthName(month, form);
    }

    wxString GetWeekDayName(wxDateTime::WeekDay weekday, wxDateTime::NameForm form) const override
    {
        return wxDateTime::GetEnglishWeekDayName(weekday, form);
    }
#endif // wxUSE_DATETIME

    wxLayoutDirection GetLayoutDirection() const override
    {
        return wxLayout_Default;
    }

    int CompareStrings(const wxString& lhs, const wxString& rhs,
        int flags) const override
    {
        const int rc = flags & wxCompare_CaseInsensitive ? lhs.CmpNoCase(rhs)
            : lhs.Cmp(rhs);
        if (rc < 0)
            return -1;
        if (rc > 0)
            return 1;
        return 0;
    }

private:
    // Ctor is private, use Create() instead.
    explicit wxUILocaleImplStdC()
    {
    }

    wxDECLARE_NO_COPY_CLASS(wxUILocaleImplStdC);
};

// ----------------------------------------------------------------------------
// Name-based wxUILocale implementation for MSW
// ----------------------------------------------------------------------------

class wxUILocaleImplName : public wxUILocaleImpl
{
public:
    static wxVector<wxString> GetPreferredUILanguages()
    {
        wxVector<wxString> preferred;

        // Check if Windows supports preferred UI languages.
        // Note: Windows 8.x might support them as well, but Windows 7
        // and below definitely do not.
        if (wxGetWinVersion() >= wxWinVersion_10)
        {
            GetUserPreferredLanguagesFromRegistry(preferred);

            ULONG numberOfLanguages = 0;
            ULONG bufferSize = 0;
            if (::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numberOfLanguages, nullptr, &bufferSize))
            {
                wxScopedArray<WCHAR> languages(bufferSize);
                if (::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numberOfLanguages, languages.get(), &bufferSize))
                {
                    WCHAR* buf = languages.get();
                    for (unsigned k = 0; k < numberOfLanguages; ++k)
                    {
                        const wxString language(buf);
                        preferred.push_back(language);
                        buf += language.length() + 1;
                    }
                }
                else
                {
                    wxLogLastError(wxT("GetUserPreferredUILanguages"));
                }
            }
            else
            {
                wxLogLastError(wxT("GetUserPreferredUILanguages"));
            }
        }
        else
        {
            // Use the default user locale for Windows 7 resp Windows 8.x and below
            wchar_t buf[LOCALE_NAME_MAX_LENGTH];
            if (::GetUserDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH))
            {
                preferred.push_back(buf);
            }
            else
            {
                wxLogLastError(wxT("GetUserDefaultLocaleName"));
            }
        }

        return preferred;
    }

    // Create object corresponding to the default user locale.
    static wxUILocaleImplName* CreateDefault()
    {
        return new wxUILocaleImplName(LOCALE_NAME_USER_DEFAULT);
    }

    // Create object corresponding to the given locale, return nullptr if not
    // supported.
    static wxUILocaleImplName* Create(const wchar_t* name)
    {
        // Getting the locale name seems to be the simplest way to see if it's
        // really supported: unknown locale result in an error here.
        if ( !::GetLocaleInfoEx(name, LOCALE_SNAME, nullptr, 0) )
            return nullptr;

        // Unfortunately under Windows 10 the call above only fails if the given
        // locale name is not a valid BCP 47 identifier. For example,
        // valid language codes can have 2 or 3 letters:
        // - using name "w" fails
        // - using name "wx" succeeds
        // - using name "wxy" succeeds
        // - using name "wxyz" fails
        // and so we need another check on these systems (but note that this
        // check must not be done under Windows 7 because there plenty of
        // actually supported locales are "constructed") by checking
        // whether the locale is "constructed" or not: "not constructed"
        // means the locale is a predefined locale, "constructed"
        // means the locale is not predefined, but has to be constructed.
        // For example, "de-US" would be a constructed locale.
        if ( wxGetWinVersion() >= wxWinVersion_10 )
        {
            // Using LOCALE_ICONSTRUCTEDLOCALE to query the locale status is
            // discouraged by Microsoft (the constant is not even defined in a
            // Windows header file). However, for now constructed locales will
            // be rejected here.
            int isConstructed = 0;
            if (!::GetLocaleInfoEx
                 (
                    name,
                    LOCALE_ICONSTRUCTEDLOCALE | LOCALE_RETURN_NUMBER,
                    (LPWSTR)&isConstructed,
                    sizeof(int)
                 ) || isConstructed != 0)
                return nullptr;
        }

        return new wxUILocaleImplName(name);
    }


    ~wxUILocaleImplName() override
    {
        free(const_cast<wchar_t*>(m_name));
    }

    void Use() override
    {
        // Construct a double NUL-terminated buffer.
        wchar_t buf[256];
        if ( m_name )
            wxStrlcpy(buf, m_name, WXSIZEOF(buf) - 1);
        else
            buf[0] = L'\0';
        buf[wxWcslen(buf) + 1] = L'\0';

        ULONG num = 1;

        if ( !::SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, buf, &num) )
            wxLogLastError(wxT("SetThreadPreferredUILanguages"));
    }

    wxString GetName() const override
    {
        return DoGetInfo(LOCALE_SNAME);
    }

    wxLocaleIdent GetLocaleId() const override
    {
        return wxLocaleIdent::FromTag(GetName());
    }

    wxString GetInfo(wxLocaleInfo index, wxLocaleCategory cat) const override
    {
        wxString str;
        switch ( index )
        {
            case wxLOCALE_THOUSANDS_SEP:
                str = DoGetInfo(LOCALE_STHOUSAND);
                break;

            case wxLOCALE_DECIMAL_POINT:
                str = DoGetInfo(cat == wxLOCALE_CAT_MONEY
                                    ? LOCALE_SMONDECIMALSEP
                                    : LOCALE_SDECIMAL);
                break;

            case wxLOCALE_SHORT_DATE_FMT:
            case wxLOCALE_LONG_DATE_FMT:
            case wxLOCALE_TIME_FMT:
                str = DoGetInfo(wxGetLCTYPEFormatFromLocalInfo(index));
                if ( !str.empty() )
                    str = wxTranslateFromUnicodeFormat(str);
                break;

            case wxLOCALE_DATE_TIME_FMT:
                // there doesn't seem to be any specific setting for this, so just
                // combine date and time ones
                //
                // we use the short date because this is what "%c" uses by default
                // ("%#c" uses long date but we have no way to specify the
                // alternate representation here)
                str << GetInfo(wxLOCALE_SHORT_DATE_FMT, cat)
                    << wxS(' ')
                    << GetInfo(wxLOCALE_TIME_FMT, cat);
                break;

            default:
                wxFAIL_MSG( "unknown wxLocaleInfo" );
        }

        return str;
    }

    wxString GetLocalizedName(wxLocaleName name, wxLocaleForm form) const override
    {
        wxString str;
        switch (name)
        {
            case wxLOCALE_NAME_LOCALE:
                switch (form)
                {
                    case wxLOCALE_FORM_NATIVE:
                        str = DoGetInfo(LOCALE_SNATIVEDISPLAYNAME);
                        break;
                    case wxLOCALE_FORM_ENGLISH:
                        str = DoGetInfo(LOCALE_SENGLISHDISPLAYNAME);
                        break;
                    default:
                        wxFAIL_MSG("unknown wxLocaleForm");
                }
                break;
            case wxLOCALE_NAME_LANGUAGE:
                switch (form)
                {
                    case wxLOCALE_FORM_NATIVE:
                        str = DoGetInfo(LOCALE_SNATIVELANGUAGENAME);
                        break;
                    case wxLOCALE_FORM_ENGLISH:
                        str = DoGetInfo(LOCALE_SENGLISHLANGUAGENAME);
                        break;
                    default:
                        wxFAIL_MSG("unknown wxLocaleForm");
                }
                break;
            case wxLOCALE_NAME_COUNTRY:
                switch (form)
                {
                    case wxLOCALE_FORM_NATIVE:
                        str = DoGetInfo(LOCALE_SNATIVECOUNTRYNAME);
                        break;
                    case wxLOCALE_FORM_ENGLISH:
                        str = DoGetInfo(LOCALE_SENGLISHCOUNTRYNAME);
                        break;
                    default:
                        wxFAIL_MSG("unknown wxLocaleForm");
                }
                break;
        }

        return str;
    }

#if wxUSE_DATETIME
    wxString GetMonthName(wxDateTime::Month month, wxDateTime::NameForm form) const override
    {
        static LCTYPE monthNameIndex[3][12] =
        {
            { LOCALE_SMONTHNAME1,  LOCALE_SMONTHNAME2,  LOCALE_SMONTHNAME3,
              LOCALE_SMONTHNAME4,  LOCALE_SMONTHNAME5,  LOCALE_SMONTHNAME6,
              LOCALE_SMONTHNAME7,  LOCALE_SMONTHNAME8,  LOCALE_SMONTHNAME9,
              LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12 },
            { LOCALE_SABBREVMONTHNAME1,  LOCALE_SABBREVMONTHNAME2,  LOCALE_SABBREVMONTHNAME3,
              LOCALE_SABBREVMONTHNAME4,  LOCALE_SABBREVMONTHNAME5,  LOCALE_SABBREVMONTHNAME6,
              LOCALE_SABBREVMONTHNAME7,  LOCALE_SABBREVMONTHNAME8,  LOCALE_SABBREVMONTHNAME9,
              LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 },
            { LOCALE_SABBREVMONTHNAME1,  LOCALE_SABBREVMONTHNAME2,  LOCALE_SABBREVMONTHNAME3,
              LOCALE_SABBREVMONTHNAME4,  LOCALE_SABBREVMONTHNAME5,  LOCALE_SABBREVMONTHNAME6,
              LOCALE_SABBREVMONTHNAME7,  LOCALE_SABBREVMONTHNAME8,  LOCALE_SABBREVMONTHNAME9,
              LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 }
        };

        const int idx = ArrayIndexFromFlag(form.GetFlags());
        if (idx == -1)
            return wxString();

        auto lctype = monthNameIndex[idx][month];
        switch ( form.GetContext() )
        {
            case wxDateTime::Context_Standalone:
                // Nothing else needed.
                break;

            case wxDateTime::Context_Formatting:
                lctype |= LOCALE_RETURN_GENITIVE_NAMES;
                break;
        }

        return DoGetInfo(lctype);
    }

    wxString GetWeekDayName(wxDateTime::WeekDay weekday, wxDateTime::NameForm form) const override
    {
        static LCTYPE weekdayNameIndex[3][12] =
        {
            { LOCALE_SDAYNAME7, LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3,
              LOCALE_SDAYNAME4, LOCALE_SDAYNAME5, LOCALE_SDAYNAME6 },
            { LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3,
              LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6 },
            { LOCALE_SSHORTESTDAYNAME7, LOCALE_SSHORTESTDAYNAME1, LOCALE_SSHORTESTDAYNAME2, LOCALE_SSHORTESTDAYNAME3,
              LOCALE_SSHORTESTDAYNAME4, LOCALE_SSHORTESTDAYNAME5, LOCALE_SSHORTESTDAYNAME6 }
        };

        const int idx = ArrayIndexFromFlag(form.GetFlags());
        if (idx == -1)
            return wxString();

        auto lctype = weekdayNameIndex[idx][weekday];
        switch ( form.GetContext() )
        {
            case wxDateTime::Context_Standalone:
                // Nothing else needed.
                break;

            case wxDateTime::Context_Formatting:
                lctype |= LOCALE_RETURN_GENITIVE_NAMES;
                break;
        }

        return DoGetInfo(lctype);
    }
#endif // wxUSE_DATETIME

    wxLayoutDirection GetLayoutDirection() const override
    {
        wxString str = DoGetInfo(LOCALE_IREADINGLAYOUT);
        // str contains a number between 0 and 3:
        // 0 = LTR, 1 = RTL, 2 = TTB+RTL, 3 = TTB + LTR
        // If str equals 1 return RTL, otherwise LTR
        return (str.IsSameAs("1") ? wxLayout_RightToLeft : wxLayout_LeftToRight);
    }

    int CompareStrings(const wxString& lhs, const wxString& rhs,
                       int flags) const override
    {
        DWORD dwFlags = 0;

        if ( flags & wxCompare_CaseInsensitive )
            dwFlags |= NORM_IGNORECASE;

        const int ret = ::CompareStringEx
            (
                m_name,
                dwFlags,
                lhs.wc_str(), -1,
                rhs.wc_str(), -1,
                nullptr,               // [out] version information -- not needed
                wxRESERVED_PARAM,
                wxRESERVED_PARAM
            );

        switch ( ret )
        {
            case CSTR_LESS_THAN:
                return -1;
            case CSTR_EQUAL:
                return 0;
            case CSTR_GREATER_THAN:
                return 1;
        }

        wxFAIL_MSG(wxS("Unreachable"));
        return 0;
    }

private:
    // Ctor is private, use CreateDefault() or Create() instead.
    //
    // Note that "name" can be null here (LOCALE_NAME_USER_DEFAULT).
    explicit wxUILocaleImplName(const wchar_t* name)
        : m_name(name ? wxStrdup(name) : nullptr)
    {
    }

    wxString DoGetInfo(LCTYPE lctype) const
    {
        wchar_t buf[256];
        if ( !::GetLocaleInfoEx(m_name, lctype, buf, WXSIZEOF(buf)) )
        {
            wxLogLastError(wxT("GetLocaleInfoEx"));
            return wxString();
        }

        return buf;
    }

    const wchar_t* const m_name;

    wxDECLARE_NO_COPY_CLASS(wxUILocaleImplName);
};

// ----------------------------------------------------------------------------
// wxUILocaleImpl implementation
// ----------------------------------------------------------------------------

/* static */
wxUILocaleImpl* wxUILocaleImpl::CreateStdC()
{
    return wxUILocaleImplStdC::Create();
}

/* static */
wxUILocaleImpl* wxUILocaleImpl::CreateUserDefault()
{
    return wxUILocaleImplName::CreateDefault();
}

/* static */
wxUILocaleImpl* wxUILocaleImpl::CreateForLanguage(const wxLanguageInfo& info)
{
    return wxUILocaleImplName::Create(info.LocaleTag.wc_str());
}

/* static */
wxUILocaleImpl* wxUILocaleImpl::CreateForLocale(const wxLocaleIdent& locId)
{
    return wxUILocaleImplName::Create(locId.GetTag(wxLOCALE_TAGTYPE_WINDOWS).wc_str());
}

/* static */
wxVector<wxString> wxUILocaleImpl::GetPreferredUILanguages()
{
    return wxUILocaleImplName::GetPreferredUILanguages();
}

#endif // wxUSE_INTL
