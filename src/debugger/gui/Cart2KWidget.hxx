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

#ifndef CARTRIDGE2K_WIDGET_HXX
#define CARTRIDGE2K_WIDGET_HXX

class Cartridge2K;

#include "CartDebugWidget.hxx"

class Cartridge2KWidget : public CartDebugWidget
{
  public:
    Cartridge2KWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      Cartridge2K& cart);
    virtual ~Cartridge2KWidget() = default;

  private:
    // No implementation for non-bankswitched ROMs
    void loadConfig() override { }
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override { }

    // Following constructors and assignment operators not supported
    Cartridge2KWidget() = delete;
    Cartridge2KWidget(const Cartridge2KWidget&) = delete;
    Cartridge2KWidget(Cartridge2KWidget&&) = delete;
    Cartridge2KWidget& operator=(const Cartridge2KWidget&) = delete;
    Cartridge2KWidget& operator=(Cartridge2KWidget&&) = delete;
};

#endif
