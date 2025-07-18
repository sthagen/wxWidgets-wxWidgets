/////////////////////////////////////////////////////////////////////////////
// Name:        wx/propgrid/property.h
// Purpose:     wxPGProperty and related support classes
// Author:      Jaakko Salli
// Created:     2008-08-23
// Copyright:   (c) Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PROPGRID_PROPERTY_H_
#define _WX_PROPGRID_PROPERTY_H_

#include "wx/defs.h"

#if wxUSE_PROPGRID

#include "wx/propgrid/propgriddefs.h"
#include "wx/bitmap.h"
#include "wx/bmpbndl.h"
#include "wx/font.h"
#include "wx/validate.h"

#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------

constexpr wxPGProperty* wxNullProperty = nullptr;


// Contains information relayed to property's OnCustomPaint.
struct wxPGPaintData
{
    // wxPropertyGrid
    const wxPropertyGrid*   m_parent;

    // Normally -1, otherwise index to drop-down list item
    // that has to be drawn.
    int                     m_choiceItem;

    // Set to drawn width in OnCustomPaint (optional).
    int                     m_drawnWidth;

    // In a measure item call, set this to the height of item
    // at m_choiceItem index.
    int                     m_drawnHeight;
};


// Base class for wxPropertyGrid cell renderers.
class WXDLLIMPEXP_PROPGRID wxPGCellRenderer : public wxObjectRefData
{
public:

    wxPGCellRenderer() = default;
    virtual ~wxPGCellRenderer() = default;

    // Render flags
    enum
    {
        // We are painting selected item
        Selected        = 0x00010000,

        // We are painting item in choice popup
        ChoicePopup     = 0x00020000,

        // We are rendering wxOwnerDrawnComboBox control
        // (or other owner drawn control, but that is only
        // officially supported one ATM).
        Control         = 0x00040000,

        // We are painting a disable property
        Disabled        = 0x00080000,

        // We are painting selected, disabled, or similar
        // item that dictates fore- and background colours,
        // overriding any cell values.
        DontUseCellFgCol    = 0x00100000,
        DontUseCellBgCol    = 0x00200000,
        DontUseCellColours  = DontUseCellFgCol |
                              DontUseCellBgCol
    };

    // Returns true if rendered something in the foreground
    // (text or bitmap).
    virtual bool Render( wxDC& dc,
                         const wxRect& rect,
                         const wxPropertyGrid* propertyGrid,
                         wxPGProperty* property,
                         int column,
                         int item,
                         int flags ) const = 0;

    // Returns size of the image in front of the editable area.
    // If property is null, then this call is for a custom value.
    // In that case the item is index to wxPropertyGrid's custom values.
    virtual wxSize GetImageSize( const wxPGProperty* property,
                                 int column,
                                 int item ) const;

    // Paints property category selection rectangle.
#if WXWIN_COMPATIBILITY_3_0
    wxDEPRECATED_BUT_USED_INTERNALLY_MSG("Use DrawCaptionSelectionRect(wxWindow*, wxDC&, ...) instead")
    virtual void DrawCaptionSelectionRect( wxDC& dc,
                                           int x, int y,
                                           int w, int h ) const;
#endif // WXWIN_COMPATIBILITY_3_0
    virtual void DrawCaptionSelectionRect(wxWindow *win, wxDC& dc,
                                          int x, int y, int w, int h) const;

    // Utility to draw vertically centered text.
    void DrawText( wxDC& dc,
                   const wxRect& rect,
                   int imageWidth,
                   const wxString& text ) const;

    // Utility to draw editor's value, or vertically
    // aligned text if editor is null.
    void DrawEditorValue( wxDC& dc, const wxRect& rect,
                          int xOffset, const wxString& text,
                          wxPGProperty* property,
                          const wxPGEditor* editor ) const;

    // Utility to render cell bitmap and set text
    // colour plus bg brush colour.
    // Returns image width, which, for instance,
    // can be passed to DrawText.
    int PreDrawCell( wxDC& dc,
                     const wxRect& rect,
                     const wxPropertyGrid* propGrid,
                     const wxPGCell& cell,
                     int flags ) const;

    // Utility to be called after drawing is done, to revert
    // whatever changes PreDrawCell() did.
    // Flags are the same as those passed to PreDrawCell().
    void PostDrawCell( wxDC& dc,
                       const wxPropertyGrid* propGrid,
                       const wxPGCell& cell,
                       int flags ) const;
};


// Default cell renderer, that can handles the common
// scenarios.
class WXDLLIMPEXP_PROPGRID wxPGDefaultRenderer : public wxPGCellRenderer
{
public:
    virtual bool Render( wxDC& dc,
                         const wxRect& rect,
                         const wxPropertyGrid* propertyGrid,
                         wxPGProperty* property,
                         int column,
                         int item,
                         int flags ) const override;

    virtual wxSize GetImageSize( const wxPGProperty* property,
                                 int column,
                                 int item ) const override;

protected:
};


class WXDLLIMPEXP_PROPGRID wxPGCellData : public wxObjectRefData
{
    friend class wxPGCell;
public:
    wxPGCellData();

    void SetText( const wxString& text )
    {
        m_text = text;
        m_hasValidText = true;
    }
    void SetBitmap( const wxBitmapBundle& bitmap ) { m_bitmapBundle = bitmap; }
    void SetFgCol( const wxColour& col ) { m_fgCol = col; }
    void SetBgCol( const wxColour& col ) { m_bgCol = col; }
    void SetFont( const wxFont& font ) { m_font = font; }

protected:
    virtual ~wxPGCellData() = default;

    wxString    m_text;
    wxBitmapBundle m_bitmapBundle;
    wxColour    m_fgCol;
    wxColour    m_bgCol;
    wxFont      m_font;

    // True if m_text is valid and specified
    bool        m_hasValidText;
};


// Base class for wxPropertyGrid cell information.
class WXDLLIMPEXP_PROPGRID wxPGCell : public wxObject
{
public:
    wxPGCell() = default;
    wxPGCell(const wxPGCell& other)
        : wxObject(other)
    {
    }

    wxPGCell( const wxString& text,
              const wxBitmapBundle& bitmap = wxBitmapBundle(),
              const wxColour& fgCol = wxNullColour,
              const wxColour& bgCol = wxNullColour );

    virtual ~wxPGCell() = default;

    wxPGCellData* GetData()
    {
        return (wxPGCellData*) m_refData;
    }

    const wxPGCellData* GetData() const
    {
        return (const wxPGCellData*) m_refData;
    }

    bool HasText() const
    {
        return (m_refData && GetData()->m_hasValidText);
    }

    // Sets empty but valid data to this cell object.
    void SetEmptyData();

    // Merges valid data from srcCell into this.
    void MergeFrom( const wxPGCell& srcCell );

    void SetText( const wxString& text );
    void SetBitmap( const wxBitmapBundle& bitmap );
    void SetFgCol( const wxColour& col );

    // Sets font of the cell.
    // Because wxPropertyGrid does not support rows of
    // different height, it makes little sense to change
    // size of the font. Therefore it is recommended
    // to use return value of wxPropertyGrid::GetFont()
    // or wxPropertyGrid::GetCaptionFont() as a basis
    // for the font that, after modifications, is passed
    // to this member function.
    void SetFont( const wxFont& font );

    void SetBgCol( const wxColour& col );

    const wxString& GetText() const { return GetData()->m_text; }
    const wxBitmapBundle& GetBitmap() const { return GetData()->m_bitmapBundle; }
    const wxColour& GetFgCol() const { return GetData()->m_fgCol; }

    // Returns font of the cell. If no specific font is set for this
    // cell, then the font will be invalid.
    const wxFont& GetFont() const { return GetData()->m_font; }

    const wxColour& GetBgCol() const { return GetData()->m_bgCol; }

    wxPGCell& operator=( const wxPGCell& other )
    {
        if ( this != &other )
        {
            Ref(other);
        }
        return *this;
    }

    // Used mostly internally to figure out if this cell is supposed
    // to have default values when attached to a grid.
    bool IsInvalid() const
    {
        return ( m_refData == nullptr );
    }

private:
    virtual wxObjectRefData *CreateRefData() const override
        { return new wxPGCellData(); }

    wxNODISCARD virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const override;
};

// -----------------------------------------------------------------------

// wxPGAttributeStorage is somewhat optimized storage for
// key=variant pairs (ie. a map).
class WXDLLIMPEXP_PROPGRID wxPGAttributeStorage
{
public:
    wxPGAttributeStorage() = default;
    wxPGAttributeStorage(const wxPGAttributeStorage& other);
    ~wxPGAttributeStorage();

    wxPGAttributeStorage& operator=(const wxPGAttributeStorage& rhs);

    void Set( const wxString& name, const wxVariant& value );
    unsigned int GetCount() const { return (unsigned int) m_map.size(); }
    wxVariant FindValue(const wxString& name) const;

    typedef std::unordered_map<wxString, wxVariantData*>::const_iterator const_iterator;
    const_iterator StartIteration() const;
    bool GetNext(const_iterator& it, wxVariant& variant) const;

protected:
    std::unordered_map<wxString, wxVariantData*> m_map;
};

enum class wxPGFlags : int
{
    // No flags.
    Null = 0,

    // Indicates bold font.
    Modified = 0x0001,

    // Disables ('greyed' text and editor does not activate) property.
    Disabled = 0x0002,

    // Hider button will hide this property.
    Hidden = 0x0004,

    // This property has custom paint image just in front of its value.
    // If property only draws custom images into a popup list, then this
    // flag should not be set.
    CustomImage = 0x0008,

    // Do not create text based editor for this property (but button-triggered
    // dialog and choice are ok).
    NoEditor = 0x0010,

    // Property is collapsed, ie. it's children are hidden.
    Collapsed = 0x0020,

    // If property is selected, then indicates that validation failed for pending
    // value.
    // If property is not selected, that indicates that the actual property
    // value has failed validation (NB: this behaviour is not currently supported,
    // but may be used in future).
    InvalidValue = 0x0040,

    // 0x0080,

    // Switched via SetWasModified(). Temporary flag - only used when
    // setting/changing property value.
    WasModified = 0x0200,

    // If set, then child properties (if any) are private, and should be
    // "invisible" to the application.
    Aggregate = 0x0400,

    // If set, then child properties (if any) are copies and should not
    // be deleted in dtor.
    ChildrenAreCopies = 0x0800,

    // Classifies this item as a non-category.
    //    Used for faster item type identification.
    Property = 0x1000,

