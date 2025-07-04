/******************************************************************************\
 * Copyright (c) 2004-2025
 *
 * Author(s):
 *  Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once
#include <vector>
#include <algorithm>
#ifdef _WIN32
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    include <windows.h>
#    include <mmsystem.h>
#elif defined( __APPLE__ ) || defined( __MACOSX )
// using mach timers for Mac
#    include <mach/mach.h>
#    include <mach/mach_error.h>
#    include <mach/mach_time.h>
#else
// using mach nanosleep for Linux
#    include <sys/time.h>
#endif
#include <QCoreApplication>
#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>
#include <QTranslator>
#include <QLibraryInfo>
#include <QUrl>
#include <QLocale>
#include <QElapsedTimer>
#include <QTextBoundaryFinder>
#include <QTimer>
#ifndef CLIENT_NO_SRV_CONNECT
#    include <QDnsLookup>
#endif
#ifndef _WIN32
#    include <QThread>
#endif
#ifndef HEADLESS
#    include <QMessageBox>
#    include <QMenu>
#    include <QWhatsThis>
#    include <QTextBrowser>
#    include <QLabel>
#    include <QCheckBox>
#    include <QComboBox>
#    include <QLineEdit>
#    include <QDateTime>
#    include <QDesktopServices>
#    include <QKeyEvent>
#    include <QStackedLayout>
#    include "ui_aboutdlgbase.h"
#endif

#include "global.h"

#ifndef SERVER_ONLY
class CClient; // forward declaration of CClient
#endif

/* Definitions ****************************************************************/
#define METER_FLY_BACK             2
#define INVALID_MIDI_CH            -1 // invalid MIDI channel definition
#define DNS_SRV_RESOLVE_TIMEOUT_MS 500

/* Global functions ***********************************************************/
// converting float to short
inline short Float2Short ( const float fInput )
{
    // lower bound
    if ( fInput < _MINSHORT )
    {
        return _MINSHORT;
    }

    // upper bound
    if ( fInput > _MAXSHORT )
    {
        return _MAXSHORT;
    }

    return static_cast<short> ( fInput );
}

// calculate the bit rate in bits per second from the number of coded bytes
inline int CalcBitRateBitsPerSecFromCodedBytes ( const int iCeltNumCodedBytes, const int iFrameSize )
{
    return ( SYSTEM_SAMPLE_RATE_HZ * iCeltNumCodedBytes * 8 ) / iFrameSize;
}

QString GetVersionAndNameStr ( const bool bDisplayInGui = true );
QString MakeClientNameTitle ( QString win, QString client );
QString TruncateString ( QString str, int position );

/******************************************************************************\
* CVector Base Class                                                           *
\******************************************************************************/
template<class TData>
class CVector : public std::vector<TData>
{
public:
    CVector() {}
    CVector ( const int iNeSi ) { Init ( iNeSi ); }
    CVector ( const int iNeSi, const TData tInVa ) { Init ( iNeSi, tInVa ); }

    CVector ( CVector const& )            = default;
    CVector& operator= ( CVector const& ) = default;

    void Init ( const int iNewSize );

    // use this init to give all elements a defined value
    void Init ( const int iNewSize, const TData tIniVal );

    // set all values to the given reset value
    void Reset ( const TData tResetVal ) { std::fill ( this->begin(), this->end(), tResetVal ); }

    void Enlarge ( const int iAddedSize ) { std::vector<TData>::resize ( std::vector<TData>::size() + iAddedSize ); }

    void Add ( const TData& tI )
    {
        Enlarge ( 1 );
        std::vector<TData>::back() = tI;
    }

    int StringFiFoWithCompare ( const QString strNewValue, const bool bDoAdding = true );

    // this function simply converts the type of size to integer
    inline int Size() const { return static_cast<int> ( std::vector<TData>::size() ); }
};

/* Implementation *************************************************************/
template<class TData>
void CVector<TData>::Init ( const int iNewSize )
{
    // clear old buffer and reserve memory for new buffer
    std::vector<TData>::clear();
    std::vector<TData>::resize ( iNewSize );
}

template<class TData>
void CVector<TData>::Init ( const int iNewSize, const TData tIniVal )
{
    // call actual init routine and reset all values to the given value
    Init ( iNewSize );
    Reset ( tIniVal );
}

// note: this is only supported for string vectors
template<class TData>
int CVector<TData>::StringFiFoWithCompare ( const QString strNewValue, const bool bDoAdding )
{
    const int iVectorSize = Size();

    CVector<QString> vstrTempList ( iVectorSize, "" );

    // init with illegal index per definition
    int iOldIndex = INVALID_INDEX;

    // init temporary list count (may be overwritten later on)
    int iTempListCnt = 0;

    if ( bDoAdding )
    {
        // store the new element in the current storage list at
        // the top, make sure we do not have more than allowed stored
        // elements
        vstrTempList[0] = strNewValue;
        iTempListCnt    = 1;
    }

    for ( int iIdx = 0; iIdx < iVectorSize; iIdx++ )
    {
        // first check if we still have space in our data storage
        if ( iTempListCnt < iVectorSize )
        {
            // only add old element if it is not the same as the
            // selected one
            if ( std::vector<TData>::operator[] ( iIdx ).compare ( strNewValue ) )
            {
                vstrTempList[iTempListCnt] = std::vector<TData>::operator[] ( iIdx );

                iTempListCnt++;
            }
            else
            {
                iOldIndex = iIdx;
            }
        }
    }

    // copy new generated list to data base
    *this = vstrTempList;

    return iOldIndex;
}

