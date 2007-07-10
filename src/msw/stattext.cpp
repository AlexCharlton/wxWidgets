/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/stattext.cpp
// Purpose:     wxStaticText
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_STATTEXT

#include "wx/stattext.h"

#ifndef WX_PRECOMP
    #include "wx/event.h"
    #include "wx/app.h"
    #include "wx/brush.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
#endif

#include "wx/msw/private.h"


// ----------------------------------------------------------------------------
// wxStaticText
// ----------------------------------------------------------------------------

bool wxStaticText::Create(wxWindow *parent,
                          wxWindowID id,
                          const wxString& label,
                          const wxPoint& pos,
                          const wxSize& size,
                          long style,
                          const wxString& name)
{
    if ( !CreateControl(parent, id, pos, size, style, wxDefaultValidator, name) )
        return false;

    if ( !MSWCreateControl(wxT("STATIC"), label, pos, size) )
        return false;

    return true;
}

wxBorder wxStaticText::GetDefaultBorder() const
{
    return wxBORDER_NONE;
}

WXDWORD wxStaticText::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    WXDWORD msStyle = wxControl::MSWGetStyle(style, exstyle);

    // translate the alignment flags to the Windows ones
    //
    // note that both wxALIGN_LEFT and SS_LEFT are equal to 0 so we shouldn't
    // test for them using & operator
    if ( style & wxALIGN_CENTRE )
        msStyle |= SS_CENTER;
    else if ( style & wxALIGN_RIGHT )
        msStyle |= SS_RIGHT;
    else
        msStyle |= SS_LEFT;

    // this style is necessary to receive mouse events
    // Win NT and later have the SS_ENDELLIPSIS style which is useful to us:
    if (wxGetOsVersion() == wxOS_WINDOWS_NT)
    {
        // for now, add the SS_ENDELLIPSIS style if wxST_ELLIPSIZE_END is given;
        // we may need to remove it later in ::SetLabel() if the given label
        // has newlines
        if ( style & wxST_ELLIPSIZE_END )
            msStyle |= SS_ENDELLIPSIS;
    }

    msStyle |= SS_NOTIFY;

    return msStyle;
}

wxSize wxStaticText::DoGetBestSize() const
{
    wxClientDC dc(wx_const_cast(wxStaticText *, this));
    wxFont font(GetFont());
    if (!font.Ok())
        font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

    dc.SetFont(font);

    wxCoord widthTextMax, heightTextTotal;
    dc.GetMultiLineTextExtent(GetLabelText(), &widthTextMax, &heightTextTotal);

#ifdef __WXWINCE__
    if ( widthTextMax )
        widthTextMax += 2;
#endif // __WXWINCE__

    // border takes extra space
    //
    // TODO: this is probably not wxStaticText-specific and should be moved
    wxCoord border;
    switch ( GetBorder() )
    {
        case wxBORDER_STATIC:
        case wxBORDER_SIMPLE:
            border = 1;
            break;

        case wxBORDER_SUNKEN:
            border = 2;
            break;

        case wxBORDER_RAISED:
        case wxBORDER_DOUBLE:
            border = 3;
            break;

        default:
            wxFAIL_MSG( _T("unknown border style") );
            // fall through

        case wxBORDER_NONE:
            border = 0;
    }

    widthTextMax += 2*border;
    heightTextTotal += 2*border;

    wxSize best(widthTextMax, heightTextTotal);
    CacheBestSize(best);
    return best;
}

void wxStaticText::DoSetSize(int x, int y, int w, int h, int sizeFlags)
{
    // note: we first need to set the size and _then_ call UpdateLabel
    wxStaticTextBase::DoSetSize(x, y, w, h, sizeFlags);

    // do we need to ellipsize the contents?
    long styleReal = ::GetWindowLong(GetHwnd(), GWL_STYLE);
    if ( !(styleReal & SS_ENDELLIPSIS) )
    {
        // we don't have SS_ENDELLIPSIS style:
        // we need to (eventually) do ellipsization ourselves
        UpdateLabel();
    }
    //else: we don't or the OS will do it for us

    // we need to refresh the window after changing its size as the standard
    // control doesn't always update itself properly
    Refresh();
}

void wxStaticText::SetLabel(const wxString& label)
{
    long styleReal = ::GetWindowLong(GetHwnd(), GWL_STYLE);
    if ( HasFlag(wxST_ELLIPSIZE_END) &&
          wxGetOsVersion() == wxOS_WINDOWS_NT )
    {
        // adding SS_ENDELLIPSIS or SS_ENDELLIPSIS "disables" the correct 
        // newline handling in static texts: the newlines in the labels are 
        // shown as square. Thus we don't use it even on newer OS when 
        // the static label contains a newline.
        if ( label.Contains(wxT('\n')) )
            styleReal &= ~SS_ENDELLIPSIS;
        else
            styleReal |= SS_ENDELLIPSIS;

        ::SetWindowLong(GetHwnd(), GWL_STYLE, styleReal);
    }
    else // style not supported natively
    {
        styleReal &= ~SS_ENDELLIPSIS;
        ::SetWindowLong(GetHwnd(), GWL_STYLE, styleReal);
    }

    // this call will save the label in m_labelOrig and set it into this window
    // (through wxWindow::SetLabel)
    m_labelOrig = label;

    if ((styleReal & SS_ENDELLIPSIS) == 0)
        DoSetLabel(GetEllipsizedLabelWithoutMarkup());
    else
        DoSetLabel(RemoveMarkup(label));

    // adjust the size of the window to fit to the label unless autoresizing is
    // disabled
    if ( !HasFlag(wxST_NO_AUTORESIZE) &&
         !IsEllipsized() )  // if ellipsize is ON, then we don't want to get resized!
    {
        InvalidateBestSize();
        DoSetSize(wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord,
                  wxSIZE_AUTO_WIDTH | wxSIZE_AUTO_HEIGHT);
    }
}

bool wxStaticText::SetFont(const wxFont& font)
{
    bool ret = wxControl::SetFont(font);

    // adjust the size of the window to fit to the label unless autoresizing is
    // disabled
    if ( !HasFlag(wxST_NO_AUTORESIZE) )
    {
        InvalidateBestSize();
        DoSetSize(wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord,
                  wxSIZE_AUTO_WIDTH | wxSIZE_AUTO_HEIGHT);
    }

    return ret;
}

// for wxST_ELLIPSIZE_* support:

wxString wxStaticText::DoGetLabel() const
{
    return wxGetWindowText(GetHwnd());
}

void wxStaticText::DoSetLabel(const wxString& str)
{
    SetWindowText(GetHwnd(), str.c_str());
}


#endif // wxUSE_STATTEXT