    // Classifies this item as a category.
    // Used for faster item type identification.
    Category = 0x2000,

    // Classifies this item as a property that has children,
    //but is not aggregate (i.e. children are not private).
    MiscParent = 0x4000,

    // Property is read-only. Editor is still created for wxTextCtrl-based
    // property editors. For others, editor is not usually created because
    // they do implement wxTE_READONLY style or equivalent.
    ReadOnly = 0x8000,

    //
    // NB: FLAGS ABOVE 0x8000 CANNOT BE USED WITH PROPERTY ITERATORS
    //

    // Property's value is composed from values of child properties.
    // This flag cannot be used with property iterators.
    ComposedValue = 0x00010000,

    // Common value of property is selectable in editor.
    // This flag cannot be used with property iterators.
    UsesCommonValue = 0x00020000,

    // Property can be set to unspecified value via editor.
    // Currently, this applxPGies to following properties:
    // - wxIntProperty, wxUIntProperty, wxFloatProperty, wxEditEnumProperty:
    // Clear the text field
    // This flag cannot be used with property iterators.
    // See wxPGProperty::SetAutoUnspecified().
    AutoUnspecified = 0x00040000,

    // Indicates that the property is being deleted and should be ignored.
    BeingDeleted = 0x00080000,

    // If set, full path is shown in wxFileProperty.
    ShowFullFileName = 0x00100000,

    // For internal use only.
    Reserved_1 = 0x10000000,

    // For internal use only.
    Reserved_2 = 0x20000000,

    // For internal use only.
    Reserved_3 = 0x40000000,

    // Topmost flag.
    Max = ShowFullFileName,

    // Property with children must have one of these set, otherwise iterators
    // will not work correctly.
    // Code should automatically take care of this, however.
    ParentalFlags = Aggregate | Category | MiscParent,

    // Combination of flags that can be stored by GetFlagsAsString
    StringStoredFlags = Disabled | Hidden | NoEditor | Collapsed
};

constexpr wxPGFlags operator|(wxPGFlags a, wxPGFlags b)
{
    return static_cast<wxPGFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline wxPGFlags operator|=(wxPGFlags & a, wxPGFlags b)
{
    return a = a | b;
}

constexpr wxPGFlags operator&(wxPGFlags a, wxPGFlags b)
{
    return static_cast<wxPGFlags>(static_cast<int>(a) & static_cast<int>(b));
}

inline wxPGFlags operator&=(wxPGFlags & a, wxPGFlags b)
{
    return a = a & b;
}

constexpr wxPGFlags operator^(wxPGFlags a, wxPGFlags b)
{
    return static_cast<wxPGFlags>(static_cast<int>(a) ^ static_cast<int>(b));
}

constexpr wxPGFlags operator~(wxPGFlags a)
{
    return static_cast<wxPGFlags>(~static_cast<int>(a));
}

constexpr bool operator!(wxPGFlags a)
{
    return static_cast<int>(a) == 0;
}

// We need these operators with int arguments for interoperability
// with wxPG_ITERATOR_FLAGS as plain enumeration).
// =====
constexpr int operator<<(wxPGFlags a, int n)
{
    return static_cast<int>(a) << n;
}

constexpr int operator|(wxPGFlags a, int b)
{
    return static_cast<int>(a) | b;
}

constexpr int operator|(int a, wxPGFlags b)
{
    return a | static_cast<int>(b);
}

constexpr int operator&(int a, wxPGFlags b)
{
    return a & static_cast<int>(b);
}
// =====

#if WXWIN_COMPATIBILITY_3_2
// These constants themselves intentionally don't use wxDEPRECATED_MSG()
// because one will be given whenever they are used with any function now
// taking wxPGFlags anyhow and giving multiple deprecation warnings for
// the same line of code is more annoying than helpful.
enum wxPGPropertyFlags
{
    wxPG_PROP_MODIFIED = static_cast<int>(wxPGFlags::Modified),
    wxPG_PROP_DISABLED = static_cast<int>(wxPGFlags::Disabled),
    wxPG_PROP_HIDDEN = static_cast<int>(wxPGFlags::Hidden),
    wxPG_PROP_CUSTOMIMAGE = static_cast<int>(wxPGFlags::CustomImage),
    wxPG_PROP_NOEDITOR = static_cast<int>(wxPGFlags::NoEditor),
    wxPG_PROP_COLLAPSED = static_cast<int>(wxPGFlags::Collapsed),
    wxPG_PROP_INVALID_VALUE = static_cast<int>(wxPGFlags::InvalidValue),
    wxPG_PROP_WAS_MODIFIED = static_cast<int>(wxPGFlags::WasModified),
    wxPG_PROP_AGGREGATE = static_cast<int>(wxPGFlags::Aggregate),
    wxPG_PROP_CHILDREN_ARE_COPIES = static_cast<int>(wxPGFlags::ChildrenAreCopies),
    wxPG_PROP_PROPERTY = static_cast<int>(wxPGFlags::Property),
    wxPG_PROP_CATEGORY = static_cast<int>(wxPGFlags::Category),
    wxPG_PROP_MISC_PARENT = static_cast<int>(wxPGFlags::MiscParent),
    wxPG_PROP_READONLY = static_cast<int>(wxPGFlags::ReadOnly),
    wxPG_PROP_COMPOSED_VALUE = static_cast<int>(wxPGFlags::ComposedValue),
    wxPG_PROP_USES_COMMON_VALUE = static_cast<int>(wxPGFlags::UsesCommonValue),
    wxPG_PROP_AUTO_UNSPECIFIED = static_cast<int>(wxPGFlags::AutoUnspecified),
    wxPG_PROP_BEING_DELETED = static_cast<int>(wxPGFlags::BeingDeleted),
    wxPG_PROP_PARENTAL_FLAGS = static_cast<int>(wxPGFlags::ParentalFlags),
    wxPG_STRING_STORED_FLAGS = static_cast<int>(wxPGFlags::StringStoredFlags),
    wxPG_PROP_MAX = static_cast<int>(wxPGFlags::Max),
    wxPG_PROP_CLASS_SPECIFIC_1 = static_cast<int>(wxPGFlags::Reserved_1),
    wxPG_PROP_CLASS_SPECIFIC_2 = static_cast<int>(wxPGFlags::Reserved_2),
    wxPG_PROP_CLASS_SPECIFIC_3 = static_cast<int>(wxPGFlags::Reserved_3),
};
#endif // WXWIN_COMPATIBILITY_3_2

// -----------------------------------------------------------------------

// Helpers to mark macros as deprecated
//
// Note that we don't do it when building wx itself if 3.0 compatibility is on,
// as these macros are still used in our own code in this case.
#if (defined(__clang__) || defined(__GNUC__)) && \
        (!defined(WXBUILDING) || !WXWIN_COMPATIBILITY_3_0)
#define wxPG_STRINGIFY(X) #X
#define wxPG_DEPRECATED_MACRO_VALUE(value, msg) \
        _Pragma(wxPG_STRINGIFY(GCC warning msg)) value
#else
#define wxPG_DEPRECATED_MACRO_VALUE(value, msg) value
#endif // clang || GCC

#if defined(__VISUALC__)
#define wxPG_MUST_DEPRECATE_MACRO_NAME
#endif

// wxPGProperty::SetAttribute() and
// wxPropertyGridInterface::SetPropertyAttribute() accept one of these as
// attribute name argument.
// You can use strings instead of constants. However, some of these
// constants are redefined to use cached strings which may reduce
// your binary size by some amount.

// Set default value for property.
#define wxPG_ATTR_DEFAULT_VALUE           wxS("DefaultValue")

// Universal, int or double. Minimum value for numeric properties.
#define wxPG_ATTR_MIN                     wxS("Min")

// Universal, int or double. Maximum value for numeric properties.
#define wxPG_ATTR_MAX                     wxS("Max")

// Universal, string. When set, will be shown as text after the displayed
// text value. Alternatively, if third column is enabled, text will be shown
// there (for any type of property).
#define wxPG_ATTR_UNITS                     wxS("Units")

// When set, will be shown as 'greyed' text in property's value cell when
// the actual displayed value is blank.
#define wxPG_ATTR_HINT                      wxS("Hint")

// Universal, wxArrayString. Set to enable auto-completion in any
// wxTextCtrl-based property editor.
#define wxPG_ATTR_AUTOCOMPLETE              wxS("AutoComplete")

// wxBoolProperty and wxFlagsProperty specific. Value type is bool.
// Default value is False.
// When set to True, bool property will use check box instead of a
// combo box as its editor control. If you set this attribute
// for a wxFlagsProperty, it is automatically applied to child
// bool properties.
#define wxPG_BOOL_USE_CHECKBOX              wxS("UseCheckbox")

// wxBoolProperty and wxFlagsProperty specific. Value type is bool.
// Default value is False.
// Set to True for the bool property to cycle value on double click
// (instead of showing the popup listbox). If you set this attribute
// for a wxFlagsProperty, it is automatically applied to child
// bool properties.
#define wxPG_BOOL_USE_DOUBLE_CLICK_CYCLING  wxS("UseDClickCycling")

// wxFloatProperty (and similar) specific, int, default -1.
// Sets the (max) precision used when floating point value is rendered as
// text. The default -1 means infinite precision.
#define wxPG_FLOAT_PRECISION                wxS("Precision")

// The text will be echoed as asterisks (wxTE_PASSWORD will be passed
// to textctrl etc.).
#define wxPG_STRING_PASSWORD                wxS("Password")

// Define base used by a wxUIntProperty. Valid constants are
// wxPG_BASE_OCT, wxPG_BASE_DEC, wxPG_BASE_HEX and wxPG_BASE_HEXL
// (lowercase characters).
#define wxPG_UINT_BASE                      wxS("Base")

// Define prefix rendered to wxUIntProperty. Accepted constants
// wxPG_PREFIX_NONE, wxPG_PREFIX_0x, and wxPG_PREFIX_DOLLAR_SIGN.
// Note:
// Only wxPG_PREFIX_NONE works with Decimal and Octal numbers.
#define wxPG_UINT_PREFIX                    wxS("Prefix")

// Specific to wxEditorDialogProperty and derivatives, wxString, default is empty.
// Sets a specific title for the editor dialog.
#define wxPG_DIALOG_TITLE                   wxS("DialogTitle")

// wxFileProperty/wxImageFileProperty specific, wxChar*, default is
// detected/varies.
// Sets the wildcard used in the triggered wxFileDialog. Format is the same.
#define wxPG_FILE_WILDCARD                  wxS("Wildcard")

// wxFileProperty/wxImageFileProperty specific, int, default 1.
// When 0, only the file name is shown (i.e. drive and directory are hidden).
#define wxPG_FILE_SHOW_FULL_PATH            wxS("ShowFullPath")

// Specific to wxFileProperty and derived properties, wxString, default empty.
// If set, then the filename is shown relative to the given path string.
#define wxPG_FILE_SHOW_RELATIVE_PATH        wxS("ShowRelativePath")

// Specific to wxFileProperty and derived properties, wxString,
// default is empty.
// Sets the initial path of where to look for files.
#define wxPG_FILE_INITIAL_PATH              wxS("InitialPath")

#if WXWIN_COMPATIBILITY_3_0
#ifdef wxPG_MUST_DEPRECATE_MACRO_NAME
#pragma deprecated(wxPG_FILE_DIALOG_TITLE)
#endif
// Specific to wxFileProperty and derivatives, wxString, default is empty.
// Sets a specific title for the dir dialog.
#define wxPG_FILE_DIALOG_TITLE wxPG_DEPRECATED_MACRO_VALUE(wxS("DialogTitle"),\
    "wxPG_FILE_DIALOG_TITLE is deprecated. Use wxPG_DIALOG_TITLE instead.")