/******************************************************************************\
* CFIFO Class (First In, First Out)                                            *
\******************************************************************************/
template<class TData>
class CFIFO : public CVector<TData>
{
public:
    CFIFO() : iCurIdx ( 0 ) {}
    CFIFO ( const int iNeSi ) : CVector<TData> ( iNeSi ), iCurIdx ( 0 ) {}
    CFIFO ( const int iNeSi, const TData tInVa ) : CVector<TData> ( iNeSi, tInVa ), iCurIdx ( 0 ) {}

    void         Add ( const TData tNewD );
    inline TData Get() { return CVector<TData>::operator[] ( iCurIdx ); }

    virtual void Init ( const int iNewSize );

    virtual void Init ( const int iNewSize, const TData tIniVal );

protected:
    int iCurIdx;
};

template<class TData>
void CFIFO<TData>::Init ( const int iNewSize )
{
    iCurIdx = 0;
    CVector<TData>::Init ( iNewSize );
}

template<class TData>
void CFIFO<TData>::Init ( const int iNewSize, const TData tIniVal )
{
    iCurIdx = 0;
    CVector<TData>::Init ( iNewSize, tIniVal );
}

template<class TData>
void CFIFO<TData>::Add ( const TData tNewD )
{
    CVector<TData>::operator[] ( iCurIdx ) = tNewD;

    // increment index and check for wrap around
    iCurIdx++;

    if ( iCurIdx >= CVector<TData>::Size() )
    {
        iCurIdx = 0;
    }
}

/******************************************************************************\
* CMovingAv Class (Moving Average)                                             *
\******************************************************************************/
template<class TData>
class CMovingAv : public CVector<TData>
{
public:
    CMovingAv() : CVector<TData>(), iCurIdx ( 0 ), iNorm ( 0 ), dCurAvResult ( 0 ), dNoDataResult ( 0 ) {}

    void Add ( const TData tNewD );

    void Init ( const int iNewSize, const double dNNoDRes = 0 );

    void Reset();

    inline double GetAverage()
    {
        // make sure we do not divide by zero
        if ( iNorm == 0 )
        {
            return dNoDataResult;
        }
        else
        {
            return dCurAvResult / iNorm;
        }
    }

    double InitializationState() const
    {
        // make sure we do not divide by zero
        if ( CVector<TData>::Size() != 0 )
        {
            return static_cast<double> ( iNorm ) / CVector<TData>::Size();
        }
        else
        {
            return 0;
        }
    }

protected:
    int    iCurIdx;
    int    iNorm;
    double dCurAvResult;
    double dNoDataResult;
};

template<class TData>
void CMovingAv<TData>::Init ( const int iNewSize, const double dNNoDRes )
{
    iNorm         = 0;
    iCurIdx       = 0;
    dCurAvResult  = 0; // only for scalars!
    dNoDataResult = dNNoDRes;
    CVector<TData>::Init ( iNewSize );
}

template<class TData>
void CMovingAv<TData>::Reset()
{
    iNorm        = 0;
    iCurIdx      = 0;
    dCurAvResult = 0; // only for scalars!
    CVector<TData>::Reset ( TData ( 0 ) );
}

template<class TData>
void CMovingAv<TData>::Add ( const TData tNewD )
{
    /*
        Optimized calculation of the moving average. We only add a new value and
        subtract the old value from the result. We only need one addition and a
        history buffer.
    */

    // subtract oldest value
    dCurAvResult -= CVector<TData>::operator[] ( iCurIdx );

    // add new value and write in memory
    dCurAvResult += tNewD;
    CVector<TData>::operator[] ( iCurIdx ) = tNewD;

    // increase position pointer and test if wrap
    iCurIdx++;
    if ( iCurIdx >= CVector<TData>::Size() )
    {
        iCurIdx = 0;
    }

    // take care of norm
    if ( iNorm < CVector<TData>::Size() )
    {
        iNorm++;
    }
}

/******************************************************************************\
* GUI Utilities                                                                *
\******************************************************************************/
#ifndef HEADLESS
// Dialog base class -----------------------------------------------------------
class CBaseDlg : public QDialog
{
    Q_OBJECT

public:
    CBaseDlg ( QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() ) : QDialog ( parent, flags ) {}

public slots:
    void keyPressEvent ( QKeyEvent* pEvent )
    {
        // block escape key
        if ( pEvent->key() != Qt::Key_Escape )
        {
#    ifdef ANDROID
            if ( pEvent->key() == Qt::Key_Back )
            {
                close(); // otherwise, dialog does not show properly again in android (nefarius2001, #832)
                return;
            }
#    endif
            QDialog::keyPressEvent ( pEvent );
        }
    }
};

// About dialog ----------------------------------------------------------------
class CAboutDlg : public CBaseDlg, private Ui_CAboutDlgBase
{
    Q_OBJECT

public:
    CAboutDlg ( QWidget* parent = nullptr );
};

// Licence dialog --------------------------------------------------------------
class CLicenceDlg : public CBaseDlg
{
    Q_OBJECT

public:
    CLicenceDlg ( QWidget* parent = nullptr );

protected:
    QPushButton* butAccept;

public slots:
    void OnAgreeStateChanged ( int value ) { butAccept->setEnabled ( value == Qt::Checked ); }
};

