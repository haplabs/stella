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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "System.hxx"
#include "CartMNetwork.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMNetwork::CartridgeMNetwork(const BytePtr& image, uInt32 size,
                         const Settings& settings)
  : Cartridge(settings),
    myCurrentRAM(0)
{}

void CartridgeMNetwork::initialize(const BytePtr& image, uInt32 size)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image.get(), std::min(bankCount() * BANK_SIZE, size));
  createCodeAccessBase(bankCount() * BANK_SIZE + RAM_SIZE);

  // Remember startup bank
  myStartBank = 0;
  myFixedSlice = bankCount() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMNetwork::reset()
{
  initializeRAM(myRAM, RAM_SIZE);

  // Install some default banks for the RAM and first segment
  bankRAM(0);
  bank(myStartBank);

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMNetwork::install(System& system)
{
  mySystem = &system;

  System::PageAccess access(this, System::PA_READ);

  // Set the page accessing methods for the hot spots
  for(uInt16 addr = (0x1FE0 & ~System::PAGE_MASK); addr < 0x2000;
      addr += System::PAGE_SIZE)
  {
    access.codeAccessBase = &myCodeAccessBase[0x1fc0];
    mySystem->setPageAccess(addr, access);
  }

  // Setup the second segment to always point to the last ROM slice
  for(uInt16 addr = 0x1A00; addr < (0x1FE0U & ~System::PAGE_MASK);
      addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myImage[myFixedSlice * BANK_SIZE + (addr & (BANK_SIZE-1))];
    access.codeAccessBase = &myCodeAccessBase[myFixedSlice * BANK_SIZE + (addr & (BANK_SIZE-1))];
    mySystem->setPageAccess(addr, access);
  }
  myCurrentSlice[1] = myFixedSlice;

  // Install some default banks for the RAM and first segment
  bankRAM(0);
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeMNetwork::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  // Switch banks if necessary
  checkSwitchBank(address);

  if((myCurrentSlice[0] == myFixedSlice) && (address < 0x0400))
  {
    // Reading from the 1K write port @ $1000 triggers an unwanted write
    uInt8 value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(peekAddress);
      return myRAM[address & 0x03FF] = value;
    }
  }
  else if((address >= 0x0800) && (address <= 0x08FF))
  {
    // Reading from the 256B write port @ $1800 triggers an unwanted write
    uInt8 value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(peekAddress);
      return myRAM[1024 + (myCurrentRAM << 8) + (address & 0x00FF)] = value;
    }
  }
  else
    return myImage[(myCurrentSlice[address >> 11] << 11) + (address & (BANK_SIZE-1))];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMNetwork::poke(uInt16 address, uInt8)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  checkSwitchBank(address);

  // NOTE: This does not handle writing to RAM, however, this
  // method should never be called for RAM because of the
  // way page accessing has been setup
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMNetwork::bankRAM(uInt16 bank)
{
  if(bankLocked()) return;

  // Remember what bank we're in
  myCurrentRAM = bank;
  uInt16 offset = bank << 8;

  // Setup the page access methods for the current bank
  System::PageAccess access(this, System::PA_WRITE);

  // Set the page accessing method for the 256 bytes of RAM writing pages
  for(uInt16 addr = 0x1800; addr < 0x1900; addr += System::PAGE_SIZE)
  {
    access.directPokeBase = &myRAM[1024 + offset + (addr & 0x00FF)];
    access.codeAccessBase = &myCodeAccessBase[0x2000 + 1024 + offset + (addr & 0x00FF)];
    mySystem->setPageAccess(addr, access);
  }

  // Set the page accessing method for the 256 bytes of RAM reading pages
  access.directPokeBase = nullptr;
  access.type = System::PA_READ;
  for(uInt16 addr = 0x1900; addr < 0x1A00; addr += System::PAGE_SIZE)
  {
    access.directPeekBase = &myRAM[1024 + offset + (addr & 0x00FF)];
    access.codeAccessBase = &myCodeAccessBase[0x2000 + 1024 + offset + (addr & 0x00FF)];
    mySystem->setPageAccess(addr, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMNetwork::bank(uInt16 slice)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myCurrentSlice[0] = slice;
  uInt16 offset = slice << 11;

  // Setup the page access methods for the current bank
  if(slice != myFixedSlice)
  {
    System::PageAccess access(this, System::PA_READ);

    // Map ROM image into first segment
    for(uInt16 addr = 0x1000; addr < 0x1800; addr += System::PAGE_SIZE)
    {
      access.directPeekBase = &myImage[offset + (addr & (BANK_SIZE-1))];
      access.codeAccessBase = &myCodeAccessBase[offset + (addr & (BANK_SIZE-1))];
      mySystem->setPageAccess(addr, access);
    }
  }
  else
  {
    System::PageAccess access(this, System::PA_WRITE);

    // Set the page accessing method for the 1K slice of RAM writing pages
    for(uInt16 addr = 0x1000; addr < 0x1400; addr += System::PAGE_SIZE)
    {
      access.directPokeBase = &myRAM[addr & 0x03FF];
      access.codeAccessBase = &myCodeAccessBase[0x2000 + (addr & 0x03FF)];
      mySystem->setPageAccess(addr, access);
    }

    // Set the page accessing method for the 1K slice of RAM reading pages
    access.directPokeBase = nullptr;
    access.type = System::PA_READ;
    for(uInt16 addr = 0x1400; addr < 0x1800; addr += System::PAGE_SIZE)
    {
      access.directPeekBase = &myRAM[addr & 0x03FF];
      access.codeAccessBase = &myCodeAccessBase[0x2000 + (addr & 0x03FF)];
      mySystem->setPageAccess(addr, access);
    }
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeMNetwork::getBank() const
{
  return myCurrentSlice[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMNetwork::patch(uInt16 address, uInt8 value)
{
  address = address & 0x0FFF;

  if(address < 0x0800)
  {
    if(myCurrentSlice[0] == myFixedSlice)
    {
      // Normally, a write to the read port won't do anything
      // However, the patch command is special in that ignores such
      // cart restrictions
      myRAM[address & 0x03FF] = value;
    }
    else
      myImage[(myCurrentSlice[0] << 11) + (address & (BANK_SIZE-1))] = value;
  }
  else if(address < 0x0900)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[1024 + (myCurrentRAM << 8) + (address & 0x00FF)] = value;
  }
  else
    myImage[(myCurrentSlice[address >> 11] << 11) + (address & (BANK_SIZE-1))] = value;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeMNetwork::getImage(uInt32& size) const
{
  size = bankCount() * BANK_SIZE;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMNetwork::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShortArray(myCurrentSlice, 2);
    out.putShort(myCurrentRAM);
    out.putByteArray(myRAM, RAM_SIZE);
  } catch(...)
  {
    cerr << "ERROR: " << name() << "::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMNetwork::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    in.getShortArray(myCurrentSlice, 2);
    myCurrentRAM = in.getShort();
    in.getByteArray(myRAM, RAM_SIZE);
  } catch(...)
  {
    cerr << "ERROR: " << name() << "::load" << endl;
    return false;
  }

  // Set up the previously used banks for the RAM and segment
  bankRAM(myCurrentRAM);
  bank(myCurrentSlice[0]);

  return true;
}