#endif // WXWIN_COMPATIBILITY_3_0

// Specific to wxFileProperty and derivatives, long, default is 0.
// Sets a specific wxFileDialog style for the file dialog, e.g. ::wxFD_SAVE.
#define wxPG_FILE_DIALOG_STYLE              wxS("DialogStyle")

#if WXWIN_COMPATIBILITY_3_0
#ifdef wxPG_MUST_DEPRECATE_MACRO_NAME
#pragma deprecated(wxPG_DIR_DIALOG_MESSAGE)
#endif
// Specific to wxDirProperty, wxString, default is empty.
// Sets a specific message for the dir dialog.
#define wxPG_DIR_DIALOG_MESSAGE wxPG_DEPRECATED_MACRO_VALUE(wxS("DialogMessage"),\
    "wxPG_DIR_DIALOG_MESSAGE is deprecated. Use wxPG_DIALOG_TITLE instead.")
#endif // WXWIN_COMPATIBILITY_3_0

// wxArrayStringProperty's string delimiter character. If this is
// a quotation mark or hyphen, then strings will be quoted instead
// (with given character).
// Default delimiter is quotation mark.
#define wxPG_ARRAY_DELIMITER                wxS("Delimiter")

// Sets displayed date format for wxDateProperty.
#define wxPG_DATE_FORMAT                    wxS("DateFormat")

// Sets wxDatePickerCtrl window style used with wxDateProperty. Default
// is wxDP_DEFAULT | wxDP_SHOWCENTURY. Using wxDP_ALLOWNONE will enable
// better unspecified value support in the editor
#define wxPG_DATE_PICKER_STYLE              wxS("PickerStyle")

#if wxUSE_SPINBTN
// SpinCtrl editor, int or double. How much number changes when button is
// pressed (or up/down on keyboard).
#define wxPG_ATTR_SPINCTRL_STEP             wxS("Step")

// SpinCtrl editor, bool. If true, value wraps at Min/Max.
#define wxPG_ATTR_SPINCTRL_WRAP             wxS("Wrap")

// SpinCtrl editor, bool. If true, moving mouse when one of the spin
//    buttons is depressed rapidly changing "spin" value.
#define wxPG_ATTR_SPINCTRL_MOTION           wxS("MotionSpin")
#endif  // wxUSE_SPINBTN

// wxMultiChoiceProperty, int.
// If 0, no user strings allowed. If 1, user strings appear before list
// strings. If 2, user strings appear after list string.
#define wxPG_ATTR_MULTICHOICE_USERSTRINGMODE    wxS("UserStringMode")

// wxColourProperty and its kind, int, default 1.
// Setting this attribute to 0 hides custom colour from property's list of
// choices.
#define wxPG_COLOUR_ALLOW_CUSTOM            wxS("AllowCustom")

// wxColourProperty and its kind: Set to True in order to support editing
// alpha colour component.
#define wxPG_COLOUR_HAS_ALPHA               wxS("HasAlpha")

// -----------------------------------------------------------------------

// Data of a single wxPGChoices choice.
class WXDLLIMPEXP_PROPGRID wxPGChoiceEntry : public wxPGCell
{
public:
    wxPGChoiceEntry();
    wxPGChoiceEntry(const wxPGChoiceEntry& other);
    wxPGChoiceEntry(const wxString& label, int value = wxPG_INVALID_VALUE);

    virtual ~wxPGChoiceEntry() = default;

    void SetValue( int value ) { m_value = value; }
    int GetValue() const { return m_value; }

    wxPGChoiceEntry& operator=(const wxPGChoiceEntry& other);

protected:
    int m_value;
};


typedef void* wxPGChoicesId;

class WXDLLIMPEXP_PROPGRID wxPGChoicesData : public wxObjectRefData
{
    friend class wxPGChoices;
public:
    // Constructor sets m_refCount to 1.
    wxPGChoicesData() = default;

    void CopyDataFrom( wxPGChoicesData* data );

    wxPGChoiceEntry& Insert( int index, const wxPGChoiceEntry& item );

    // Delete all entries
    void Clear();

    unsigned int GetCount() const
    {
        return (unsigned int) m_items.size();
    }

    const wxPGChoiceEntry& Item( unsigned int i ) const
    {
        wxASSERT_MSG( i < GetCount(), wxS("invalid index") );
        return m_items[i];
    }

    wxPGChoiceEntry& Item( unsigned int i )
    {
        wxASSERT_MSG( i < GetCount(), wxS("invalid index") );
        return m_items[i];
    }

private:
    std::vector<wxPGChoiceEntry> m_items;

protected:
    virtual ~wxPGChoicesData();
};

constexpr wxPGChoicesData* wxPGChoicesEmptyData = nullptr;


// Helper class for managing choices of wxPropertyGrid properties.
// Each entry can have label, value, bitmap, text colour, and background
// colour.
// wxPGChoices uses reference counting, similar to other wxWidgets classes.
// This means that assignment operator and copy constructor only copy the
// reference and not the actual data. Use Copy() member function to create
// a real copy.
// If you do not specify value for entry, index is used.
class WXDLLIMPEXP_PROPGRID wxPGChoices
{
public:
    typedef long ValArrItem;

    // Default constructor.
    wxPGChoices()
    {
        Init();
    }

    // Copy constructor, uses reference counting. To create a real copy,
    // use Copy() member function instead.
    wxPGChoices( const wxPGChoices& a )
    {
        if ( a.m_data != wxPGChoicesEmptyData )
        {
            m_data = a.m_data;
            m_data->IncRef();
        }
        else
        {
            Init();
        }
    }

    // Constructor.
    // count - Number of labels.
    // labels - Labels themselves.
    // values - Values for choices. If nullptr, indexes are used.
    wxPGChoices(size_t count, const wxString* labels, const long* values = nullptr)
    {
        Init();
        Add(count, labels, values);
    }

    // Constructor overload taking wxChar strings, provided mostly for
    // compatibility.
    // labels - Labels for choices, nullptr-terminated.
    // values - Values for choices. If nullptr, indexes are used.
    wxPGChoices( const wxChar* const* labels, const long* values = nullptr )
    {
        Init();
        Add(labels,values);
    }

    // Constructor.
    // labels - Labels for choices.
    // values - Values for choices. If empty, indexes are used.
    wxPGChoices( const wxArrayString& labels,
                 const wxArrayInt& values = wxArrayInt() )
    {
        Init();
        Add(labels,values);
    }

    // Simple interface constructor.
    wxPGChoices( wxPGChoicesData* data )
        : m_data(data)
    {
        wxCHECK_RET(data, "Data pointer cannot be null");
        m_data->IncRef();
    }

    // Destructor.
    ~wxPGChoices()
    {
        Free();
    }

    // Adds to current.
    // If did not have own copies, creates them now. If was empty, identical
    // to set except that creates copies.
    void Add(size_t count, const wxString* labels, const long* values = nullptr);

    // Overload taking wxChar strings, provided mostly for compatibility.
    // labels - Labels for added choices, nullptr-terminated.
    // values - Values for added choices. If empty, relevant entry indexes are used.
    void Add( const wxChar* const* labels, const ValArrItem* values = nullptr );

    // Version that works with wxArrayString and wxArrayInt.
    void Add( const wxArrayString& arr, const wxArrayInt& arrint = wxArrayInt() );

    // Adds a single choice.
    // label - Label for added choice.
    // value - Value for added choice. If unspecified, index is used.
    wxPGChoiceEntry& Add( const wxString& label,
                          int value = wxPG_INVALID_VALUE );

    // Adds a single item, with bitmap.
    wxPGChoiceEntry& Add( const wxString& label,
                          const wxBitmapBundle& bitmap,
                          int value = wxPG_INVALID_VALUE );

    // Adds a single item with full entry information.
    wxPGChoiceEntry& Add( const wxPGChoiceEntry& entry )
    {
        return Insert(entry, -1);
    }

    // Adds a single item, sorted.
    wxPGChoiceEntry& AddAsSorted( const wxString& label,
                                  int value = wxPG_INVALID_VALUE );

    // Assigns choices data, using reference counting. To create a real copy,
    // use Copy() member function instead.
    void Assign( const wxPGChoices& a )
    {
        AssignData(a.m_data);
    }

    // Assigns data from another set of choices.
    void AssignData( wxPGChoicesData* data );

    // Delete all choices.
    void Clear();

    // Returns a real copy of the choices.
    wxPGChoices Copy() const
    {
        wxPGChoices dst;
        dst.EnsureData();
        dst.m_data->CopyDataFrom(m_data);
        return dst;
    }

    void EnsureData()
    {
        if ( m_data == wxPGChoicesEmptyData )
            m_data = new wxPGChoicesData();
    }

    // Gets a unsigned number identifying this list.
    wxPGChoicesId GetId() const { return (wxPGChoicesId) m_data; }

    // Returns label of item.
    const wxString& GetLabel( unsigned int ind ) const
    {
        return Item(ind).GetText();
    }

    // Returns number of items.
    unsigned int GetCount () const
    {
        return m_data ? m_data->GetCount() : 0;
    }

    // Returns value of item.
    int GetValue( unsigned int ind ) const { return Item(ind).GetValue(); }