// Help menu -------------------------------------------------------------------
class CHelpMenu : public QMenu
{
    Q_OBJECT

public:
    CHelpMenu ( const bool bIsClient, QWidget* parent = nullptr );

protected:
    CAboutDlg AboutDlg;

public slots:
    void OnHelpWhatsThis() { QWhatsThis::enterWhatsThisMode(); }
    void OnHelpAbout() { AboutDlg.exec(); }
    void OnHelpAboutQt() { QMessageBox::aboutQt ( nullptr, QString ( tr ( "About Qt" ) ) ); }
    void OnHelpClientGetStarted() { QDesktopServices::openUrl ( QUrl ( CLIENT_GETTING_STARTED_URL ) ); }
    void OnHelpServerGetStarted() { QDesktopServices::openUrl ( QUrl ( SERVER_GETTING_STARTED_URL ) ); }
    void OnHelpSoftwareMan() { QDesktopServices::openUrl ( QUrl ( SOFTWARE_MANUAL_URL ) ); }
};

// Language combo box ----------------------------------------------------------
class CLanguageComboBox : public QComboBox
{
    Q_OBJECT

public:
    CLanguageComboBox ( QWidget* parent = nullptr );

    void Init ( QString& strSelLanguage );

protected:
    int iIdxSelectedLanguage;

public slots:
    void OnLanguageActivated ( int iLanguageIdx );

signals:
    void LanguageChanged ( QString strLanguage );
};

// StackedLayout which auto-reduces to the size of the currently visible widget
class CMinimumStackedLayout : public QStackedLayout
{
    Q_OBJECT
public:
    CMinimumStackedLayout ( QWidget* parent = nullptr ) : QStackedLayout ( parent ) {}
    virtual QSize sizeHint() const override;
};
#endif

/******************************************************************************\
* Other Classes/Enums                                                          *
\******************************************************************************/
// Audio channel configuration -------------------------------------------------
enum EAudChanConf
{
    // used for settings -> enum values should be fixed
    CC_MONO               = 0,
    CC_MONO_IN_STEREO_OUT = 1,
    CC_STEREO             = 2
};

// Audio compression type enum -------------------------------------------------
enum EAudComprType
{
    // used for protocol -> enum values must be fixed!
    CT_NONE   = 0,
    CT_CELT   = 1,
    CT_OPUS   = 2,
    CT_OPUS64 = 3 // using OPUS with 64 samples frame size
};

// Network transport flags -----------------------------------------------------
enum ENetwFlags
{
    // used for protocol -> enum values must be fixed!
    NF_NONE         = 0,
    NF_WITH_COUNTER = 1 // using a network counter to correctly order UDP packets in jitter buffer
};

// Audio quality enum ----------------------------------------------------------
enum EAudioQuality
{
    // used for settings and the comobo box index -> enum values must be fixed!
    AQ_LOW    = 0,
    AQ_NORMAL = 1,
    AQ_HIGH   = 2
};

// Get data status enum --------------------------------------------------------
enum EGetDataStat
{
    GS_BUFFER_OK,
    GS_BUFFER_UNDERRUN,
    GS_CHAN_NOW_DISCONNECTED,
    GS_CHAN_NOT_CONNECTED
};

// GUI design enum -------------------------------------------------------------
enum EGUIDesign
{
    // used for settings -> enum values should be fixed
    GD_STANDARD  = 0,
    GD_ORIGINAL  = 1,
    GD_SLIMFADER = 2
};

// MeterStyle enum -------------------------------------------------------------
enum EMeterStyle
{
    // used for settings -> enum values should be fixed
    MT_BAR_NARROW      = 0,
    MT_BAR_WIDE        = 1,
    MT_LED_STRIPE      = 2,
    MT_LED_ROUND_SMALL = 3,
    MT_LED_ROUND_BIG   = 4
};

// Server licence type enum ----------------------------------------------------
enum ELicenceType
{
    // used for protocol -> enum values must be fixed!
    LT_NO_LICENCE      = 0,
    LT_CREATIVECOMMONS = 1
};

// Server jam recorder state enum ----------------------------------------------
enum ERecorderState
{
    // used for protocol -> enum values must be fixed!
    RS_UNDEFINED       = 0,
    RS_NOT_INITIALISED = 1,
    RS_NOT_ENABLED     = 2,
    RS_RECORDING       = 3
};

// Channel sort type -----------------------------------------------------------
enum EChSortType
{
    // used for settings -> enum values should be fixed
    ST_NO_SORT           = 0,
    ST_BY_NAME           = 1,
    ST_BY_INSTRUMENT     = 2,
    ST_BY_GROUPID        = 3,
    ST_BY_CITY           = 4,
    ST_BY_SERVER_CHANNEL = 5
};

// Directory type --------------------------------------------------------------
enum EDirectoryType
{
    // used for settings -> enum values should be fixed
    AT_NONE                 = -1, // means not registered, "invalid value"
    AT_DEFAULT              = 0,
    AT_ANY_GENRE2           = 1,
    AT_ANY_GENRE3           = 2,
    AT_GENRE_ROCK           = 3,
    AT_GENRE_JAZZ           = 4,
    AT_GENRE_CLASSICAL_FOLK = 5,
    AT_GENRE_CHORAL         = 6,
    AT_CUSTOM               = 7 // Must be the last entry!
};

inline QString DirectoryTypeToString ( EDirectoryType eAddrType )
{
    switch ( eAddrType )
    {
    case AT_NONE:
        return QCoreApplication::translate ( "CServerDlg", "None" );

    case AT_ANY_GENRE2:
        return QCoreApplication::translate ( "CClientSettingsDlg", "Any Genre 2" );

    case AT_ANY_GENRE3:
        return QCoreApplication::translate ( "CClientSettingsDlg", "Any Genre 3" );

    case AT_GENRE_ROCK:
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Rock" );

    case AT_GENRE_JAZZ:
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Jazz" );

    case AT_GENRE_CLASSICAL_FOLK:
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Classical/Folk" );

    case AT_GENRE_CHORAL:
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Choral/Barbershop" );

    case AT_CUSTOM:
        return QCoreApplication::translate ( "CClientSettingsDlg", "Custom" );

    default: // AT_DEFAULT
        return QCoreApplication::translate ( "CClientSettingsDlg", "Any Genre 1" );
    }
}

