//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef BANKSWITCH_HXX
#define BANKSWITCH_HXX

#include <map>

#include "FSNode.hxx"
#include "bspf.hxx"

/**
  This class contains all information about the bankswitch schemes supported
  by Stella, as well as convenience functions to map from scheme type to
  readable string, and vice-versa.

  It also includes all logic that determines what a 'valid' rom filename is.
  That is, all extensions that represent valid schemes.

  @author  Stephen Anthony
*/
class Bankswitch
{
  public:
    // Currently supported bankswitch schemes
    enum class Type {
      _AUTO,  _0840,    _2IN1,  _4IN1,   _8IN1,   _16IN1, _32IN1,
      _64IN1, _128IN1,  _2K,    _3E,     _3EP,    _3F,    _4A50,
      _4K,    _4KSC,    _AR,    _BF,     _BFSC,   _BUS,   _CDF,
      _CM,    _CTY,     _CV,    _CVP,    _DASH,   _DF,    _DFSC,
      _DPC,   _DPCP,    _E0,    _E7,     _E78K,   _EF,    _EFSC,
      _F0,    _F4,      _F4SC,  _F6,     _F6SC,   _F8,    _F8SC,
      _FA,    _FA2,     _FE,    _MDM,    _SB,     _UA,    _WD,
      _X07,
    #ifdef CUSTOM_ARM
      _CUSTOM,
    #endif
      NumSchemes
    };

    // Info about the various bankswitch schemes, useful for displaying
    // in GUI dropdown boxes, etc
    struct Description {
      const char* const name;
      const char* const desc;
    };
    static Description BSList[int(Type::NumSchemes)];

  public:
    // Convert BSType enum to string
    static string typeToName(Bankswitch::Type type);

    // Convert string to BSType enum
    static Bankswitch::Type nameToType(const string& name);

    // Determine bankswitch type by filename extension
    // Use '_AUTO' if unknown
    static Bankswitch::Type typeFromExtension(const FilesystemNode& file);

    /**
      Is this a valid ROM filename (does it have a valid extension?).

      @param name  Filename of potential ROM file
      @param ext   The extension extracted from the given file
     */
    static bool isValidRomName(const string& name, string& ext);

    /**
      Convenience functions for different parameter types.
     */
    static bool isValidRomName(const FilesystemNode& name, string& ext);
    static bool isValidRomName(const FilesystemNode& name);
    static bool isValidRomName(const string& name);

  private:
    struct TypeComparator {
      bool operator() (const string& a, const string& b) const {
        return BSPF::compareIgnoreCase(a, b) < 0;
      }
    };
    using ExtensionMap = std::map<string, Bankswitch::Type, TypeComparator>;
    static ExtensionMap ourExtensions;

    using NameToTypeMap = std::map<string, Bankswitch::Type, TypeComparator>;
    static NameToTypeMap ourNameToTypes;

  private:
    // Following constructors and assignment operators not supported
    Bankswitch() = delete;
    Bankswitch(const Bankswitch&) = delete;
    Bankswitch(Bankswitch&&) = delete;
    Bankswitch& operator=(const Bankswitch&) = delete;
    Bankswitch& operator=(Bankswitch&&) = delete;
};

#endif