    // Returns array of values matching the given strings. Unmatching strings
    // result in wxPG_INVALID_VALUE entry in array.
    wxArrayInt GetValuesForStrings( const wxArrayString& strings ) const;

    // Returns array of indices matching given strings. Unmatching strings
    // are added to 'unmatched', if not null.
    wxArrayInt GetIndicesForStrings( const wxArrayString& strings,
                                     wxArrayString* unmatched = nullptr ) const;

    // Returns index of item with given label.
    int Index( const wxString& str ) const;
    // Returns index of item with given value.
    int Index( int val ) const;

    // Inserts a single item.
    wxPGChoiceEntry& Insert( const wxString& label,
                             int index,
                             int value = wxPG_INVALID_VALUE );

    // Inserts a single item with full entry information.
    wxPGChoiceEntry& Insert( const wxPGChoiceEntry& entry, int index );

    // Returns false if this is a constant empty set of choices,
    // which should not be modified.
    bool IsOk() const
    {
        return ( m_data != wxPGChoicesEmptyData );
    }

    const wxPGChoiceEntry& Item( unsigned int i ) const
    {
        wxASSERT( IsOk() );
        return m_data->Item(i);
    }

    // Returns item at given index.
    wxPGChoiceEntry& Item( unsigned int i )
    {
        wxASSERT( IsOk() );
        return m_data->Item(i);
    }

    // Removes count items starting at position nIndex.
    void RemoveAt(size_t nIndex, size_t count = 1);

    // Sets contents from lists of strings and values.
    // Does not create copies for itself.
    // TODO: Deprecate.
    void Set(size_t count, const wxString* labels, const long* values = nullptr)
    {
        Free();
        Add(count, labels, values);
    }

    void Set( const wxChar* const* labels, const long* values = nullptr )
    {
        Free();
        Add(labels,values);
    }

    // Sets contents from lists of strings and values.
    // Version that works with wxArrayString and wxArrayInt.
    void Set( const wxArrayString& labels,
              const wxArrayInt& values = wxArrayInt() )
    {
        Free();
        Add(labels,values);
    }

    // Creates exclusive copy of current choices
    void AllocExclusive();

    // Returns data, increases refcount.
    wxPGChoicesData* GetData()
    {
        wxASSERT( m_data->GetRefCount() != -1 );
        m_data->IncRef();
        return m_data;
    }

    // Returns plain data ptr - no refcounting stuff is done.
    wxPGChoicesData* GetDataPtr() const { return m_data; }

    // Changes ownership of data to you.
    wxPGChoicesData* ExtractData()
    {
        wxPGChoicesData* data = m_data;
        m_data = wxPGChoicesEmptyData;
        return data;
    }

    // Returns array of choice labels.
    wxArrayString GetLabels() const;

    void operator= (const wxPGChoices& a)
    {
        if (this != &a)
            AssignData(a.m_data);
    }

    wxPGChoiceEntry& operator[](unsigned int i)
    {
        return Item(i);
    }

    const wxPGChoiceEntry& operator[](unsigned int i) const
    {
        return Item(i);
    }

protected:
    wxPGChoicesData*    m_data;

    void Init();
    void Free();
};

// -----------------------------------------------------------------------

// wxPGProperty is base class for all wxPropertyGrid properties.
class WXDLLIMPEXP_PROPGRID wxPGProperty : public wxObject
{
    friend class wxPropertyGrid;
    friend class wxPropertyGridInterface;
    friend class wxPropertyGridPageState;
    friend class wxPropertyGridPopulator;

    wxDECLARE_ABSTRACT_CLASS(wxPGProperty);
public:
#if WXWIN_COMPATIBILITY_3_0
    typedef wxUint32 FlagType;
#elif WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use wxPGFlags type instead")
    typedef wxUint32 FlagType;
#endif // WXWIN_COMPATIBILITY_3_0, WXWIN_COMPATIBILITY_3_2

    // Virtual destructor.
    // It is customary for derived properties to implement this.
    virtual ~wxPGProperty();

    // This virtual function is called after m_value has been set.
    // Remarks:
    // - If m_value was set to Null variant (i.e. unspecified value),
    //   OnSetValue() will not be called.
    // - m_value may be of any variant type. Typically properties internally
    //   support only one variant type, and as such OnSetValue() provides a
    //   good opportunity to convert
    //   supported values into internal type.
    // - Default implementation does nothing.
    virtual void OnSetValue();

    // Override this to return something else than m_value as the value.
    virtual wxVariant DoGetValue() const { return m_value; }

    // Implement this function in derived class to check the value.
    // Return true if it is ok. Returning false prevents property change
    // events from occurring.
    // Remark: Default implementation always returns true.
    virtual bool ValidateValue( wxVariant& value,
                                wxPGValidationInfo& validationInfo ) const;