// Server registration state ---------------------------------------------
enum ESvrRegStatus
{
    SRS_NOT_REGISTERED,
    SRS_BAD_ADDRESS,
    SRS_REQUESTED,
    SRS_TIME_OUT,
    SRS_UNKNOWN_RESP,
    SRS_REGISTERED,
    SRS_SERVER_LIST_FULL,
    SRS_VERSION_TOO_OLD,
    SRS_NOT_FULFILL_REQUIREMENTS
};

inline QString svrRegStatusToString ( ESvrRegStatus eSvrRegStatus )
{
    switch ( eSvrRegStatus )
    {
    case SRS_NOT_REGISTERED:
        return QCoreApplication::translate ( "CServerDlg", "Not registered" );

    case SRS_BAD_ADDRESS:
        return QCoreApplication::translate ( "CServerDlg", "Bad address" );

    case SRS_REQUESTED:
        return QCoreApplication::translate ( "CServerDlg", "Registration requested" );

    case SRS_TIME_OUT:
        return QCoreApplication::translate ( "CServerDlg", "Registration failed" );

    case SRS_UNKNOWN_RESP:
        return QCoreApplication::translate ( "CServerDlg", "Check server version" );

    case SRS_REGISTERED:
        return QCoreApplication::translate ( "CServerDlg", "Registered" );

    case SRS_SERVER_LIST_FULL:
        return QCoreApplication::translate ( "CServerDlg", "Server list full at directory" );

    case SRS_VERSION_TOO_OLD:
        return QCoreApplication::translate ( "CServerDlg", "Your server version is too old" );

    case SRS_NOT_FULFILL_REQUIREMENTS:
        return QCoreApplication::translate ( "CServerDlg", "Requirements not fulfilled" );
    }

    return QString ( QCoreApplication::translate ( "CServerDlg", "Unknown value %1" ) ).arg ( eSvrRegStatus );
}

// Directory registration outcome ----------------------------------------------
enum ESvrRegResult
{
    // used for protocol -> enum values must be fixed!
    SRR_REGISTERED              = 0,
    SRR_SERVER_LIST_FULL        = 1,
    SRR_VERSION_TOO_OLD         = 2,
    SRR_NOT_FULFILL_REQIREMENTS = 3
};

// Skill level enum ------------------------------------------------------------
enum ESkillLevel
{
    // used for protocol -> enum values must be fixed!
    SL_NOT_SET      = 0,
    SL_BEGINNER     = 1,
    SL_INTERMEDIATE = 2,
    SL_PROFESSIONAL = 3
};

// define the GUI RGB colors for each skill level
#define RGBCOL_R_SL_NOT_SET         255
#define RGBCOL_G_SL_NOT_SET         255
#define RGBCOL_B_SL_NOT_SET         255
#define RGBCOL_R_SL_BEGINNER        255
#define RGBCOL_G_SL_BEGINNER        255
#define RGBCOL_B_SL_BEGINNER        200
#define RGBCOL_R_SL_INTERMEDIATE    225
#define RGBCOL_G_SL_INTERMEDIATE    255
#define RGBCOL_B_SL_INTERMEDIATE    225
#define RGBCOL_R_SL_SL_PROFESSIONAL 255
#define RGBCOL_G_SL_SL_PROFESSIONAL 225
#define RGBCOL_B_SL_SL_PROFESSIONAL 225

// Stereo signal level meter ---------------------------------------------------
class CStereoSignalLevelMeter
{
public:
    //### TODO: BEGIN ###//
    // Calculate smoothing factor from sample rate and frame size (64 or 128 samples frame size).
    //      But tests with 128 and 64 samples frame size have shown that the meter fly back
    //      is ok for both numbers of samples frame size with a factor of 0.99.
    //### TODO: END ###//

    CStereoSignalLevelMeter ( const bool bNIsStereoOut = true, const double dNSmoothingFactor = 0.99 ) :
        dSmoothingFactor ( dNSmoothingFactor ),
        bIsStereoOut ( bNIsStereoOut )
    {
        Reset();
    }

    void Update ( const CVector<short>& vecsAudio, const int iInSize, const bool bIsStereoIn );

    double        GetLevelForMeterdBLeftOrMono() { return CalcLogResultForMeter ( dCurLevelLOrMono ); }
    double        GetLevelForMeterdBRight() { return CalcLogResultForMeter ( dCurLevelR ); }
    static double CalcLogResultForMeter ( const double& dLinearLevel );

    void Reset()
    {
        dCurLevelLOrMono = 0.0;
        dCurLevelR       = 0.0;
    }

protected:
    double UpdateCurLevel ( double dCurLevel, const double dMax );

    double dCurLevelLOrMono;
    double dCurLevelR;
    double dSmoothingFactor;
    bool   bIsStereoOut;
};

// Host address ----------------------------------------------------------------
class CHostAddress
{
public:
    enum EStringMode
    {
        SM_IP_PORT,
        SM_IP_NO_LAST_BYTE,
        SM_IP_NO_LAST_BYTE_PORT
    };

    CHostAddress() : InetAddr ( static_cast<quint32> ( 0 ) ), iPort ( 0 ) {}

