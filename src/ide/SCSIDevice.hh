#ifndef SCSIDEVICE_HH
#define SCSIDEVICE_HH

#include "SCSI.hh"

namespace openmsx {

class SCSIDevice
{
public:
	static const unsigned BIT_SCSI2          = 0x0001;
	static const unsigned BIT_SCSI2_ONLY     = 0x0002;
	static const unsigned BIT_SCSI3          = 0x0004;

	static const unsigned MODE_SCSI1         = 0x0000;
	static const unsigned MODE_SCSI2         = 0x0003;
	static const unsigned MODE_SCSI3         = 0x0005;
	static const unsigned MODE_UNITATTENTION = 0x0008; // report unit attention
	static const unsigned MODE_MEGASCSI      = 0x0010; // report disk change when call of
	                                              // 'test unit ready'
	static const unsigned MODE_NOVAXIS       = 0x0100;

	static const unsigned BUFFER_SIZE   = 0x10000; // 64KB

	virtual ~SCSIDevice() = default;

	virtual void reset() = 0;
	virtual bool isSelected() = 0;
	virtual unsigned executeCmd(const byte* cdb, SCSI::Phase& phase,
	                            unsigned& blocks) = 0;
	virtual unsigned executingCmd(SCSI::Phase& phase, unsigned& blocks) = 0;
	virtual byte getStatusCode() = 0;
	virtual int msgOut(byte value) = 0;
	virtual byte msgIn() = 0;
	virtual void disconnect() = 0;
	virtual void busReset() = 0; // only used in MB89352 controller

	virtual unsigned dataIn(unsigned& blocks) = 0;
	virtual unsigned dataOut(unsigned& blocks) = 0;
};

} // namespace openmsx

#endif
