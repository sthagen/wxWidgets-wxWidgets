/////////////////////////////////////////////////////////////////////////////
// Name:        docs/doxygen/overviews/persistence.h
// Purpose:     overview of persistent objects
// Author:      Vadim Zeitlin
// Created:     2009-01-23
// Copyright:   (c) 2009 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/**

@page overview_persistence Persistent Objects Overview

@tableofcontents

Persistent objects are simply the objects which automatically save their state
when they are destroyed and restore it when they are recreated, even during
another program invocation.

Most often, persistent objects are, in fact, persistent windows as it is
especially convenient to automatically restore the UI state when the program is
restarted but an object of any class can be made persistent. Moreover,
persistence is implemented in a non-intrusive way so that the original object
class doesn't need to be modified at all in order to add support for saving and
restoring its properties.

The persistence framework includes the following components:

- wxPersistenceManager which all persistent objects register themselves with.
  This class handles actual saving and restoring of persistent data as well as
  various global aspects of persistence, e.g. it can be used to disable
  restoring the saved data.
- wxPersistentObject is the base class for all persistent objects or, rather,
  adaptors for the persistent objects as this class main purpose is to provide
  the bridge between the original class -- which has no special persistence
  support -- and wxPersistenceManager,
- wxPersistentWindow<> which derives from wxPersistentObject and implements some
  of its methods using wxWindow-specific functionality. Notably,
  wxPersistenceManager handles the destruction of persistent windows
  automatically implicitly while it has to be done explicitly for the
  arbitrary persistent objects.
- wxCreatePersistentObject() function which is used to create the
  appropriate persistence adapter for the object.


@section persistence_using Using Persistent Windows

wxWidgets has built-in support for a (constantly growing) number of controls.
Currently the following classes are supported:

- wxTopLevelWindow (and hence wxFrame and wxDialog), see wxPersistentTLW.
- wxBookCtrlBase (i.e. wxNotebook, wxListbook, wxToolbook and wxChoicebook),
  see wxPersistentBookCtrl and wxPersistentTreeBookCtrl.
- wxDataViewCtrl (and derivatives such as wxDataViewListCtrl), see
  wxPersistentDataViewCtrl.
- wxCheckBox, see wxPersistentCheckBox.
- wxComboBox, see wxPersistentComboBox.
- wxRadioButton, see wxPersistentRadioButton.

To automatically save and restore the properties of the windows of classes
listed above you need to:

-# Set a unique name for the window by either using this name in its
   constructor or calling wxWindow::SetName(): this step is important as the
   name is used in the configuration file and so must be unique among all
   windows of the same class.
-# Call wxPersistenceManager::Register() at any moment after creating the
   window and then wxPersistenceManager::Restore() when the settings may be
   restored (which can't be always done immediately, e.g. often the window
   needs to be populated first). If settings can be restored immediately after
   the window creation, as is often the case for wxTopLevelWindow, for
   example, then wxPersistenceManager::RegisterAndRestore() can be used to do
   both at once.
-# If you do not want the settings for the window to be saved (for example
   the changes to the dialog size are usually not saved if the dialog was
   cancelled), you need to call wxPersistenceManager::Unregister() manually.
   Otherwise the settings will be automatically saved when the control itself
   is destroyed.

A convenient wxPersistentRegisterAndRestore() helper function can be used to
perform the first steps at once, e.g. to automatically save and restore the
position of a @c MyFrame window in your application, you need to only do the
following:

@code
#include <wx/persist/toplevel.h>

...

MyFrame::MyFrame(wxWindow* parent, ...)
       : wxFrame(parent, ...)
{
    ... all the other initialization ...

    // Restore the previously saved geometry, if any, and register this frame
    // for its geometry to be saved when it is closed using the given wxConfig
    // key name.
    if ( !wxPersistentRegisterAndRestore(this, "my_frame_name") )
    {
        // Choose some custom default size for the first run -- or don't do
        // anything at all and let the system use the default initial size.
        SetClientSize(FromDIP(wxSize(800, 600)));
    }

    Show(true);
}
@endcode

And here is an example of using a notebook control which automatically
remembers the last open page without using the helper function:

@code
    wxNotebook *book = new wxNotebook(parent, wxID_ANY);
    book->SetName("MyBook"); // do not use the default name
    book->AddPage(...);
    book->AddPage(...);
    book->AddPage(...);

    // We don't check for the return value here as the first page is selected
    // by default anyhow and we just keep this selection in this case.
    wxPersistenceManager::Get().RegisterAndRestore(book);
@endcode


@section persistence_defining Defining Custom Persistent Windows

User-defined classes can be easily integrated with wxPersistenceManager. To add
support for your custom class @c MyWidget you just need to:

-# Define a new `MyPersistentWidget` class inheriting from
   `wxPersistentWindow<MyWidget>`.
-# Implement its pure virtual GetKind() method returning a unique string
   identifying all `MyWidget` objects, typically something like `"widget"`
-# Implement its pure virtual Save() and Restore() methods to actually save
   and restore the widget settings using wxPersistentObject::SaveValue() and
   wxPersistentObject::RestoreValue() methods.
-# Define wxCreatePersistentObject() overload taking `MyWidget*` and
   returning a new `MyPersistentWidget` object.

If you want to add persistence support for a class not deriving from wxWindow,
you need to derive `MyPersistentWidget` directly from wxPersistentObject and
so implement its pure virtual wxPersistentObject::GetName() method too.
Additionally, you must ensure that wxPersistenceManager::SaveAndUnregister() is
called when your object is destroyed as this can be only done automatically for
windows.

*/