    CHostAddress ( const QHostAddress& NInetAddr, const quint16 iNPort ) : InetAddr ( NInetAddr ), iPort ( iNPort ) {}

    CHostAddress ( const CHostAddress& NHAddr ) : InetAddr ( NHAddr.InetAddr ), iPort ( NHAddr.iPort ) {}

    // copy operator
    CHostAddress& operator= ( const CHostAddress& NHAddr )
    {
        InetAddr = NHAddr.InetAddr;
        iPort    = NHAddr.iPort;
        return *this;
    }

    // compare operator
    bool operator== ( const CHostAddress& CompAddr ) const { return ( ( CompAddr.InetAddr == InetAddr ) && ( CompAddr.iPort == iPort ) ); }

    int Compare ( const CHostAddress& other ) const;

    QString toString ( const EStringMode eStringMode = SM_IP_PORT ) const;

    QHostAddress InetAddr;
    quint16      iPort;
};

// Instrument picture data base ------------------------------------------------
// this is a pure static class
class CInstPictures
{
public:
    enum EInstCategory
    {
        IC_OTHER_INSTRUMENT,
        IC_WIND_INSTRUMENT,
        IC_STRING_INSTRUMENT,
        IC_PLUCKING_INSTRUMENT,
        IC_PERCUSSION_INSTRUMENT,
        IC_KEYBOARD_INSTRUMENT,
        IC_MULTIPLE_INSTRUMENT
    };

    // per definition: the very first instrument is the "not used" instrument
    static int  GetNotUsedInstrument() { return 0; }
    static bool IsNotUsedInstrument ( const int iInstrument ) { return iInstrument == 0; }

    static int           GetNumAvailableInst() { return GetTable().Size(); }
    static QString       GetResourceReference ( const int iInstrument );
    static QString       GetName ( const int iInstrument );
    static EInstCategory GetCategory ( const int iInstrument );
    static void          UpdateTableOnLanguageChange() { GetTable ( true ); }

    //### TODO: BEGIN ###//
    // make use of instrument category (not yet implemented)
    //### TODO: END ###//

protected:
    class CInstPictProps
    {
    public:
        CInstPictProps() : strName ( "" ), strResourceReference ( "" ), eInstCategory ( IC_OTHER_INSTRUMENT ) {}

        CInstPictProps ( const QString NsName, const QString NsResRef, const EInstCategory NeInstCat ) :
            strName ( NsName ),
            strResourceReference ( NsResRef ),
            eInstCategory ( NeInstCat )
        {}

        QString       strName;
        QString       strResourceReference;
        EInstCategory eInstCategory;
    };

    static bool                     IsInstIndexInRange ( const int iIdx );
    static CVector<CInstPictProps>& GetTable ( const bool bReGenerateTable = false );
};

// Locale management class -----------------------------------------------------
class CLocale
{
public:
    static QString                 GetCountryFlagIconsResourceReference ( const QLocale::Country eCountry /* Always a Qt5 (!) code */ );
    static QMap<QString, QString>  GetAvailableTranslations();
    static QPair<QString, QString> FindSysLangTransFileName ( const QMap<QString, QString>& TranslMap );
    static void                    LoadTranslation ( const QString strLanguage, QCoreApplication* pApp );
    static QLocale::Country        WireFormatCountryCodeToQtCountry ( unsigned short iCountryCode );
    static unsigned short          QtCountryToWireFormatCountryCode ( const QLocale::Country eCountry );
    static bool                    IsCountryCodeSupported ( unsigned short iCountryCode );
    static QLocale::Country        GetCountryCodeByTwoLetterCode ( QString sTwoLetterCode );
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // ./tools/qt5_to_qt6_country_code_table.py generates these lists:
    constexpr int const static wireFormatToQt6Table[] = {
        0,   1,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
        29,  30,  31,  32,  33,  35,  36,  37,  38,  39,  40,  41,  43,  45,  46,  48,  49,  50,  51,  53,  54,  55,  57,  56,  58,  59,  118,
        60,  61,  63,  64,  65,  67,  68,  69,  232, 70,  71,  72,  73,  74,  75,  77,  80,  81,  82,  83,  84,  100, 85,  86,  87,  88,  89,
        90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  102, 101, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 116, 117, 119,
        120, 122, 123, 124, 125, 174, 218, 127, 128, 129, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147,
        148, 149, 150, 151, 152, 153, 154, 155, 156, 158, 159, 160, 161, 162, 163, 164, 165, 62,  166, 167, 168, 170, 169, 171, 172, 173, 175,
        176, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 197, 198, 201, 202, 203, 204, 205, 206, 208,
        209, 210, 212, 213, 214, 215, 216, 217, 220, 221, 196, 200, 222, 223, 224, 76,  225, 226, 227, 228, 229, 230, 231, 233, 234, 235, 236,
        238, 239, 240, 241, 242, 243, 244, 245, 246, 248, 247, 250, 251, 252, 253, 254, 255, 34,  249, 256, 257, 259, 42,  260, 261, 52,  157,
        207, 195, 199, 130, 14,  2,   66,  47,  115, 121, 237, 219, 44,  211, 126, 79,  177, 258, 78,
    };
    constexpr int const static qt6CountryToWireFormat[] = {
        0,   1,   248, 2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  247, 13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
        25,  26,  27,  28,  29,  30,  31,  233, 32,  33,  34,  35,  36,  37,  38,  238, 39,  255, 40,  41,  250, 42,  43,  44,  45,  241, 46,
        47,  48,  50,  49,  51,  52,  54,  55,  152, 56,  57,  58,  249, 59,  60,  61,  63,  64,  65,  66,  67,  68,  204, 69,  261, 258, 70,
        71,  72,  73,  74,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  75,  92,  91,  93,  94,  95,  96,  97,
        98,  99,  100, 101, 102, 103, 104, 251, 105, 106, 53,  107, 108, 252, 109, 110, 111, 112, 257, 115, 116, 117, 246, 118, 119, 120, 121,
        122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 242, 144, 145, 146, 147,
        148, 149, 150, 151, 153, 154, 155, 157, 156, 158, 159, 160, 113, 161, 162, 259, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173,
        174, 175, 176, 177, 178, 179, 244, 199, 180, 181, 245, 200, 182, 183, 184, 185, 186, 187, 243, 188, 189, 190, 256, 191, 192, 193, 194,
        195, 196, 114, 254, 197, 198, 201, 202, 203, 205, 206, 207, 208, 209, 210, 211, 62,  212, 213, 214, 215, 253, 216, 217, 218, 219, 220,
        221, 222, 223, 224, 226, 225, 234, 227, 228, 229, 230, 231, 232, 235, 236, 260, 237, 239, 240,
    };
    constexpr int const static qt6CountryToWireFormatLen = sizeof ( qt6CountryToWireFormat ) / sizeof ( qt6CountryToWireFormat[0] );
#endif
};

