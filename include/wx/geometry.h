/////////////////////////////////////////////////////////////////////////////
// Name:        wx/geometry.h
// Purpose:     Common Geometry Classes
// Author:      Stefan Csomor
// Created:     08/05/99
// Copyright:   (c) 1999 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GEOMETRY_H_
#define _WX_GEOMETRY_H_

#include "wx/defs.h"

#if wxUSE_GEOMETRY

#include "wx/utils.h"
#include "wx/gdicmn.h"
#include "wx/math.h"

class WXDLLIMPEXP_FWD_BASE wxDataInputStream;
class WXDLLIMPEXP_FWD_BASE wxDataOutputStream;

// clipping from Cohen-Sutherland

enum wxOutCode
{
    wxInside = 0x00 ,
    wxOutLeft = 0x01 ,
    wxOutRight = 0x02 ,
    wxOutTop = 0x08 ,
    wxOutBottom = 0x04
};

class WXDLLIMPEXP_CORE wxPoint2DInt
{
public :
    wxPoint2DInt() = default;
    inline wxPoint2DInt( wxInt32 x , wxInt32 y );
    inline wxPoint2DInt( const wxPoint &pt );
    // default copy ctor and copy-assign operator are OK

    // noops for this class, just return the coords
    inline void GetFloor( wxInt32 *x , wxInt32 *y ) const;
    inline void GetRounded( wxInt32 *x , wxInt32 *y ) const;

    inline wxDouble GetVectorLength() const;
           wxDouble GetVectorAngle() const;
    inline void SetVectorLength( wxDouble length );
           void SetVectorAngle( wxDouble degrees );
    // set the vector length to 1.0, preserving the angle
    inline void Normalize();

    inline wxDouble GetDistance( const wxPoint2DInt &pt ) const;
    inline wxDouble GetDistanceSquare( const wxPoint2DInt &pt ) const;
    inline wxInt32 GetDotProduct( const wxPoint2DInt &vec ) const;
    inline wxInt32 GetCrossProduct( const wxPoint2DInt &vec ) const;

    // the reflection of this point
    wxPoint2DInt operator-() const;

    inline wxPoint2DInt& operator+=(const wxPoint2DInt& pt);
    inline wxPoint2DInt& operator-=(const wxPoint2DInt& pt);
    inline wxPoint2DInt& operator*=(wxDouble n);
    inline wxPoint2DInt& operator*=(wxInt32 n);
    inline wxPoint2DInt& operator/=(wxDouble n);
    inline wxPoint2DInt& operator/=(wxInt32 n);
    inline operator wxPoint() const;
    inline bool operator==(const wxPoint2DInt& pt) const;
    inline bool operator!=(const wxPoint2DInt& pt) const;

