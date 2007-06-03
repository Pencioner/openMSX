// $Id$
/* Ported from:
** Source: /cvsroot/bluemsx/blueMSX/Src/IoDevice/MB89352.h,v
** Revision: 1.4
** Date: 2007/03/28 17:35:35
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, white cat
*/

#ifndef MB89352_HH
#define MB89352_HH

#include "SCSI.hh"
#include <memory>
#include <vector>

namespace openmsx {

class SCSIDevice;
class XMLElement;
class MSXMotherBoard;

class MB89352
{
public:
	MB89352(MSXMotherBoard& motherBoard, const XMLElement& config);
	~MB89352();

	void reset(bool scsireset);
	byte readRegister(byte reg);
	byte peekRegister(byte reg) const;
	byte readDREG();
	byte peekDREG() const;
	void writeRegister(byte reg, byte value);
	void writeDREG(byte value);

private:
	void disconnect();
	void softReset();
	void setACKREQ(byte& value);
	void resetACKREQ();
	byte getSSTS() const;

	byte myId;                      // SPC SCSI ID 0..7
	byte targetId;                  // SCSI Device target ID 0..7
	byte regs[16];                  // SPC register
	bool rst;                       // SCSI bus reset signal
	byte atn;                       // SCSI bus attention signal
	SCSI::Phase phase;              //
	SCSI::Phase nextPhase;          // for message system
	bool isEnabled;                 // spc enable flag
	bool isBusy;                    // spc now working
	bool isTransfer;                // hardware transfer mode
	int msgin;                      // Message In flag
	int counter;                    // read and written number of bytes
	                                // within the range in the buffer
	unsigned blockCounter;          // Number of blocks outside buffer
	                                // (512bytes / block)
	int tc;                         // counter for hardware transfer
	//TODO: bool devBusy;           // CDROM busy (buffer conflict prevention)
	std::auto_ptr<SCSIDevice> dev[8];
	byte* pCdb;                     // cdb pointer
	unsigned bufIdx;                // buffer index
	byte cdb[12];                   // Command Descripter Block
	std::vector<byte> buffer;       // buffer for transfer
};

} // namespace openmsx

#endif