// Info of a channel -----------------------------------------------------------
class CChannelCoreInfo
{
public:
    CChannelCoreInfo() :
        strName ( "" ),
        eCountry ( QLocale::AnyCountry ),
        strCity ( "" ),
        iInstrument ( CInstPictures::GetNotUsedInstrument() ),
        eSkillLevel ( SL_NOT_SET )
    {}

    CChannelCoreInfo ( const QString           NsName,
                       const QLocale::Country& NeCountry,
                       const QString&          NsCity,
                       const int               NiInstrument,
                       const ESkillLevel       NeSkillLevel ) :
        strName ( NsName ),
        eCountry ( NeCountry ),
        strCity ( NsCity ),
        iInstrument ( NiInstrument ),
        eSkillLevel ( NeSkillLevel )
    {}

    CChannelCoreInfo ( const CChannelCoreInfo& NCorInf ) :
        strName ( NCorInf.strName ),
        eCountry ( NCorInf.eCountry ),
        strCity ( NCorInf.strCity ),
        iInstrument ( NCorInf.iInstrument ),
        eSkillLevel ( NCorInf.eSkillLevel )
    {}

    // compare operator
    bool operator!= ( const CChannelCoreInfo& CompChanInfo )
    {
        return ( ( CompChanInfo.strName != strName ) || ( CompChanInfo.eCountry != eCountry ) || ( CompChanInfo.strCity != strCity ) ||
                 ( CompChanInfo.iInstrument != iInstrument ) || ( CompChanInfo.eSkillLevel != eSkillLevel ) );
    }

    CChannelCoreInfo& operator= ( const CChannelCoreInfo& ) = default;

    // fader tag text (channel name)
    QString strName;

    // country in which the client is located
    QLocale::Country eCountry;

    // city in which the client is located
    QString strCity;

    // instrument ID of the client (which instrument is he/she playing)
    int iInstrument;

    // skill level of the musician
    ESkillLevel eSkillLevel;
};

class CChannelInfo : public CChannelCoreInfo
{
public:
    CChannelInfo() : HostAddr ( CHostAddress() ), iChanID ( 0 ) {}

    CChannelInfo ( const int NiID, const CChannelCoreInfo& NCorInf, const CHostAddress& Addr = CHostAddress() ) :
        CChannelCoreInfo ( NCorInf ), HostAddr ( Addr ), iChanID ( NiID ) {}

    CChannelInfo ( const int               NiID,
                   const QString&          NsName,
                   const QLocale::Country& NeCountry,
                   const QString&          NsCity,
                   const int               NiInstrument,
                   const ESkillLevel       NeSkillLevel,
                   const CHostAddress&     Addr = CHostAddress() ) :
        CChannelCoreInfo ( NsName, NeCountry, NsCity, NiInstrument, NeSkillLevel ),
        HostAddr ( Addr ),
        iChanID ( NiID )
    {}

    // internet address of the client
    CHostAddress HostAddr;

    // ID of the channel
    int iChanID;
};

// Server info -----------------------------------------------------------------
class CServerCoreInfo
{
public:
    CServerCoreInfo() : strName ( "" ), eCountry ( QLocale::AnyCountry ), strCity ( "" ), iMaxNumClients ( 0 ), bPermanentOnline ( false ) {}

    CServerCoreInfo ( const QString&          NsName,
                      const QLocale::Country& NeCountry,
                      const QString&          NsCity,
                      const int               NiMaxNumClients,
                      const bool              NbPermOnline ) :
        strName ( NsName ),
        eCountry ( NeCountry ),
        strCity ( NsCity ),
        iMaxNumClients ( NiMaxNumClients ),
        bPermanentOnline ( NbPermOnline )
    {}

    // name of the server
    QString strName;

    // country in which the server is located
    QLocale::Country eCountry;

    // city in which the server is located
    QString strCity;

    // maximum number of clients which can connect to the server at the same
    // time
    int iMaxNumClients;

    // is the server permanently online or not (flag)
    bool bPermanentOnline;
};

class CServerInfo : public CServerCoreInfo
{
public:
    CServerInfo() : HostAddr ( CHostAddress() ), LHostAddr ( CHostAddress() ) {}