    friend wxPoint2DInt operator+(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
    {
        return wxPoint2DInt( pt1.m_x + pt2.m_x , pt1.m_y + pt2.m_y );
    }

    friend wxPoint2DInt operator-(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
    {
        return wxPoint2DInt( pt1.m_x - pt2.m_x , pt1.m_y - pt2.m_y );
    }


    friend wxPoint2DInt operator*(wxInt32 n , const wxPoint2DInt& pt)
    {
        return wxPoint2DInt( pt.m_x * n , pt.m_y * n );
    }

    friend wxPoint2DInt operator*(wxDouble n , const wxPoint2DInt& pt)
    {
        return wxPoint2DInt( static_cast<wxInt32>(pt.m_x * n) ,
            static_cast<wxInt32>(pt.m_y * n) );
    }

    friend wxPoint2DInt operator*(const wxPoint2DInt& pt , wxInt32 n)
    {
        return wxPoint2DInt( pt.m_x * n , pt.m_y * n );
    }

    friend wxPoint2DInt operator*(const wxPoint2DInt& pt , wxDouble n)
    {
        return wxPoint2DInt( static_cast<wxInt32>(pt.m_x * n) ,
            static_cast<wxInt32>(pt.m_y * n) );
    }

    friend wxPoint2DInt operator/(const wxPoint2DInt& pt , wxInt32 n)
    {
        return wxPoint2DInt( pt.m_x / n , pt.m_y / n );
    }

    friend wxPoint2DInt operator/(const wxPoint2DInt& pt , wxDouble n)
    {
        return wxPoint2DInt( static_cast<wxInt32>(pt.m_x / n) ,
            static_cast<wxInt32>(pt.m_y / n) );
    }

#if wxUSE_STREAMS
    void WriteTo( wxDataOutputStream &stream ) const;
    void ReadFrom( wxDataInputStream &stream );
#endif // wxUSE_STREAMS

#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("Multiplying points doesn't make sense")
    wxPoint2DInt& operator*=(const wxPoint2DInt& pt)
    {
        m_x *= pt.m_x;
        m_y *= pt.m_y;
        return *this;
    }

    wxDEPRECATED_MSG("Multiplying points doesn't make sense")
    friend wxPoint2DInt operator*(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
    {
        return wxPoint2DInt( pt1.m_x * pt2.m_x , pt1.m_y * pt2.m_y );
    }

    wxDEPRECATED_MSG("Dividing points doesn't make sense")
    inline wxPoint2DInt& operator/=(const wxPoint2DInt& pt)
    {
        m_x /= pt.m_x;
        m_y /= pt.m_y;
        return *this;
    }

    wxDEPRECATED_MSG("Dividing points doesn't make sense")
    friend wxPoint2DInt operator/(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
    {
        return wxPoint2DInt( pt1.m_x / pt2.m_x , pt1.m_y / pt2.m_y );
    }
#endif // WXWIN_COMPATIBILITY_3_2

    wxInt32 m_x = 0;
    wxInt32 m_y = 0;
};

inline wxPoint2DInt::wxPoint2DInt( wxInt32 x , wxInt32 y )
{
    m_x = x;
    m_y = y;
}

inline wxPoint2DInt::wxPoint2DInt( const wxPoint &pt )
{
    m_x = pt.x;
    m_y = pt.y;
}

inline void wxPoint2DInt::GetFloor( wxInt32 *x , wxInt32 *y ) const
{
    if ( x )
        *x = m_x;
    if ( y )
        *y = m_y;
}

inline void wxPoint2DInt::GetRounded( wxInt32 *x , wxInt32 *y ) const
{
    GetFloor(x, y);
}

inline wxDouble wxPoint2DInt::GetVectorLength() const
{
    return sqrt( static_cast<wxDouble>(m_x*m_x + m_y*m_y) );
}

inline void wxPoint2DInt::SetVectorLength( wxDouble length )
{
    wxDouble before = GetVectorLength();
    m_x = (wxInt32)(m_x * length / before);
    m_y = (wxInt32)(m_y * length / before);
}

inline void wxPoint2DInt::Normalize()
{
    SetVectorLength( 1 );
}

inline wxDouble wxPoint2DInt::GetDistance( const wxPoint2DInt &pt ) const
{
    return sqrt( GetDistanceSquare( pt ) );
}

inline wxDouble wxPoint2DInt::GetDistanceSquare( const wxPoint2DInt &pt ) const
{
    return ( ((wxDouble)pt.m_x-m_x)*((wxDouble)pt.m_x-m_x) +
             ((wxDouble)pt.m_y-m_y)*((wxDouble)pt.m_y-m_y) );
}

inline wxInt32 wxPoint2DInt::GetDotProduct( const wxPoint2DInt &vec ) const
{
    return ( m_x * vec.m_x + m_y * vec.m_y );
}

inline wxInt32 wxPoint2DInt::GetCrossProduct( const wxPoint2DInt &vec ) const
{
    return ( m_x * vec.m_y - vec.m_x * m_y );
}

inline wxPoint2DInt::operator wxPoint() const
{
    return wxPoint( m_x, m_y);
}

inline wxPoint2DInt wxPoint2DInt::operator-() const
{
    return wxPoint2DInt( -m_x, -m_y);
}

inline wxPoint2DInt& wxPoint2DInt::operator+=(const wxPoint2DInt& pt)
{
    m_x += pt.m_x;
    m_y += pt.m_y;
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator-=(const wxPoint2DInt& pt)
{
    m_x -= pt.m_x;
    m_y -= pt.m_y;
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator*=(wxDouble n)
{
    m_x = wxRound(m_x * n);
    m_y = wxRound(m_y * n);
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator*=(wxInt32 n)
{
    m_x *= n;
    m_y *= n;
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator/=(wxDouble n)
{
    m_x = wxRound(m_x / n);
    m_y = wxRound(m_y / n);
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator/=(wxInt32 n)
{
    m_x /= n;
    m_y /= n;
    return *this;
}

inline bool wxPoint2DInt::operator==(const wxPoint2DInt& pt) const
{
    return m_x == pt.m_x && m_y == pt.m_y;
}

inline bool wxPoint2DInt::operator!=(const wxPoint2DInt& pt) const
{
    return m_x != pt.m_x || m_y != pt.m_y;
}

// wxPoint2Ds represent a point or a vector in a 2d coordinate system

class WXDLLIMPEXP_CORE wxPoint2DDouble
{
public :
    wxPoint2DDouble() = default;
    inline wxPoint2DDouble( wxDouble x , wxDouble y );
    wxPoint2DDouble( const wxPoint2DInt &pt )
        { m_x = (wxDouble) pt.m_x ; m_y = (wxDouble) pt.m_y ; }
    wxPoint2DDouble( const wxPoint &pt )
        { m_x = (wxDouble) pt.x ; m_y = (wxDouble) pt.y ; }
    // default copy ctor and copy-assign operator are OK

    // two different conversions to integers, floor and rounding
    inline void GetFloor( wxInt32 *x , wxInt32 *y ) const;
    inline void GetRounded( wxInt32 *x , wxInt32 *y ) const;

    inline wxPoint GetFloor() const;
    inline wxPoint GetRounded() const;

    inline wxDouble GetVectorLength() const;
     wxDouble GetVectorAngle() const ;
    void SetVectorLength( wxDouble length );
    void SetVectorAngle( wxDouble degrees );
    // set the vector length to 1.0, preserving the angle
    void Normalize();

    inline wxDouble GetDistance( const wxPoint2DDouble &pt ) const;
    inline wxDouble GetDistanceSquare( const wxPoint2DDouble &pt ) const;
    inline wxDouble GetDotProduct( const wxPoint2DDouble &vec ) const;
    inline wxDouble GetCrossProduct( const wxPoint2DDouble &vec ) const;

    // the reflection of this point
    wxPoint2DDouble operator-() const;

    inline wxPoint2DDouble& operator+=(const wxPoint2DDouble& pt);
    inline wxPoint2DDouble& operator-=(const wxPoint2DDouble& pt);
    inline wxPoint2DDouble& operator*=(wxDouble n);
    inline wxPoint2DDouble& operator*=(wxInt32 n);
    inline wxPoint2DDouble& operator/=(wxDouble n);
    inline wxPoint2DDouble& operator/=(wxInt32 n);

    inline bool operator==(const wxPoint2DDouble& pt) const;
    inline bool operator!=(const wxPoint2DDouble& pt) const;

    friend wxPoint2DDouble operator+(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
    {
        return wxPoint2DDouble( pt1.m_x + pt2.m_x , pt1.m_y + pt2.m_y );
    }

    friend wxPoint2DDouble operator-(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
    {
        return wxPoint2DDouble( pt1.m_x - pt2.m_x , pt1.m_y - pt2.m_y );
    }


    friend wxPoint2DDouble operator*(wxDouble n , const wxPoint2DDouble& pt)
    {
        return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
    }

    friend wxPoint2DDouble operator*(wxInt32 n , const wxPoint2DDouble& pt)
    {
        return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
    }

    friend wxPoint2DDouble operator*(const wxPoint2DDouble& pt , wxDouble n)
    {
        return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
    }

    friend wxPoint2DDouble operator*(const wxPoint2DDouble& pt , wxInt32 n)
    {
        return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
    }

    friend wxPoint2DDouble operator/(const wxPoint2DDouble& pt , wxDouble n)
    {
        return wxPoint2DDouble( pt.m_x / n , pt.m_y / n );
    }

    friend wxPoint2DDouble operator/(const wxPoint2DDouble& pt , wxInt32 n)
    {
        return wxPoint2DDouble( pt.m_x / n , pt.m_y / n );
    }

#if WXWIN_COMPATIBILITY_3_2
    wxDEPRECATED_MSG("Multiplying points doesn't make sense")
    inline wxPoint2DDouble& operator*=(const wxPoint2DDouble& pt)
    {
        m_x *= pt.m_x;
        m_y *= pt.m_y;
        return *this;
    }

    wxDEPRECATED_MSG("Multiplying points doesn't make sense")
    friend wxPoint2DDouble operator*(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
    {
        return wxPoint2DDouble( pt1.m_x * pt2.m_x , pt1.m_y * pt2.m_y );
    }

    wxDEPRECATED_MSG("Dividing points doesn't make sense")
    inline wxPoint2DDouble& operator/=(const wxPoint2DDouble& pt)
    {
        m_x /= pt.m_x;
        m_y /= pt.m_y;
        return *this;
    }

    wxDEPRECATED_MSG("Dividing points doesn't make sense")
    friend wxPoint2DDouble operator/(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
    {
        return wxPoint2DDouble( pt1.m_x / pt2.m_x , pt1.m_y / pt2.m_y );
    }
#endif // WXWIN_COMPATIBILITY_3_2

    wxDouble m_x = 0.0;
    wxDouble m_y = 0.0;
};

inline wxPoint2DDouble::wxPoint2DDouble( wxDouble x , wxDouble y )
{
    m_x = x;
    m_y = y;
}

inline void wxPoint2DDouble::GetFloor( wxInt32 *x , wxInt32 *y ) const
{
    *x = (wxInt32) floor( m_x );
    *y = (wxInt32) floor( m_y );
}

inline void wxPoint2DDouble::GetRounded( wxInt32 *x , wxInt32 *y ) const
{
    *x = wxRound( m_x );
    *y = wxRound( m_y );
}

inline wxPoint wxPoint2DDouble::GetFloor() const
{
    return wxPoint( (wxInt32) floor( m_x ), (wxInt32) floor( m_y ) );
}

inline wxPoint wxPoint2DDouble::GetRounded() const
{
    return wxPoint( wxRound( m_x ), wxRound( m_y ) );
}

inline wxDouble wxPoint2DDouble::GetVectorLength() const
{
    return sqrt( (m_x)*(m_x) + (m_y)*(m_y) ) ;
}

inline void wxPoint2DDouble::SetVectorLength( wxDouble length )
{
    wxDouble before = GetVectorLength() ;
    m_x = (m_x * length / before) ;
    m_y = (m_y * length / before) ;
}

inline void wxPoint2DDouble::Normalize()
{
    SetVectorLength( 1 );
}

inline wxDouble wxPoint2DDouble::GetDistance( const wxPoint2DDouble &pt ) const
{
    return sqrt( GetDistanceSquare( pt ) );
}

inline wxDouble wxPoint2DDouble::GetDistanceSquare( const wxPoint2DDouble &pt ) const
{
    return ( (pt.m_x-m_x)*(pt.m_x-m_x) + (pt.m_y-m_y)*(pt.m_y-m_y) );
}

inline wxDouble wxPoint2DDouble::GetDotProduct( const wxPoint2DDouble &vec ) const
{
    return ( m_x * vec.m_x + m_y * vec.m_y );
}

inline wxDouble wxPoint2DDouble::GetCrossProduct( const wxPoint2DDouble &vec ) const
{
    return ( m_x * vec.m_y - vec.m_x * m_y );
}

inline wxPoint2DDouble wxPoint2DDouble::operator-() const
{
    return wxPoint2DDouble( -m_x, -m_y);
}

inline wxPoint2DDouble& wxPoint2DDouble::operator+=(const wxPoint2DDouble& pt)
{
    m_x += pt.m_x;
    m_y += pt.m_y;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator-=(const wxPoint2DDouble& pt)
{
    m_x -= pt.m_x;
    m_y -= pt.m_y;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator*=(wxDouble n)
{
    m_x *= n;
    m_y *= n;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator*=(wxInt32 n)
{
    m_x *= n;
    m_y *= n;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator/=(wxDouble n)
{
    m_x /= n;
    m_y /= n;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator/=(wxInt32 n)
{
    m_x /= n;
    m_y /= n;
    return *this;
}

inline bool wxPoint2DDouble::operator==(const wxPoint2DDouble& pt) const
{
    return wxIsSameDouble(m_x, pt.m_x) && wxIsSameDouble(m_y, pt.m_y);
}

inline bool wxPoint2DDouble::operator!=(const wxPoint2DDouble& pt) const
{
    return !(*this == pt);
}

// wxRect2Ds are axis-aligned rectangles, each side of the rect is parallel to the x- or m_y- axis. The rectangle is either defined by the
// top left and bottom right corner, or by the top left corner and size. A point is contained within the rectangle if
// left <= x < right  and top <= m_y < bottom , thus it is a half open interval.

class WXDLLIMPEXP_CORE wxRect2DDouble
{
public:
    wxRect2DDouble() = default;
    wxRect2DDouble(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
        { m_x = x; m_y = y; m_width = w;  m_height = h; }

    explicit wxRect2DDouble(const wxRect& rect)
    {
        m_x = static_cast<wxDouble>(rect.x);
        m_y = static_cast<wxDouble>(rect.y);
        m_width = static_cast<wxDouble>(rect.width);
        m_height = static_cast<wxDouble>(rect.height);
    }

    wxNODISCARD wxRect ToRect() const
    {
        return wxRect(wxRound(m_x), wxRound(m_y),
                      wxRound(m_width), wxRound(m_height));
    }
/*
    wxRect2DDouble(const wxPoint2DDouble& topLeft, const wxPoint2DDouble& bottomRight);
    wxRect2DDouble(const wxPoint2DDouble& pos, const wxSize& size);
    wxRect2DDouble(const wxRect2DDouble& rect);
*/
        // single attribute accessors

    wxNODISCARD wxPoint2DDouble GetPosition() const
        { return wxPoint2DDouble(m_x, m_y); }
    wxNODISCARD wxSize GetSize() const
        { return wxSize((int) m_width, (int) m_height); }

    wxNODISCARD wxDouble GetX() const
        { return m_x; }

    wxNODISCARD wxDouble GetY() const
        { return m_y; }

    wxNODISCARD wxDouble GetWidth() const
        { return m_width; }

    void SetWidth(wxDouble w) { m_width = w; }

    wxNODISCARD wxDouble GetHeight() const
        { return m_height; }

    void SetHeight(wxDouble h) { m_height = h; }

    // for the edge and corner accessors there are two setters counterparts, the Set.. functions keep the other corners at their
        // position whenever sensible, the Move.. functions keep the size of the rect and move the other corners appropriately

    inline wxDouble GetLeft() const { return m_x; }
    inline void SetLeft( wxDouble n ) { m_width += m_x - n; m_x = n; }
    inline void MoveLeftTo( wxDouble n ) { m_x = n; }
    inline wxDouble GetTop() const { return m_y; }
    inline void SetTop( wxDouble n ) { m_height += m_y - n; m_y = n; }
    inline void MoveTopTo( wxDouble n ) { m_y = n; }
    inline wxDouble GetBottom() const { return m_y + m_height; }
    inline void SetBottom( wxDouble n ) { m_height += n - (m_y+m_height);}
    inline void MoveBottomTo( wxDouble n ) { m_y = n - m_height; }
    inline wxDouble GetRight() const { return m_x + m_width; }
    inline void SetRight( wxDouble n ) { m_width += n - (m_x+m_width) ; }
    inline void MoveRightTo( wxDouble n ) { m_x = n - m_width; }

    inline wxPoint2DDouble GetLeftTop() const
        { return wxPoint2DDouble( m_x , m_y ); }
    inline void SetLeftTop( const wxPoint2DDouble &pt )
        { m_width += m_x - pt.m_x; m_height += m_y - pt.m_y; m_x = pt.m_x; m_y = pt.m_y; }
    inline void MoveLeftTopTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x; m_y = pt.m_y; }
    inline wxPoint2DDouble GetLeftBottom() const
        { return wxPoint2DDouble( m_x , m_y + m_height ); }
    inline void SetLeftBottom( const wxPoint2DDouble &pt )
        { m_width += m_x - pt.m_x; m_height += pt.m_y - (m_y+m_height) ; m_x = pt.m_x; }
    inline void MoveLeftBottomTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x; m_y = pt.m_y - m_height; }
    inline wxPoint2DDouble GetRightTop() const
        { return wxPoint2DDouble( m_x+m_width , m_y ); }
    inline void SetRightTop( const wxPoint2DDouble &pt )
        { m_width += pt.m_x - ( m_x + m_width ); m_height += m_y - pt.m_y; m_y = pt.m_y; }
    inline void MoveRightTopTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x - m_width; m_y = pt.m_y; }
    inline wxPoint2DDouble GetRightBottom() const
        { return wxPoint2DDouble( m_x+m_width , m_y + m_height ); }
    inline void SetRightBottom( const wxPoint2DDouble &pt )
        { m_width += pt.m_x - ( m_x + m_width ); m_height += pt.m_y - (m_y+m_height);}
    inline void MoveRightBottomTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x - m_width; m_y = pt.m_y - m_height; }
    inline wxPoint2DDouble GetCentre() const
        { return wxPoint2DDouble( m_x+m_width/2 , m_y+m_height/2 ); }
    inline void SetCentre( const wxPoint2DDouble &pt )
        { MoveCentreTo( pt ); }    // since this is impossible without moving...
    inline void MoveCentreTo( const wxPoint2DDouble &pt )
        { m_x += pt.m_x - (m_x+m_width/2); m_y += pt.m_y -(m_y+m_height/2); }
    inline wxOutCode GetOutCode( const wxPoint2DDouble &pt ) const
        { return (wxOutCode) (( ( pt.m_x < m_x ) ? wxOutLeft : 0 ) +
                     ( ( pt.m_x > m_x + m_width ) ? wxOutRight : 0 ) +
                     ( ( pt.m_y < m_y ) ? wxOutTop : 0 )  +
                     ( ( pt.m_y > m_y + m_height ) ? wxOutBottom : 0 )); }
    inline wxOutCode GetOutcode(const wxPoint2DDouble &pt) const
        { return GetOutCode(pt) ; }
    inline bool Contains( const wxPoint2DDouble &pt ) const
        { return  GetOutCode( pt ) == wxInside; }
    inline bool Contains( const wxRect2DDouble &rect ) const
        { return ( ( ( m_x <= rect.m_x ) && ( rect.m_x + rect.m_width <= m_x + m_width ) ) &&
                ( ( m_y <= rect.m_y ) && ( rect.m_y + rect.m_height <= m_y + m_height ) ) ); }
    inline bool IsEmpty() const
        { return m_width <= 0 || m_height <= 0; }
    inline bool HaveEqualSize( const wxRect2DDouble &rect ) const
        { return wxIsSameDouble(rect.m_width, m_width) && wxIsSameDouble(rect.m_height, m_height); }

    inline void Inset( wxDouble x , wxDouble y )
        { m_x += x; m_y += y; m_width -= 2 * x; m_height -= 2 * y; }
    inline void Inset( wxDouble left , wxDouble top ,wxDouble right , wxDouble bottom  )
        { m_x += left; m_y += top; m_width -= left + right; m_height -= top + bottom;}
    inline void Offset( const wxPoint2DDouble &pt )
        { m_x += pt.m_x; m_y += pt.m_y; }
    inline void Offset(wxDouble dx, wxDouble dy)
        { Offset({ dx, dy }); }

    wxRect2DDouble& Inflate(wxDouble dx, wxDouble dy);
    wxRect2DDouble& Inflate(const wxSize& d)
        { return Inflate(static_cast<wxDouble>(d.x), static_cast<wxDouble>(d.y)); }
    wxRect2DDouble& Inflate(wxDouble d) { return Inflate(d, d); }
    wxRect2DDouble Inflate(wxDouble dx, wxDouble dy) const
    {
        wxRect2DDouble r = *this;
        r.Inflate(dx, dy);
        return r;
    }

    wxRect2DDouble& Deflate(wxDouble dx, wxDouble dy) { return Inflate(-dx, -dy); }
    wxRect2DDouble& Deflate(const wxSize& d)
        { return Inflate(-static_cast<wxDouble>(d.x), -static_cast<wxDouble>(d.y)); }
    wxRect2DDouble& Deflate(wxDouble d) { return Inflate(-d); }
    wxRect2DDouble Deflate(wxDouble dx, wxDouble dy) const
    {
        wxRect2DDouble r = *this;
        r.Deflate(dx, dy);
        return r;
    }

    void ConstrainTo( const wxRect2DDouble &rect );

    wxNODISCARD wxPoint2DDouble Interpolate( wxInt32 widthfactor, wxInt32 heightfactor ) const
        { return wxPoint2DDouble( m_x + m_width * widthfactor , m_y + m_height * heightfactor ); }

    static void Intersect( const wxRect2DDouble &src1 , const wxRect2DDouble &src2 , wxRect2DDouble *dest );
    inline void Intersect( const wxRect2DDouble &otherRect )
        { Intersect( *this , otherRect , this ); }
    inline wxRect2DDouble CreateIntersection( const wxRect2DDouble &otherRect ) const
        { wxRect2DDouble result; Intersect( *this , otherRect , &result); return result; }
    wxNODISCARD bool Intersects( const wxRect2DDouble &rect ) const;

    static void Union( const wxRect2DDouble &src1 , const wxRect2DDouble &src2 , wxRect2DDouble *dest );
    void Union( const wxRect2DDouble &otherRect )
        { Union( *this , otherRect , this ); }
    void Union( const wxPoint2DDouble &pt );
    inline wxRect2DDouble CreateUnion( const wxRect2DDouble &otherRect ) const
        { wxRect2DDouble result; Union( *this , otherRect , &result); return result; }

    inline void Scale( wxDouble f )
        { m_x *= f; m_y *= f; m_width *= f; m_height *= f;}
    inline void Scale( wxInt32 num , wxInt32 denum )
        { m_x *= ((wxDouble)num)/((wxDouble)denum); m_y *= ((wxDouble)num)/((wxDouble)denum);
                m_width *= ((wxDouble)num)/((wxDouble)denum); m_height *= ((wxDouble)num)/((wxDouble)denum);}

    inline bool operator == (const wxRect2DDouble& rect) const
        { return wxIsSameDouble(m_x, rect.m_x) && wxIsSameDouble(m_y, rect.m_y) && HaveEqualSize(rect); }
    inline bool operator != (const wxRect2DDouble& rect) const
        { return !(*this == rect); }

    wxDouble m_x = 0.0;
    wxDouble m_y = 0.0;
    wxDouble m_width = 0.0;
    wxDouble m_height = 0.0;
};


// wxRect2Ds are a axis-aligned rectangles, each side of the rect is parallel to the x- or m_y- axis. The rectangle is either defined by the
// top left and bottom right corner, or by the top left corner and size. A point is contained within the rectangle if
// left <= x < right  and top <= m_y < bottom , thus it is a half open interval.

class WXDLLIMPEXP_CORE wxRect2DInt
{
public:
       wxRect2DInt() = default;
       wxRect2DInt( const wxRect& r ) { m_x = r.x ; m_y = r.y ; m_width = r.width ; m_height = r.height ; }
       wxRect2DInt(wxInt32 x, wxInt32 y, wxInt32 w, wxInt32 h) { m_x = x; m_y = y; m_width = w;  m_height = h; }
       wxRect2DInt(const wxPoint2DInt& topLeft, const wxPoint2DInt& bottomRight);
       inline wxRect2DInt(const wxPoint2DInt& pos, const wxSize& size);
    // default copy ctor and copy-assign operator are OK

        // single attribute accessors

       wxPoint2DInt GetPosition() const { return wxPoint2DInt(m_x, m_y); }
       wxSize GetSize() const { return wxSize(m_width, m_height); }

        // for the edge and corner accessors there are two setters counterparts, the Set.. functions keep the other corners at their
        // position whenever sensible, the Move.. functions keep the size of the rect and move the other corners appropriately

      inline wxInt32 GetLeft() const { return m_x; }
       inline void SetLeft( wxInt32 n ) { m_width += m_x - n; m_x = n; }
       inline void MoveLeftTo( wxInt32 n ) { m_x = n; }
       inline wxInt32 GetTop() const { return m_y; }
       inline void SetTop( wxInt32 n ) { m_height += m_y - n; m_y = n; }
       inline void MoveTopTo( wxInt32 n ) { m_y = n; }
       inline wxInt32 GetBottom() const { return m_y + m_height; }
       inline void SetBottom( wxInt32 n ) { m_height += n - (m_y+m_height);}
       inline void MoveBottomTo( wxInt32 n ) { m_y = n - m_height; }
       inline wxInt32 GetRight() const { return m_x + m_width; }
       inline void SetRight( wxInt32 n ) { m_width += n - (m_x+m_width) ; }
       inline void MoveRightTo( wxInt32 n ) { m_x = n - m_width; }

        inline wxPoint2DInt GetLeftTop() const { return wxPoint2DInt( m_x , m_y ); }
        inline void SetLeftTop( const wxPoint2DInt &pt ) { m_width += m_x - pt.m_x; m_height += m_y - pt.m_y; m_x = pt.m_x; m_y = pt.m_y; }
        inline void MoveLeftTopTo( const wxPoint2DInt &pt ) { m_x = pt.m_x; m_y = pt.m_y; }
        inline wxPoint2DInt GetLeftBottom() const { return wxPoint2DInt( m_x , m_y + m_height ); }
        inline void SetLeftBottom( const wxPoint2DInt &pt ) { m_width += m_x - pt.m_x; m_height += pt.m_y - (m_y+m_height) ; m_x = pt.m_x; }
        inline void MoveLeftBottomTo( const wxPoint2DInt &pt ) { m_x = pt.m_x; m_y = pt.m_y - m_height; }
        inline wxPoint2DInt GetRightTop() const { return wxPoint2DInt( m_x+m_width , m_y ); }
        inline void SetRightTop( const wxPoint2DInt &pt ) { m_width += pt.m_x - ( m_x + m_width ); m_height += m_y - pt.m_y; m_y = pt.m_y; }
        inline void MoveRightTopTo( const wxPoint2DInt &pt ) { m_x = pt.m_x - m_width; m_y = pt.m_y; }
        inline wxPoint2DInt GetRightBottom() const { return wxPoint2DInt( m_x+m_width , m_y + m_height ); }
        inline void SetRightBottom( const wxPoint2DInt &pt ) { m_width += pt.m_x - ( m_x + m_width ); m_height += pt.m_y - (m_y+m_height);}
        inline void MoveRightBottomTo( const wxPoint2DInt &pt ) { m_x = pt.m_x - m_width; m_y = pt.m_y - m_height; }
        inline wxPoint2DInt GetCentre() const { return wxPoint2DInt( m_x+m_width/2 , m_y+m_height/2 ); }
        inline void SetCentre( const wxPoint2DInt &pt ) { MoveCentreTo( pt ); }    // since this is impossible without moving...
        inline void MoveCentreTo( const wxPoint2DInt &pt ) { m_x += pt.m_x - (m_x+m_width/2); m_y += pt.m_y -(m_y+m_height/2); }
        inline wxOutCode GetOutCode( const wxPoint2DInt &pt ) const
            { return (wxOutCode) (( ( pt.m_x < m_x ) ? wxOutLeft : 0 ) +
                     ( ( pt.m_x >= m_x + m_width ) ? wxOutRight : 0 ) +
                     ( ( pt.m_y < m_y ) ? wxOutTop : 0 )  +
                     ( ( pt.m_y >= m_y + m_height ) ? wxOutBottom : 0 )); }
        inline wxOutCode GetOutcode( const wxPoint2DInt &pt ) const
            { return GetOutCode( pt ) ; }
        inline bool Contains( const wxPoint2DInt &pt ) const
            { return  GetOutCode( pt ) == wxInside; }
        inline bool Contains( const wxRect2DInt &rect ) const
            { return ( ( ( m_x <= rect.m_x ) && ( rect.m_x + rect.m_width <= m_x + m_width ) ) &&
                ( ( m_y <= rect.m_y ) && ( rect.m_y + rect.m_height <= m_y + m_height ) ) ); }
        inline bool IsEmpty() const
            { return ( m_width <= 0 || m_height <= 0 ); }
        inline bool HaveEqualSize( const wxRect2DInt &rect ) const
            { return ( rect.m_width == m_width && rect.m_height == m_height ); }

        inline void Inset( wxInt32 x , wxInt32 y ) { m_x += x; m_y += y; m_width -= 2 * x; m_height -= 2 * y; }
        inline void Inset( wxInt32 left , wxInt32 top ,wxInt32 right , wxInt32 bottom  )
            { m_x += left; m_y += top; m_width -= left + right; m_height -= top + bottom;}
        inline void Offset( const wxPoint2DInt &pt ) { m_x += pt.m_x; m_y += pt.m_y; }
        void ConstrainTo( const wxRect2DInt &rect );
        wxPoint2DInt Interpolate( wxInt32 widthfactor, wxInt32 heightfactor ) const
            { return wxPoint2DInt( m_x + m_width * widthfactor, m_y + m_height * heightfactor ); }

        static void Intersect( const wxRect2DInt &src1 , const wxRect2DInt &src2 , wxRect2DInt *dest );
        inline void Intersect( const wxRect2DInt &otherRect ) { Intersect( *this , otherRect , this ); }
        inline wxRect2DInt CreateIntersection( const wxRect2DInt &otherRect ) const { wxRect2DInt result; Intersect( *this , otherRect , &result); return result; }
        bool Intersects( const wxRect2DInt &rect ) const;

        static void Union( const wxRect2DInt &src1 , const wxRect2DInt &src2 , wxRect2DInt *dest );
        void Union( const wxRect2DInt &otherRect )  { Union( *this , otherRect , this ); }
        void Union( const wxPoint2DInt &pt );
        inline wxRect2DInt CreateUnion( const wxRect2DInt &otherRect ) const { wxRect2DInt result; Union( *this , otherRect , &result); return result; }

        inline void Scale( wxInt32 f ) { m_x *= f; m_y *= f; m_width *= f; m_height *= f;}
        inline void Scale( wxInt32 num , wxInt32 denum )
            { m_x *= ((wxInt32)num)/((wxInt32)denum); m_y *= ((wxInt32)num)/((wxInt32)denum);
                m_width *= ((wxInt32)num)/((wxInt32)denum); m_height *= ((wxInt32)num)/((wxInt32)denum);}

       bool operator == (const wxRect2DInt& rect) const;
       bool operator != (const wxRect2DInt& rect) const;

#if wxUSE_STREAMS
       void WriteTo( wxDataOutputStream &stream ) const;
       void ReadFrom( wxDataInputStream &stream );
#endif // wxUSE_STREAMS

       wxInt32 m_x = 0;
       wxInt32 m_y = 0;
       wxInt32 m_width = 0;
       wxInt32 m_height = 0;
};

inline wxRect2DInt::wxRect2DInt( const wxPoint2DInt &a , const wxPoint2DInt &b)
{
    m_x = wxMin( a.m_x , b.m_x );
    m_y = wxMin( a.m_y , b.m_y );
    m_width = abs( a.m_x - b.m_x );
    m_height = abs( a.m_y - b.m_y );
}

inline wxRect2DInt::wxRect2DInt( const wxPoint2DInt& pos, const wxSize& size)
{
    m_x = pos.m_x;
    m_y = pos.m_y;
    m_width = size.x;
    m_height = size.y;
}

inline bool wxRect2DInt::operator == (const wxRect2DInt& rect) const
{
    return (m_x==rect.m_x && m_y==rect.m_y &&
            m_width==rect.m_width && m_height==rect.m_height);
}

inline bool wxRect2DInt::operator != (const wxRect2DInt& rect) const
{
    return !(*this == rect);
}

class WXDLLIMPEXP_CORE wxTransform2D
{
public :
    virtual ~wxTransform2D() = default;
    virtual void                    Transform( wxPoint2DInt* pt )const  = 0;
    virtual void                    Transform( wxRect2DInt* r ) const;
    virtual wxPoint2DInt    Transform( const wxPoint2DInt &pt ) const;
    virtual wxRect2DInt        Transform( const wxRect2DInt &r ) const ;

    virtual void                    InverseTransform( wxPoint2DInt* pt ) const  = 0;
    virtual void                    InverseTransform( wxRect2DInt* r ) const ;
    virtual wxPoint2DInt    InverseTransform( const wxPoint2DInt &pt ) const ;
    virtual wxRect2DInt        InverseTransform( const wxRect2DInt &r ) const ;
};


#endif // wxUSE_GEOMETRY

#endif // _WX_GEOMETRY_H_