    // Converts text into wxVariant value appropriate for this property.
    // Parameters:
    // variant - On function entry this is the old value (should not be
    //   null wxVariant in normal cases). Translated value must be assigned
    //   back to it.
    // text - Text to be translated into variant.
    // flags - If wxPGPropValFormatFlags::FullValue is set, returns complete, storable value instead
    //   of displayable one (they may be different).
    //   If wxPGPropValFormatFlags::CompositeFragment is set, text is interpreted as a part of
    //   composite property string value (as generated by ValueToString()
    //   called with this same flag).
    // Returns true if resulting wxVariant value was different.
    // Default implementation converts semicolon delimited tokens into
    // child values. Only works for properties with children.
    // You might want to take into account that m_value is Null variant
    // if property value is unspecified (which is usually only case if
    // you explicitly enabled that sort behaviour).
#if WXWIN_COMPATIBILITY_3_2
    mutable bool m_oldStringToValueCalled = false;
    bool StringToValueWithCheck(wxVariant& variant, const wxString& text, wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;
    wxDEPRECATED_BUT_USED_INTERNALLY_MSG("use StringToValue with 'flags' argument as wxPGPropValFormatFlags")
    virtual bool StringToValue( wxVariant& variant, const wxString& text,
                                int flags ) const
    {
        m_oldStringToValueCalled = true;
        return StringToValue(variant, text, static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    virtual bool StringToValue(wxVariant& variant, const wxString& text,
                               wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;

    // Converts integer (possibly a choice selection) into wxVariant value
    // appropriate for this property.
    // Parameters:
    // variant - On function entry this is the old value (should not be null wxVariant
    //   in normal cases). Translated value must be assigned back to it.
    // number - Integer to be translated into variant.
    // flags - If wxPGPropValFormatFlags::FullValue is set, returns complete, storable value
    //   instead of displayable one.
    // Returns true if resulting wxVariant value was different.
    // Remarks
    // - If property is not supposed to use choice or spinctrl or other editor
    //   with int-based value, it is not necessary to implement this method.
    // - Default implementation simply assign given int to m_value.
    // - If property uses choice control, and displays a dialog on some choice
    //   items, then it is preferred to display that dialog in IntToValue
    //   instead of OnEvent.
    // - You might want to take into account that m_value is Null variant
    //   if property value is unspecified (which is usually only case if
    //   you explicitly enabled that sort behaviour).
#if WXWIN_COMPATIBILITY_3_2
    mutable bool m_oldIntToValueCalled = false;
    bool IntToValueWithCheck(wxVariant& variant, int number, wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;
    wxDEPRECATED_BUT_USED_INTERNALLY_MSG("use IntToValue with 'flags' argument as wxPGPropValFormatFlags")
    virtual bool IntToValue(wxVariant& value, int number, int flags) const
    {
        m_oldIntToValueCalled = true;
        return IntToValue(value, number, static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    virtual bool IntToValue(wxVariant& value, int number,
                            wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;

    // Converts property value into a text representation.
    // Parameters:
    // value - Value to be converted.
    // flags - If wxPGPropValFormatFlags::Null (default value), then displayed string is returned.
    //   If wxPGPropValFormatFlags::FullValue is set, returns complete, storable string value
    //   instead of displayable. If wxPGPropValFormatFlags::EditableValue is set, returns
    //   string value that must be editable in textctrl. If
    //   wxPGPropValFormatFlags::CompositeFragment is set, returns text that is appropriate to
    //   display as a part of string property's composite text
    //   representation.
    // Default implementation calls GenerateComposedValue().
#if WXWIN_COMPATIBILITY_3_2
    mutable bool m_oldValueToStringCalled = false;
    wxString ValueToStringWithCheck(wxVariant& variant, wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;
    wxDEPRECATED_BUT_USED_INTERNALLY_MSG("use ValueToString with 'flags' argument as wxPGPropValFormatFlags")
    virtual wxString ValueToString(wxVariant& value, int flags) const
    {
        m_oldValueToStringCalled = true;
        return ValueToString(value, static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    virtual wxString ValueToString(wxVariant& value, wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;

    // Converts string to a value, and if successful, calls SetValue() on it.
    // Default behaviour is to do nothing.
    // Returns true if value was changed.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetValueFromString with 'flags' argument as wxPGPropValFormatFlags")
    bool SetValueFromString(const wxString& text, int flags)
    {
        return SetValueFromString(text, static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    bool SetValueFromString(const wxString& text, wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::ProgrammaticValue);

    // Converts integer to a value, and if successful, calls SetValue() on it.
    // Default behaviour is to do nothing.
    // Parameters:
    //   value - Int to get the value from.
    //   flags - If has wxPGPropValFormatFlags::FullValue, then the value given is a actual value
    //     and not an index.
    // Returns true if value was changed.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetValueFromInt with 'flags' argument as wxPGPropValFormatFlags")
    bool SetValueFromInt(long value, int flags)
    {
        return SetValueFromInt(value, static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    bool SetValueFromInt(long value, wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null);

    // Returns size of the custom painted image in front of property.
    // This method must be overridden to return non-default value if
    // OnCustomPaint is to be called.
    // item - Normally -1, but can be an index to the property's list of items.
    // Remarks:
    // - Default behaviour is to return wxSize(0,0), which means no image.
    // - Default image width or height is indicated with dimension -1.
    // - You can also return wxPG_DEFAULT_IMAGE_SIZE, i.e. wxDefaultSize.
    virtual wxSize OnMeasureImage( int item = -1 ) const;

    // Events received by editor widgets are processed here.
    // Note that editor class usually processes most events. Some, such as
    // button press events of TextCtrlAndButton class, can be handled here.
    // Also, if custom handling for regular events is desired, then that can
    // also be done (for example, wxSystemColourProperty custom handles
    // wxEVT_CHOICE to display colour picker dialog when
    // 'custom' selection is made).
    // If the event causes value to be changed, SetValueInEvent()
    // should be called to set the new value.
    // event - Associated wxEvent.
    // Should return true if any changes in value should be reported.
    // If property uses choice control, and displays a dialog on some choice
    // items, then it is preferred to display that dialog in IntToValue
    // instead of OnEvent.
    virtual bool OnEvent( wxPropertyGrid* propgrid,
                          wxWindow* wnd_primary,
                          wxEvent& event );

    // Called after value of a child property has been altered. Must return
    // new value of the whole property (after any alterations warranted by
    // child's new value).
    // Note that this function is usually called at the time that value of
    // this property, or given child property, is still pending for change,
    // and as such, result of GetValue() or m_value should not be relied
    // on.
    // Parameters:
    //   thisValue - Value of this property. Changed value should be returned
    //     (in previous versions of wxPropertyGrid it was only necessary to
    //     write value back to this argument).
    //   childIndex - Index of child changed (you can use Item(childIndex)
    //     to get child property).
    //   childValue - (Pending) value of the child property.
    // Returns modified value of the whole property.
    virtual wxVariant ChildChanged( wxVariant& thisValue,
                                    int childIndex,
                                    wxVariant& childValue ) const;

    // Returns pointer to an instance of used editor.
    virtual const wxPGEditor* DoGetEditorClass() const;

    // Returns pointer to the wxValidator that should be used
    // with the editor of this property (nullptr for no validator).
    // Setting validator explicitly via SetPropertyValidator
    // will override this.
    // You can get common filename validator by returning
    // wxFileProperty::GetClassValidator(). wxDirProperty,
    // for example, uses it.
    virtual wxValidator* DoGetValidator () const;

    // Override to paint an image in front of the property value text or
    // drop-down list item (but only if wxPGProperty::OnMeasureImage is
    // overridden as well).
    // If property's OnMeasureImage() returns size that has height != 0 but
    // less than row height ( < 0 has special meanings), wxPropertyGrid calls
    // this method to draw a custom image in a limited area in front of the
    // editor control or value text/graphics, and if control has drop-down
    // list, then the image is drawn there as well (even in the case
    // OnMeasureImage() returned higher height than row height).
    // NOTE: Following applies when OnMeasureImage() returns a "flexible"
    // height ( using wxPG_FLEXIBLE_SIZE(W,H) macro), which implies variable
    // height items: If rect.x is < 0, then this is a measure item call, which
    // means that dc is invalid and only thing that should be done is to set
    // paintdata.m_drawnHeight to the height of the image of item at index
    // paintdata.m_choiceItem. This call may be done even as often as once
    // every drop-down popup show.
    // Parameters:
    //   dc - wxDC to paint on.
    //   rect - Box reserved for custom graphics. Includes surrounding rectangle,
    //     if any. If x is < 0, then this is a measure item call (see above).
    //   paintdata - wxPGPaintData structure with much useful data.
    // Remarks:
    // - You can actually exceed rect width, but if you do so then
    //   paintdata.m_drawnWidth must be set to the full width drawn in
    //   pixels.
    // - Due to technical reasons, rect's height will be default even if
    //   custom height was reported during measure call.
    // - Brush is guaranteed to be default background colour. It has been
    //   already used to clear the background of area being painted. It
    //   can be modified.
    // - Pen is guaranteed to be 1-wide 'black' (or whatever is the proper
    //   colour) pen for drawing framing rectangle. It can be changed as
    //   well.
    // See ValueToString()
    virtual void OnCustomPaint( wxDC& dc,
                                const wxRect& rect,
                                wxPGPaintData& paintdata );

    // Returns used wxPGCellRenderer instance for given property column
    // (label=0, value=1).
    // Default implementation returns editor's renderer for all columns.
    virtual wxPGCellRenderer* GetCellRenderer( int column ) const;

    // Returns which choice is currently selected. Only applies to properties
    // which have choices.
    // Needs to be reimplemented in derived class if property value does not
    // map directly to a choice. Integer as index, bool, and string usually do.
    virtual int GetChoiceSelection() const;

    // Refresh values of child properties.
    // Automatically called after value is set.
    virtual void RefreshChildren();

    // Reimplement this member function to add special handling for
    // attributes of this property.
    // Return false to have the attribute automatically stored in
    // m_attributes. Default implementation simply does that and
    // nothing else.
    // To actually set property attribute values from the
    // application, use wxPGProperty::SetAttribute() instead.
    virtual bool DoSetAttribute( const wxString& name, wxVariant& value );

    // Returns value of an attribute.
    // Override if custom handling of attributes is needed.
    // Default implementation simply return nullptr variant.
    virtual wxVariant DoGetAttribute( const wxString& name ) const;

    // Returns instance of a new wxPGEditorDialogAdapter instance, which is
    // used when user presses the (optional) button next to the editor control;
    // Default implementation returns nullptr (ie. no action is generated when
    // button is pressed).
    virtual wxPGEditorDialogAdapter* GetEditorDialog() const;

    // Called whenever validation has failed with given pending value.
    // If you implement this in your custom property class, please
    // remember to call the baser implementation as well, since they
    // may use it to revert property into pre-change state.
    virtual void OnValidationFailure( wxVariant& pendingValue );

    // Append a new choice to property's list of choices.
    int AddChoice( const wxString& label, int value = wxPG_INVALID_VALUE )
    {
        return InsertChoice(label, wxNOT_FOUND, value);
    }

    // Returns true if children of this property are component values (for
    // instance, points size, face name, and is_underlined are component
    // values of a font).
    bool AreChildrenComponents() const
    {
        return !!(m_flags & (wxPGFlags::ComposedValue|wxPGFlags::Aggregate));
    }

    // Deletes children of the property.
    void DeleteChildren();

    // Removes entry from property's wxPGChoices and editor control (if it is
    // active).
    // If selected item is deleted, then the value is set to unspecified.
    void DeleteChoice( int index );

    // Enables or disables the property. Disabled property usually appears
    // as having grey text.
    // See wxPropertyGridInterface::EnableProperty()
    void Enable( bool enable = true );

    // Call to enable or disable usage of common value (integer value that can
    // be selected for properties instead of their normal values) for this
    // property.
    // Common values are disabled by the default for all properties.
    void EnableCommonValue( bool enable = true )
    {
        ChangeFlag(wxPGFlags::UsesCommonValue, enable);
    }

    // Composes text from values of child properties.
    wxString GenerateComposedValue() const
    {
        wxString s;
        DoGenerateComposedValue(s);
        return s;
    }

    // Returns property's label.
    const wxString& GetLabel() const { return m_label; }

    // Returns property's name with all (non-category, non-root) parents.
    wxString GetName() const;

    // Returns property's base name (i.e. parent's name is not added
    // in any case).
    const wxString& GetBaseName() const { return m_name; }

    // Returns read-only reference to property's list of choices.
    const wxPGChoices& GetChoices() const
    {
        return m_choices;
    }

    // Returns coordinate to the top y of the property. Note that the
    // position of scrollbars is not taken into account.
    int GetY() const;

    // Returns property's value.
    wxVariant GetValue() const
    {
        return DoGetValue();
    }

    // Returns reference to the internal stored value. GetValue is preferred
    // way to get the actual value, since GetValueRef ignores DoGetValue,
    // which may override stored value.
    wxVariant& GetValueRef()
    {
        return m_value;
    }

    const wxVariant& GetValueRef() const
    {
        return m_value;
    }

    // Helper function (for wxPython bindings and such) for settings protected
    // m_value.
    wxVariant GetValuePlain() const
    {
        return m_value;
    }

    // Returns text representation of property's value.
    // flags - If wxPGPropValFormatFlags::Null (default value), then displayed string is returned.
    //   If wxPGPropValFormatFlags::FullValue is set, returns complete, storable string value
    //   instead of displayable. If wxPGPropValFormatFlags::EditableValue is set, returns
    //   string value that must be editable in textctrl. If
    //   wxPGPropValFormatFlags::CompositeFragment is set, returns text that is appropriate to
    //   display as a part of string property's composite text
    //   representation.
    // In older versions, this function used to be overridden to convert
    // property's value into a string representation. This function is
    // now handled by ValueToString(), and overriding this function now
    // will result in run-time assertion failure.
#if WXWIN_COMPATIBILITY_3_2
    mutable bool m_oldGetValueAsString = false;
    wxString GetValueAsStringWithCheck(wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;
    wxDEPRECATED_BUT_USED_INTERNALLY_MSG("use GetValueAsString with 'flags' argument as wxPGPropValFormatFlags")
    virtual wxString GetValueAsString(int flags) const
    {
        m_oldGetValueAsString = true;
        return GetValueAsString(static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    virtual wxString GetValueAsString(wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const;

    // Returns wxPGCell of given column.
    // Const version of this member function returns 'default'
    // wxPGCell object if the property itself didn't hold
    // cell data.
    const wxPGCell& GetCell( unsigned int column ) const;

    // Returns wxPGCell of given column, creating one if necessary.
    wxPGCell& GetCell( unsigned int column )
    {
        return GetOrCreateCell(column);
    }

    // Returns wxPGCell of given column, creating one if necessary.
    wxPGCell& GetOrCreateCell( unsigned int column );

    // Return number of displayed common values for this property.
    int GetDisplayedCommonValueCount() const;

    // Returns property's displayed text.
    wxString GetDisplayedString() const
    {
#if WXWIN_COMPATIBILITY_3_2
        // Special implementation with check if user-overriden obsolete function is still in use
        return GetValueAsStringWithCheck(wxPGPropValFormatFlags::Null);
#else
        return GetValueAsString(wxPGPropValFormatFlags::Null);
#endif // WXWIN_COMPATIBILITY_3_2 | !WXWIN_COMPATIBILITY_3_2
    }

    // Returns property's hint text (shown in empty value cell).
    wxString GetHintText() const;

    // Returns property grid where property lies.
    wxPropertyGrid* GetGrid() const;

    // Returns owner wxPropertyGrid, but only if one is currently
    // on a page displaying this property.
    wxPropertyGrid* GetGridIfDisplayed() const;

    // Returns highest level non-category, non-root parent. Useful when you
    // have nested properties with children.
    // Thus, if immediate parent is root or category, this will return the
    // property itself.
    wxPGProperty* GetMainParent() const;

    // Return parent of property.
    wxPGProperty* GetParent() const { return m_parent; }

    // Returns true if property has editable wxTextCtrl when selected.
    // Although disabled properties do not displayed editor, they still
    // Returns true here as being disabled is considered a temporary
    // condition (unlike being read-only or having limited editing enabled).
    bool IsTextEditable() const;

    // Returns true if property's value is considered unspecified.
    // This usually means that value is Null variant.
    bool IsValueUnspecified() const
    {
        return m_value.IsNull();
    }

#if WXWIN_COMPATIBILITY_3_0
    // Returns non-zero if property has given flag set.
    wxDEPRECATED_MSG("use HasFlag() with 'flag' argument as wxPGFlags")
    FlagType HasFlag( FlagType flag ) const
    {
        return ( static_cast<FlagType>(m_flags) & flag );
    }
#endif
    // Returns true if property has given flag set.
    bool HasFlag(wxPGFlags flag) const
    {
        return !!(m_flags & flag);
    }
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use HasFlag() with 'flag' argument as wxPGFlags")
    // Returns true if property has given flag set.
    bool HasFlag(int flag) const
    {
        return HasFlag(static_cast<wxPGFlags>(flag));
    }
#endif // WXWIN_COMPATIBILITY_3_2

    // Returns true if property has all given flags set.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use HasFlagExact() with 'flags' argument as wxPGFlags")
    bool HasFlagsExact(int flags) const
    {
        return HasFlagsExact(static_cast<wxPGFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    bool HasFlagsExact(wxPGFlags flags) const
    {
        return (m_flags & flags) == flags;
    }

    // Returns comma-delimited string of property attributes.
    const wxPGAttributeStorage& GetAttributes() const
    {
        return m_attributes;
    }

    // Returns m_attributes as list wxVariant.
    wxVariant GetAttributesAsList() const;

#if WXWIN_COMPATIBILITY_3_0
    // Returns property flags.
    wxDEPRECATED_MSG("Use HasFlag or HasFlagsExact functions instead.")
    FlagType GetFlags() const
    {
        return static_cast<FlagType>(m_flags);
    }
#endif

    // Returns wxPGEditor that will be used and created when
    // property becomes selected. Returns more accurate value
    // than DoGetEditorClass().
    const wxPGEditor* GetEditorClass() const;

    // Returns value type used by this property.
    wxString GetValueType() const
    {
        return m_value.GetType();
    }

    // Returns editor used for given column. nullptr for no editor.
    const wxPGEditor* GetColumnEditor( int column ) const
    {
        return column == 1 ? GetEditorClass() : nullptr;
    }

    // Returns common value selected for this property. -1 for none.
    int GetCommonValue() const
    {
        return m_commonValue;
    }

    // Returns true if property has even one visible child.
    bool HasVisibleChildren() const;

    // Use this member function to add independent (i.e. regular) children to
    // a property.
    // Returns inserted childProperty.
    // wxPropertyGrid is not automatically refreshed by this function.
    wxPGProperty* InsertChild( int index, wxPGProperty* childProperty );

    // Inserts a new choice to property's list of choices.
    int InsertChoice( const wxString& label, int index, int value = wxPG_INVALID_VALUE );

    // Returns true if this property is actually a wxPropertyCategory.
    bool IsCategory() const { return !!(m_flags & wxPGFlags::Category); }

    // Returns true if this property is actually a wxRootProperty.
    bool IsRoot() const { return (m_parent == nullptr); }

    // Returns true if this is a sub-property.
    bool IsSubProperty() const
    {
        wxPGProperty* parent = m_parent;
        if ( parent && !parent->IsCategory() )
            return true;
        return false;
    }

    // Returns last visible sub-property, recursively.
    const wxPGProperty* GetLastVisibleSubItem() const;

    // Returns property's default value. If property's value type is not
    // a built-in one, and "DefaultValue" attribute is not defined, then
    // this function usually returns Null variant.
    wxVariant GetDefaultValue() const;

    // Returns maximum allowed length of the text the user can enter in
    // the property text editor.
    int GetMaxLength() const
    {
        return m_maxLen;
    }

    // Determines, recursively, if all children are not unspecified.
    // pendingList - Assumes members in this wxVariant list as pending
    //   replacement values.
    bool AreAllChildrenSpecified( const wxVariant* pendingList = nullptr ) const;

    // Updates composed values of parent non-category properties, recursively.
    // Returns topmost property updated.
    // Must not call SetValue() (as can be called in it).
    wxPGProperty* UpdateParentValues();

    // Returns true if containing grid uses wxPG_EX_AUTO_UNSPECIFIED_VALUES.
    bool UsesAutoUnspecified() const
    {
        return !!(m_flags & wxPGFlags::AutoUnspecified);
    }

    // Returns bitmap that appears next to value text. Only returns non-null
    // bitmap if one was set with SetValueImage().
    wxBitmap* GetValueImage() const;

    // Returns property attribute value, null variant if not found.
    wxVariant GetAttribute( const wxString& name ) const;

    // Returns named attribute, as string, if found.
    // Otherwise defVal is returned.
    wxString GetAttribute( const wxString& name, const wxString& defVal ) const;

    // Returns named attribute, as long, if found.
    // Otherwise defVal is returned.
    long GetAttributeAsLong( const wxString& name, long defVal ) const;

    // Returns named attribute, as double, if found.
    // Otherwise defVal is returned.
    double GetAttributeAsDouble( const wxString& name, double defVal ) const;

    unsigned int GetDepth() const { return (unsigned int)m_depth; }

    // Gets flags as a'|' delimited string. Note that flag names are not
    // prepended with 'wxPGFlags'.
    // flagmask - String will only be made to include flags combined by this parameter.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use GetFlagsAsString() with 'flags' argument as wxPGFlags")
    wxString GetFlagsAsString( int flagsMask ) const
    {
        return GetFlagsAsString(static_cast<wxPGFlags>(flagsMask));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    wxString GetFlagsAsString(wxPGFlags flagsMask) const;

    // Returns position in parent's array.
    unsigned int GetIndexInParent() const
    {
        return m_arrIndex;
    }

    // Hides or reveals the property.
    // hide - true for hide, false for reveal.
    // flags - By default changes are applied recursively. Set this
    //   parameter to wxPGPropertyValuesFlags::DontRecurse to prevent this.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use Hide with flags argument as wxPGPropertyValuesFlags")
    bool Hide(bool hide, int flags)
    {
        return Hide(hide, static_cast<wxPGPropertyValuesFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    bool Hide(bool hide, wxPGPropertyValuesFlags = wxPGPropertyValuesFlags::Recurse);

    // Returns true if property has visible children.
    bool IsExpanded() const
        { return (!(m_flags & wxPGFlags::Collapsed) && HasAnyChild()); }

    // Returns true if all parents expanded.
    bool IsVisible() const;

    // Returns true if property is enabled.
    bool IsEnabled() const { return !(m_flags & wxPGFlags::Disabled); }

    // If property's editor is created this forces its recreation.
    // Useful in SetAttribute etc. Returns true if actually did anything.
    bool RecreateEditor();

    // If property's editor is active, then update its value.
    void RefreshEditor();

    // Sets an attribute for this property.
    // name - Text identifier of attribute. See @ref propgrid_property_attributes.
    // value - Value of attribute.
    // Setting attribute's value to Null variant will simply remove it
    // from property's set of attributes.
    void SetAttribute( const wxString& name, wxVariant value );

    void SetAttributes( const wxPGAttributeStorage& attributes );

    // Set if user can change the property's value to unspecified by
    // modifying the value of the editor control (usually by clearing
    // it).  Currently, this can work with following properties:
    // wxIntProperty, wxUIntProperty, wxFloatProperty, wxEditEnumProperty.
    // enable - Whether to enable or disable this behaviour (it is disabled
    // by default).
    void SetAutoUnspecified( bool enable = true )
    {
        ChangeFlag(wxPGFlags::AutoUnspecified, enable);
    }

    // Sets property's background colour.
    // colour - Background colour to use.
    // flags - Default is wxPGPropertyValuesFlags::Recurse which causes colour to be set recursively.
    //   Omit this flag to only set colour for the property in question
    //   and not any of its children.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetBackgroundColour with flags argument as wxPGPropertyValuesFlags")
    void SetBackgroundColour(const wxColour& colour, int flags)
    {
        SetBackgroundColour(colour, static_cast<wxPGPropertyValuesFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void SetBackgroundColour(const wxColour& colour,
                             wxPGPropertyValuesFlags flags = wxPGPropertyValuesFlags::Recurse);

    // Sets property's text colour.
    // colour - Text colour to use.
    // flags - Default is wxPGPropertyValuesFlags::Recurse which causes colour to be set recursively.
    // Omit this flag to only set colour for the property in question
    // and not any of its children.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetTextColour with flags argument as wxPGPropertyValuesFlags")
    void SetTextColour(const wxColour& colour, int flags)
    {
        SetTextColour(colour, static_cast<wxPGPropertyValuesFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void SetTextColour(const wxColour& colour,
                       wxPGPropertyValuesFlags flags = wxPGPropertyValuesFlags::Recurse);

    // Sets property's default text and background colours.
    // flags - Default is wxPGPropertyValuesFlags::Recurse
    //   which causes colours to be set recursively.
    //   Omit this flag to only set colours for the property in question
    //   and not any of its children.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetDefaultColours with wxPGPropertyValuesFlags argument")
    void SetDefaultColours(int flags)
    {
        SetDefaultColours(static_cast<wxPGPropertyValuesFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void SetDefaultColours(wxPGPropertyValuesFlags flags = wxPGPropertyValuesFlags::Recurse);

    // Set default value of a property. Synonymous to
    // SetAttribute("DefaultValue", value);
    void SetDefaultValue( wxVariant& value );

    // Sets editor for a property.
    // editor - For builtin editors, use wxPGEditor_X, where X is builtin editor's
    //   name (TextCtrl, Choice, etc. see wxPGEditor documentation for full
    //   list).
    // For custom editors, use pointer you received from
    // wxPropertyGrid::RegisterEditorClass().
    void SetEditor( const wxPGEditor* editor )
    {
        m_customEditor = editor;
    }

    // Sets editor for a property, , by editor name.
    void SetEditor( const wxString& editorName );

    // Sets cell information for given column.
    void SetCell( int column, const wxPGCell& cell );

    // Sets common value selected for this property. -1 for none.
    void SetCommonValue( int commonValue )
    {
        m_commonValue = commonValue;
    }

    // Sets flags from a '|' delimited string. Note that flag names are not
    // prepended with 'wxPGFlags'.
    void SetFlagsFromString( const wxString& str );

    // Sets property's "is it modified?" flag. Affects children recursively.
    void SetModifiedStatus( bool modified )
    {
        SetFlagRecursively(wxPGFlags::Modified, modified);
    }

    // Call in OnEvent(), OnButtonClick() etc. to change the property value
    // based on user input.
    // This method is const since it doesn't actually modify value, but posts
    // given variant as pending value, stored in wxPropertyGrid.
    void SetValueInEvent( const wxVariant& value ) const;

    // Call this to set value of the property.
    // Unlike methods in wxPropertyGrid, this does not automatically update
    // the display.
    // Use wxPropertyGrid::ChangePropertyValue() instead if you need to run
    // through validation process and send property change event.
    // If you need to change property value in event, based on user input, use
    // SetValueInEvent() instead.
    // pList - Pointer to list variant that contains child values. Used to
    //   indicate which children should be marked as modified.
    // flags - Various flags (for instance, wxPGSetValueFlags::RefreshEditor,
    //  which is enabled by default).
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetValue with flags argument as wxPGSetValueFlags")
    void SetValue(wxVariant value, wxVariant* pList, int flags)
    {
        SetValue(value, pList, static_cast<wxPGSetValueFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void SetValue(wxVariant value, wxVariant* pList = nullptr,
                  wxPGSetValueFlags flags = wxPGSetValueFlags::RefreshEditor);

    // Set wxBitmap in front of the value. This bitmap may be ignored
    // by custom cell renderers.
    void SetValueImage( const wxBitmapBundle& bmp );

    // Sets selected choice and changes property value.
    // Tries to retain value type, although currently if it is not string,
    // then it is forced to integer.
    void SetChoiceSelection( int newValue );

    void SetExpanded( bool expanded )
    {
        ChangeFlag(wxPGFlags::Collapsed, !expanded);
    }

    // Sets or clears given property flag. Mainly for internal use.
    // Setting a property flag never has any side-effect, and is
    // intended almost exclusively for internal use. So, for
    // example, if you want to disable a property, call
    // Enable(false) instead of setting wxPGFlags::Disabled flag.
    void ChangeFlag( wxPGFlags flag, bool set )
    {
        if ( set )
            m_flags |= flag;
        else
            m_flags &= ~flag;
    }
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use ChangeFlag with flags argument as wxPGFlags")
    void ChangeFlag(wxPGPropertyFlags flags, bool set)
    {
        ChangeFlag(static_cast<wxPGFlags>(flags), set);
    }
#endif // WXWIN_COMPATIBILITY_3_2

    // Sets or clears given property flag, recursively. This function is
    // primarily intended for internal use.
    void SetFlagRecursively( wxPGFlags flag, bool set );
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetFlagRecursively with flags argument as wxPGFlags")
    void SetFlagRecursively(wxPGPropertyFlags flags, bool set)
    {
        ChangeFlag(static_cast<wxPGFlags>(flags), set);
    }
#endif // WXWIN_COMPATIBILITY_3_2

    // Sets property's help string, which is shown, for example, in
    // wxPropertyGridManager's description text box.
    void SetHelpString( const wxString& helpString )
    {
        m_helpString = helpString;
    }

    // Sets property's label.
    // Properties under same parent may have same labels. However,
    // property names must still remain unique.
    void SetLabel( const wxString& label );

    // Sets new (base) name for property.
    void SetName( const wxString& newName );

    // Changes what sort of parent this property is for its children.
    // flag - Use one of the following values: wxPGFlags::MiscParent (for
    //   generic parents), wxPGFlags::Category (for categories), or
    //   wxPGFlags::Aggregate (for derived property classes with private
    //   children).
    // You generally do not need to call this function.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetParentalType() with 'flag' argument as wxPGFlags")
    void SetParentalType(int flag)
    {
        SetParentalType(static_cast<wxPGFlags>(flag));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void SetParentalType(wxPGFlags flag)
    {
        m_flags &= ~(wxPGFlags::Property | wxPGFlags::ParentalFlags);
        m_flags |= flag;
    }

    // Sets property's value to unspecified (i.e. Null variant).
    void SetValueToUnspecified()
    {
        wxVariant val;  // Create null variant
        SetValue(val, nullptr, wxPGSetValueFlags::RefreshEditor);
    }

    // Helper function (for wxPython bindings and such) for settings protected
    // m_value.
    void SetValuePlain( const wxVariant& value )
    {
        m_value = value;
    }

#if wxUSE_VALIDATORS
    // Sets wxValidator for a property.
    void SetValidator( const wxValidator& validator )
    {
        m_validator = wxDynamicCast(validator.Clone(),wxValidator);
    }

    // Gets assignable version of property's validator.
    wxValidator* GetValidator() const
    {
        return  m_validator ? m_validator : DoGetValidator();
    }
#endif // wxUSE_VALIDATORS

    // Returns client data (void*) of a property.
    void* GetClientData() const
    {
        return m_clientData;
    }

    // Sets client data (void*) of a property.
    // This untyped client data has to be deleted manually.
    void SetClientData( void* clientData )
    {
        m_clientData = clientData;
    }

    // Sets client object of a property.
    void SetClientObject(wxClientData* clientObject)
    {
        delete m_clientObject;
        m_clientObject = clientObject;
    }

    // Gets managed client object of a property.
    wxClientData *GetClientObject() const { return m_clientObject; }

    // Sets new set of choices for the property.
    // This operation deselects the property and clears its
    // value.
    bool SetChoices( const wxPGChoices& choices );

    // Set max length of text in text editor.
    bool SetMaxLength( int maxLen );

    // Call with 'false' in OnSetValue to cancel value changes after all
    // (i.e. cancel 'true' returned by StringToValue() or IntToValue()).
    void SetWasModified( bool set = true )
    {
        ChangeFlag(wxPGFlags::WasModified, set);
    }

    // Returns property's help or description text.
    const wxString& GetHelpString() const
    {
        return m_helpString;
    }

    // Returns true if candidateParent is some parent of this property.
    // Use, for example, to detect if item is inside collapsed section.
    bool IsSomeParent( wxPGProperty* candidate_parent ) const;

    // Adapts list variant into proper value using consecutive
    // ChildChanged-calls.
    void AdaptListToValue( wxVariant& list, wxVariant* value ) const;

    // Adds a private child property. If you use this instead of
    // wxPropertyGridInterface::Insert() or
    // wxPropertyGridInterface::AppendIn(), then property's parental
    // type will automatically be set up to wxPGFlags::Aggregate. In other
    // words, all properties of this property will become private.
    void AddPrivateChild( wxPGProperty* prop );

    // Use this member function to add independent (i.e. regular) children to
    // a property.
    // wxPropertyGrid is not automatically refreshed by this function.
    wxPGProperty* AppendChild( wxPGProperty* prop )
    {
        return InsertChild(-1, prop);
    }

    // Returns height of children, recursively, and
    // by taking expanded/collapsed status into account.
    // lh - Line height. Pass result of GetGrid()->GetRowHeight() here.
    // iMax - Only used (internally) when finding property y-positions.
    int GetChildrenHeight( int lh, int iMax = -1 ) const;

    // Returns number of child properties.
    unsigned int GetChildCount() const
    {
        return (unsigned int) m_children.size();
    }

    // Checks if contains any child property.
    bool HasAnyChild() const
    {
        return !m_children.empty();
    }

    // Returns sub-property at index i.
    wxPGProperty* Item( unsigned int i ) const
        { return m_children[i]; }

    // Returns last sub-property.
    wxPGProperty* Last() const { return m_children.back(); }

    // Returns index of given child property. wxNOT_FOUND if
    // given property is not child of this.
    int Index( const wxPGProperty* p ) const;

    // Puts correct indexes to children
    void FixIndicesOfChildren( unsigned int starthere = 0 );

    // Converts image width into full image offset, with margins.
    int GetImageOffset( int imageWidth ) const;

    // Returns wxPropertyGridPageState in which this property resides.
    wxPropertyGridPageState* GetParentState() const { return m_parentState; }

    wxPGProperty* GetItemAtY( unsigned int y,
                              unsigned int lh,
                              unsigned int* nextItemY ) const;

    // Returns property at given virtual y coordinate.
    wxPGProperty* GetItemAtY( unsigned int y ) const;

    // Returns (direct) child property with given name (or nullptr if not found).
    wxPGProperty* GetPropertyByName( const wxString& name ) const;

    // Returns various display-related information for given column
#if WXWIN_COMPATIBILITY_3_0
    wxDEPRECATED_MSG("don't use GetDisplayInfo function with argument of 'const wxPGCell**' type. Use 'wxPGCell*' argument instead")
    void GetDisplayInfo( unsigned int column,
                         int choiceIndex,
                         int flags,
                         wxString* pString,
                         const wxPGCell** pCell );
#endif //  WXWIN_COMPATIBILITY_3_0
    // This function can return modified (customized) cell object.
    void GetDisplayInfo( unsigned int column,
                         int choiceIndex,
                         int flags,
                         wxString* pString,
                         wxPGCell* pCell );


#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_BUT_USED_INTERNALLY(static wxString* sm_wxPG_LABEL;)
#endif // WXWIN_COMPATIBILITY_3_2
    const static wxString       sm_labelItem;

    // This member is public so scripting language bindings
    // wrapper code can access it freely.
    void*                       m_clientData;

protected:

    // Ctors are ptotected because wxPGProperty is only a base class
    // for all property classes and shouldn't be instantiated directly.
    wxPGProperty();
    // All non-abstract property classes should have a constructor with
    // the same first two arguments as this one.
    wxPGProperty(const wxString& label, const wxString& name);

    // Sets property cell in fashion that reduces number of exclusive
    // copies of cell data. Used when setting, for instance, same
    // background colour for a number of properties.
    // firstCol - First column to affect.
    // lastCol- Last column to affect.
    // preparedCell - Pre-prepared cell that is used for those which cell data
    //   before this matched unmodCellData.
    // srcData - If unmodCellData did not match, valid cell data from this
    //   is merged into cell (usually generating new exclusive copy
    //   of cell's data).
    // unmodCellData - If cell's cell data matches this, its cell is now set to
    //   preparedCell.
    // ignoreWithFlags - Properties with any one of these flags are skipped.
    // recursively - If true, apply this operation recursively in child properties.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use AdaptiveSetCell() with 'ignoreWithFlags' argument as wxPGFlags")
    void AdaptiveSetCell( unsigned int firstCol,
                          unsigned int lastCol,
                          const wxPGCell& preparedCell,
                          const wxPGCell& srcData,
                          wxPGCellData* unmodCellData,
                          int ignoreWithFlags,
                          bool recursively )
    {
        AdaptiveSetCell(firstCol, lastCol, preparedCell, srcData, unmodCellData,
                        static_cast<wxPGFlags>(ignoreWithFlags), recursively);
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void AdaptiveSetCell(unsigned int firstCol,
                         unsigned int lastCol,
                         const wxPGCell& preparedCell,
                         const wxPGCell& srcData,
                         wxPGCellData* unmodCellData,
                         wxPGFlags ignoreWithFlags,
                         bool recursively);

    // Clear cells associated with property.
    // recursively - If true, apply this operation recursively in child properties.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use ClearCells() with 'ignoreWithFlags' argument as wxPGFlags")
    void ClearCells(int ignoreWithFlags, bool recursively)
    {
        ClearCells(static_cast<wxPGFlags>(ignoreWithFlags), recursively);
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void ClearCells(wxPGFlags ignoreWithFlags, bool recursively);

    // Makes sure m_cells has size of column+1 (or more).
    void EnsureCells( unsigned int column );

    // Returns (direct) child property with given name (or nullptr if not found),
    // with hint index.
    // hintIndex - Start looking for the child at this index.
    // Does not support scope (i.e. Parent.Child notation).
    wxPGProperty* GetPropertyByNameWH( const wxString& name,
                                       unsigned int hintIndex ) const;

    // This is used by Insert etc.
    void DoAddChild( wxPGProperty* prop,
                     int index = -1,
                     bool correct_mode = true );

#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use DoGenerateComposedValue with 'flags' argument as wxPGPropValFormatFlags")
    void DoGenerateComposedValue(wxString& text, int flags,
                                 const wxVariantList* valueOverrides,
                                 wxPGHashMapS2S* childResults) const
    {
        DoGenerateComposedValue(text, static_cast<wxPGPropValFormatFlags>(flags),
                                valueOverrides, childResults);
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void DoGenerateComposedValue(wxString& text,
                                 wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::ValueIsCurrent,
                                 const wxVariantList* valueOverrides = nullptr,
                                 wxPGHashMapS2S* childResults = nullptr) const;

    bool DoHide( bool hide, wxPGPropertyValuesFlags flags );

    void DoSetName(const wxString& str) { m_name = str; }

    // Deletes all sub-properties.
    void Empty();

    bool HasCell( unsigned int column ) const
    {
        return m_cells.size() > column;
    }

    void InitAfterAdded( wxPropertyGridPageState* pageState,
                         wxPropertyGrid* propgrid );

    // Returns true if child property is selected.
    bool IsChildSelected( bool recursive = false ) const;

    // Removes child property with given pointer. Does not delete it.
    void RemoveChild( wxPGProperty* p );

    // Removes child property at given index. Does not delete it.
    void RemoveChild(unsigned int index);

    // Sorts children using specified comparison function.
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("Don't use SortChildren function with argument of 'int (*)(wxPGProperty**, wxPGProperty**)' type. Use 'bool (*)(wxPGProperty*, wxPGProperty*)' argument instead")
    void SortChildren(int (*fCmp)(wxPGProperty**, wxPGProperty**));
#endif // WXWIN_COMPATIBILITY_3_2
    void SortChildren(bool (*fCmp)(wxPGProperty*, wxPGProperty*));

    void DoEnable( bool enable );

    void DoPreAddChild( int index, wxPGProperty* prop );

    void SetParentState( wxPropertyGridPageState* pstate )
        { m_parentState = pstate; }

    void SetFlag( wxPGFlags flag )
    {
        //
        // NB: While using wxPGFlags here makes it difficult to
        //     combine different flags, it usefully prevents user from
        //     using incorrect flags (say, wxWindow styles).
        m_flags |= flag;
    }
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use SetFlag() with 'flag' argument as wxPGFlags")
    void SetFlag( wxPGPropertyFlags flag )
    {
        SetFlag(static_cast<wxPGFlags>(flag));
    }
#endif // WXWIN_COMPATIBILITY_3_2

#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use ClearFlag() with 'flag' argument as wxPGFlags")
    void ClearFlag(wxPGPropertyFlags flag)
    {
        ClearFlag(static_cast<wxPGFlags>(flag));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    void ClearFlag(wxPGFlags flag)
    {
        m_flags &= ~(flag);
    }

    // Called when the property is being removed from the grid and/or
    // page state (but *not* when it is also deleted).
    void OnDetached(wxPropertyGridPageState* state,
                    wxPropertyGrid* propgrid);

    // Call after fixed sub-properties added/removed after creation.
    // if oldSelInd >= 0 and < new max items, then selection is
    // moved to it.
    void SubPropsChanged( int oldSelInd = -1 );

    int GetY2( int lh ) const;

    wxString                    m_label;
    wxString                    m_name;
    wxPGProperty*               m_parent;
    wxPropertyGridPageState*    m_parentState;

    wxClientData*               m_clientObject;

    // Overrides editor returned by property class
    const wxPGEditor*           m_customEditor;
#if wxUSE_VALIDATORS
    // Editor is going to get this validator
    wxValidator*                m_validator;
#endif
    // Show this in front of the value
    //
    // TODO: Can bitmap be implemented with wxPGCell?
    wxBitmapBundle              m_valueBitmapBundle;
    mutable wxBitmap            m_valueBitmap;

    wxVariant                   m_value;
    wxPGAttributeStorage        m_attributes;
    std::vector<wxPGProperty*>  m_children;

    // Extended cell information
    std::vector<wxPGCell>       m_cells;

    // Choices shown in drop-down list of editor control.
    wxPGChoices                 m_choices;

    // Help shown in statusbar or help box.
    wxString                    m_helpString;

    // Index in parent's property array.
    unsigned int                m_arrIndex;

    // If not -1, then overrides m_value
    int                         m_commonValue;

    wxPGFlags                   m_flags;

    // Maximum length (for string properties). Could be in some sort of
    // wxBaseStringProperty, but currently, for maximum flexibility and
    // compatibility, we'll stick it here.
    int                         m_maxLen;

    // Root has 0, categories etc. at that level 1, etc.
    unsigned char               m_depth;

    // m_depthBgCol indicates width of background colour between margin and item
    // (essentially this is category's depth, if none then equals m_depth).
    unsigned char               m_depthBgCol;

private:
    // Called in constructors.
    void Init();
    void Init( const wxString& label, const wxString& name );
};

// -----------------------------------------------------------------------

//
// Property class declaration helper macros
// (wxPGRootPropertyClass and wxPropertyCategory require this).
//

#define WX_PG_DECLARE_DOGETEDITORCLASS \
    virtual const wxPGEditor* DoGetEditorClass() const override;

#ifndef WX_PG_DECLARE_PROPERTY_CLASS
    #define WX_PG_DECLARE_PROPERTY_CLASS(CLASSNAME) \
        public: \
            wxDECLARE_DYNAMIC_CLASS(CLASSNAME); \
            WX_PG_DECLARE_DOGETEDITORCLASS \
        private:
#endif

// Implements sans constructor function. Also, first arg is class name, not
// property name.
#define wxPG_IMPLEMENT_PROPERTY_CLASS_PLAIN(PROPNAME, EDITOR) \
const wxPGEditor* PROPNAME::DoGetEditorClass() const \
{ \
    return wxPGEditor_##EDITOR; \
}

#if WXWIN_COMPATIBILITY_3_0
// This macro is deprecated. Use wxPG_IMPLEMENT_PROPERTY_CLASS_PLAIN instead.
#define WX_PG_IMPLEMENT_PROPERTY_CLASS_PLAIN(PROPNAME,T,EDITOR) \
wxPG_IMPLEMENT_PROPERTY_CLASS_PLAIN(PROPNAME, EDITOR)
#endif // WXWIN_COMPATIBILITY_3_0

// -----------------------------------------------------------------------

// Root parent property.
class WXDLLIMPEXP_PROPGRID wxPGRootProperty : public wxPGProperty
{
public:
    WX_PG_DECLARE_PROPERTY_CLASS(wxPGRootProperty)
public:

    // Constructor.
    wxPGRootProperty( const wxString& name = wxS("<Root>") );
    virtual ~wxPGRootProperty() = default;

#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use StringToValue with 'flags' argument as wxPGPropValFormatFlags")
    virtual bool StringToValue( wxVariant&, const wxString&, int ) const override
    {
        return false;
    }
#endif // WXWIN_COMPATIBILITY_3_2
    virtual bool StringToValue( wxVariant&, const wxString&, wxPGPropValFormatFlags ) const override
    {
        return false;
    }

protected:
};

// -----------------------------------------------------------------------

// Category (caption) property.
class WXDLLIMPEXP_PROPGRID wxPropertyCategory : public wxPGProperty
{
    friend class wxPropertyGridPageState;
    WX_PG_DECLARE_PROPERTY_CLASS(wxPropertyCategory)
public:

    // Default constructor is only used in special cases.
    wxPropertyCategory();

    wxPropertyCategory( const wxString& label,
                        const wxString& name = wxPG_LABEL );
    virtual ~wxPropertyCategory() = default;

    int GetTextExtent( const wxWindow* wnd, const wxFont& font ) const;

#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use ValueToString with 'flags' argument as wxPGPropValFormatFlags")
    virtual wxString ValueToString(wxVariant& value, int flags) const override
    {
        m_oldValueToStringCalled = true;
        return ValueToString(value, static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    virtual wxString ValueToString(wxVariant& value, wxPGPropValFormatFlags flags) const override;
#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("use GetValueAsString with 'flags' argument as wxPGPropValFormatFlags")
    virtual wxString GetValueAsString(int flags) const override
    {
        m_oldGetValueAsString = true;
        return GetValueAsString(static_cast<wxPGPropValFormatFlags>(flags));
    }
#endif // WXWIN_COMPATIBILITY_3_2
    virtual wxString GetValueAsString(wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

protected:
    void SetTextColIndex( unsigned int colInd )
        { m_capFgColIndex = (wxByte) colInd; }
    unsigned int GetTextColIndex() const
        { return (unsigned int) m_capFgColIndex; }

    void CalculateTextExtent(const wxWindow* wnd, const wxFont& font);

    int     m_textExtent;  // pre-calculated length of text
    wxByte  m_capFgColIndex;  // caption text colour index

private:
    void Init();
};

// -----------------------------------------------------------------------

#endif // wxUSE_PROPGRID

#endif // _WX_PROPGRID_PROPERTY_H_