    CServerInfo ( const CHostAddress&     NHAddr,
                  const CHostAddress&     NLAddr,
                  const QString&          NsName,
                  const QLocale::Country& NeCountry,
                  const QString&          NsCity,
                  const int               NiMaxNumClients,
                  const bool              NbPermOnline ) :
        CServerCoreInfo ( NsName, NeCountry, NsCity, NiMaxNumClients, NbPermOnline ),
        HostAddr ( NHAddr ),
        LHostAddr ( NLAddr )
    {}

    // internet address of the server
    CHostAddress HostAddr;

    // server internal address
    CHostAddress LHostAddr;
};

// Network transport properties ------------------------------------------------
class CNetworkTransportProps
{
public:
    CNetworkTransportProps() :
        iBaseNetworkPacketSize ( 0 ),
        iBlockSizeFact ( 0 ),
        iNumAudioChannels ( 0 ),
        iSampleRate ( 0 ),
        eAudioCodingType ( CT_NONE ),
        eFlags ( NF_NONE ),
        iAudioCodingArg ( 0 )
    {}

    CNetworkTransportProps ( const uint32_t      iNBNPS,
                             const uint16_t      iNBSF,
                             const uint32_t      iNNACH,
                             const uint32_t      iNSR,
                             const EAudComprType eNACT,
                             const ENetwFlags    eNFlags,
                             const int32_t       iNACA ) :
        iBaseNetworkPacketSize ( iNBNPS ),
        iBlockSizeFact ( iNBSF ),
        iNumAudioChannels ( iNNACH ),
        iSampleRate ( iNSR ),
        eAudioCodingType ( eNACT ),
        eFlags ( eNFlags ),
        iAudioCodingArg ( iNACA )
    {}

    uint32_t      iBaseNetworkPacketSize;
    uint16_t      iBlockSizeFact;
    uint32_t      iNumAudioChannels;
    uint32_t      iSampleRate;
    EAudComprType eAudioCodingType;
    ENetwFlags    eFlags;
    int32_t       iAudioCodingArg;
};

// Network utility functions ---------------------------------------------------
class NetworkUtil
{
public:
    static bool ParseNetworkAddressString ( QString strAddress, QHostAddress& InetAddr, bool bEnableIPv6 );

#ifndef CLIENT_NO_SRV_CONNECT
    static bool ParseNetworkAddressSrv ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 );
    static bool ParseNetworkAddressWithSrvDiscovery ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 );
#endif
    static bool ParseNetworkAddress ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 );

    static QString      FixAddress ( const QString& strAddress );
    static CHostAddress GetLocalAddress();
    static CHostAddress GetLocalAddress6();
    static QString      GetDirectoryAddress ( const EDirectoryType eDirectoryType, const QString& strDirectoryAddress );
    static bool         IsPrivateNetworkIP ( const QHostAddress& qhAddr );
};

// Operating system utility functions ------------------------------------------
class COSUtil
{
public:
    enum EOpSystemType
    {
        // used for protocol -> enum values must be fixed!
        OT_WINDOWS = 0,
        OT_MAC_OS  = 1,
        OT_LINUX   = 2,
        OT_ANDROID = 3,
        OT_I_OS    = 4,
        OT_UNIX    = 5
    };

    static QString GetOperatingSystemString ( const EOpSystemType eOSType )
    {
        switch ( eOSType )
        {
        case OT_WINDOWS:
            return "Windows";
        case OT_MAC_OS:
            return "MacOS";
        case OT_LINUX:
            return "Linux";
        case OT_ANDROID:
            return "Android";
        case OT_I_OS:
            return "iOS";
        case OT_UNIX:
            return "Unix";
        default:
            return "Unknown";
        }
    }

    static EOpSystemType GetOperatingSystem()
    {
#ifdef _WIN32
        return OT_WINDOWS;
#elif defined( Q_OS_MACOS )
        return OT_MAC_OS;
#elif defined( Q_OS_IOS )
        return OT_I_OS;
#elif defined( Q_OS_ANDROID ) || defined( ANDROID )
        return OT_ANDROID;
#else
        return OT_LINUX;
#endif
    }
};

// CRC -------------------------------------------------------------------------
class CCRC
{
public:
    CCRC() : iPoly ( ( 1 << 5 ) | ( 1 << 12 ) ), iBitOutMask ( 1 << 16 ) { Reset(); }

    void     Reset();
    void     AddByte ( const uint8_t byNewInput );
    bool     CheckCRC ( const uint32_t iCRC ) { return iCRC == GetCRC(); }
    uint32_t GetCRC();

protected:
    uint32_t iPoly;
    uint32_t iBitOutMask;
    uint32_t iStateShiftReg;
};

// Mathematics utilities -------------------------------------------------------
class MathUtils
{
public:
    static int round ( double x )
    {
        return static_cast<int> ( ( x - floor ( x ) ) >= 0.5 ) ? static_cast<int> ( ceil ( x ) ) : static_cast<int> ( floor ( x ) );
    }

    static void UpDownIIR1 ( double& dOldValue, const double& dNewValue, const double& dWeightUp, const double& dWeightDown )
    {
        // different IIR weights for up and down direction
        if ( dNewValue < dOldValue )
        {
            dOldValue = dOldValue * dWeightDown + ( 1.0 - dWeightDown ) * dNewValue;
        }
        else
        {
            dOldValue = dOldValue * dWeightUp + ( 1.0 - dWeightUp ) * dNewValue;
        }
    }

