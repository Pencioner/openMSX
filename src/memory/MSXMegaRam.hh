// $Id$

#ifndef __MEGARAM_HH__
#define __MEGARAM_HH__

#include "MSXMemDevice.hh"
#include "MSXIODevice.hh"


class MSXMegaRam : public MSXMemDevice, public MSXIODevice
{
	public:
		MSXMegaRam(Device* config, const EmuTime &time);
		virtual ~MSXMegaRam();
		
		virtual void reset(const EmuTime &time);
		virtual byte readMem(word address, const EmuTime &time);
		virtual const byte* getReadCacheLine(word address) const;
		virtual void writeMem(word address, byte value,
		                      const EmuTime &time);
		virtual byte* getWriteCacheLine(word address) const;

		virtual byte readIO(byte port, const EmuTime &time);
		virtual void writeIO(byte port, byte value, const EmuTime &time);

	private:
		void setBank(byte page, byte block);

		byte *ram;
		byte maxBlock;
		byte bank[4];
		bool writeMode;
};

#endif