    static int DecideWithHysteresis ( const double dValue, const int iOldValue, const double dHysteresis )
    {
        // apply hysteresis
        if ( dValue > static_cast<double> ( iOldValue ) )
        {
            return round ( dValue - dHysteresis );
        }
        else
        {
            return round ( dValue + dHysteresis );
        }
    }

    // calculate pan gains: in cross fade mode the pan center is attenuated
    // by 6 dB, otherwise the center equals full gain for both channels
    static inline float GetLeftPan ( const float fPan, const bool bXFade ) { return bXFade ? 1 - fPan : std::min ( 0.5f, 1 - fPan ) * 2; }
    static inline float GetRightPan ( const float fPan, const bool bXFade ) { return bXFade ? fPan : std::min ( 0.5f, fPan ) * 2; }

    // calculate linear gain from fader values which are in dB
    static float CalcFaderGain ( const float fValue )
    {
        // convert actual slider range in gain values
        // and normalize so that maximum gain is 1
        const float fInValueRange0_1 = fValue / AUD_MIX_FADER_MAX;

        // map range from 0..1 to range -35..0 dB and calculate linear gain
        if ( fValue == 0 )
        {
            return 0; // -infinity
        }
        else
        {
            return powf ( 10.0f, ( fInValueRange0_1 - 1.0f ) * AUD_MIX_FADER_RANGE_DB / 20.0f );
        }
    }
};

/******************************************************************************\
* Timing measurement                                                           *
\******************************************************************************/
// intended for debugging the timing jitter of the sound card or server timer
class CTimingMeas
{
public:
    CTimingMeas ( const int iNNMeas, const QString strNFName = "" ) : iNumMeas ( iNNMeas ), vElapsedTimes ( iNNMeas ), strFileName ( strNFName )
    {
        Reset();
    }

    void Reset() { iCnt = INVALID_INDEX; }
    void Measure()
    {
        // exclude the very first measurement (initialization phase)
        if ( iCnt == INVALID_INDEX )
        {
            iCnt = 0;
        }
        else
        {
            // store current measurement
            vElapsedTimes[iCnt++] = ElapsedTimer.nsecsElapsed();

            // reset count if number of measurements are done
            if ( iCnt >= iNumMeas )
            {
                iCnt = 0;

                // store results in a file if file name is given
                if ( !strFileName.isEmpty() )
                {
                    QFile File ( strFileName );

                    if ( File.open ( QIODevice::WriteOnly | QIODevice::Text ) )
                    {
                        QTextStream streamFile ( &File );
                        for ( int i = 0; i < iNumMeas; i++ )
                        {
                            // convert ns in ms and store the value
                            streamFile << i << " " << static_cast<double> ( vElapsedTimes[i] ) / 1000000 << "\n";
                        }
                    }
                }
            }
        }
        ElapsedTimer.start();
    }

protected:
    int           iNumMeas;
    CVector<int>  vElapsedTimes;
    QString       strFileName;
    QElapsedTimer ElapsedTimer;
    int           iCnt;
};

// High resolution timer
#if ( defined( WIN32 ) || defined( _WIN32 ) )
// using QTimer for Windows
class CHighPrecisionTimer : public QObject
{
    Q_OBJECT

public:
    CHighPrecisionTimer ( const bool bNewUseDoubleSystemFrameSize );

    void Start();
    void Stop();
    bool isActive() const { return Timer.isActive(); }

protected:
    QTimer       Timer;
    CVector<int> veciTimeOutIntervals;
    int          iCurPosInVector;
    int          iIntervalCounter;
    bool         bUseDoubleSystemFrameSize;

public slots:
    void OnTimer();

signals:
    void timeout();
};
#else

class CHighPrecisionTimer : public QThread
{
    Q_OBJECT

public:
    CHighPrecisionTimer ( const bool bUseDoubleSystemFrameSize );

    void Start();
    void Stop();
    bool isActive() { return bRun; }

protected:
    virtual void run();

    bool     bRun;

#    if defined( __APPLE__ ) || defined( __MACOSX )
    uint64_t Delay;
    uint64_t NextEnd;
#    else
    long     Delay;
    timespec NextEnd;
#    endif

signals:
    void timeout();
};
#endif

/******************************************************************************\
* Statistics                                                                   *
\******************************************************************************/
// Error rate measurement ------------------------------------------------------
class CErrorRate
{
public:
    CErrorRate() {}

    void Init ( const int iHistoryLength, const bool bNBlockOnDoubleErr = false )
    {
        // initialize buffer (use "no data result" of 1.0 which stands for the
        // worst error rate possible)
        ErrorsMovAvBuf.Init ( iHistoryLength, 1.0 );

        bPreviousState = true;

        // store setting
        bBlockOnDoubleErrors = bNBlockOnDoubleErr;
    }

    void Reset()
    {
        ErrorsMovAvBuf.Reset();
        bPreviousState = true;
    }

    void Update ( const bool bState )
    {
        // if two states were false, do not use the new value
        if ( bBlockOnDoubleErrors && bPreviousState && bState )
        {
            return;
        }

        // add errors as values 0 and 1 to get correct error rate average
        if ( bState )
        {
            ErrorsMovAvBuf.Add ( 1 );
        }
        else
        {
            ErrorsMovAvBuf.Add ( 0 );
        }

        // store state
        bPreviousState = bState;
    }

    double GetAverage() { return ErrorsMovAvBuf.GetAverage(); }
    double InitializationState() { return ErrorsMovAvBuf.InitializationState(); }

protected:
    CMovingAv<char> ErrorsMovAvBuf;
    bool            bBlockOnDoubleErrors;
    bool            bPreviousState;
};
