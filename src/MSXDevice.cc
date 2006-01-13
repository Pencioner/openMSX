// $Id$

#include "MSXDevice.hh"
#include "XMLElement.hh"
#include "MSXMotherBoard.hh"
#include "CartridgeSlotManager.hh"
#include "MSXCPUInterface.hh"
#include "CPU.hh"
#include "StringOp.hh"
#include "MSXException.hh"

using std::string;
using std::vector;

namespace openmsx {

byte MSXDevice::unmappedRead[0x10000];
byte MSXDevice::unmappedWrite[0x10000];


MSXDevice::MSXDevice(MSXMotherBoard& motherBoard_, const XMLElement& config,
                     const EmuTime& /*time*/)
	: deviceConfig(config), motherBoard(motherBoard_)
{
	initMem();
	registerSlots(config);
	registerPorts(config);
}

MSXDevice::~MSXDevice()
{
	unregisterPorts(deviceConfig);
	unregisterSlots(deviceConfig);
}

void MSXDevice::initMem()
{
	static bool alreadyInit = false;
	if (alreadyInit) {
		return;
	}
	alreadyInit = true;
	memset(unmappedRead, 0xFF, 0x10000);
}

void MSXDevice::getMemRegions(const XMLElement& config, MemRegions& result)
{
	XMLElement::Children memConfigs;
	config.getChildren("mem", memConfigs);
	for (XMLElement::Children::const_iterator it = memConfigs.begin();
	     it != memConfigs.end(); ++it) {
		unsigned base = (*it)->getAttributeAsInt("base");
		unsigned size = (*it)->getAttributeAsInt("size");
		if ((base >= 0x10000) || (size > 0x10000)) {
			throw FatalError(
				"Invalid memory specification for device " +
				getName() + " should be in range "
				"[0x0000,0x10000).");
		}
		if ((base & CPU::CACHE_LINE_LOW) || (size & CPU::CACHE_LINE_LOW)) {
			int tmp = CPU::CACHE_LINE_SIZE; // solves link error
			throw FatalError(
				"Invalid memory specification for device " +
				getName() + " should be aligned at " +
				StringOp::toHexString(tmp, 4) + ".");
		}
		result.push_back(std::make_pair(base, size));
	}
}

void MSXDevice::registerSlots(const XMLElement& config)
{
	MemRegions memRegions;
	getMemRegions(config, memRegions);
	if (memRegions.empty()) {
		return;
	}

	CartridgeSlotManager& slotManager = getMotherBoard().getSlotManager();
	ps = 0;
	ss = 0;
	const XMLElement* parent = config.getParent();
	while (true) {
		const string& name = parent->getName();
		if (name == "secondary") {
			const string& secondSlot = parent->getAttribute("slot");
			ss = slotManager.getSlotNum(secondSlot);
		} else if (name == "primary") {
			const string& primSlot = parent->getAttribute("slot");
			ps = slotManager.getSlotNum(primSlot);
			break;
		}
		parent = parent->getParent();
		if (!parent) {
			throw FatalError("Invalid memory specification");
		}
	}

	if (ps == -256) {
		// any slot
		slotManager.getSlot(ps, ss);
	} else if (ps < 0) {
		// specified slot by name (carta, cartb, ...)
		slotManager.getSlot(-ps - 1, ps, ss);
	} else {
		// numerical specified slot (0, 1, 2, 3)
	}

	for (MemRegions::const_iterator it = memRegions.begin();
	     it != memRegions.end(); ++it) {
		getMotherBoard().getCPUInterface().registerMemDevice(
			*this, ps, ss, it->first, it->second);
	}
}

void MSXDevice::unregisterSlots(const XMLElement& config)
{
	MemRegions memRegions;
	getMemRegions(config, memRegions);
	for (MemRegions::const_iterator it = memRegions.begin();
	     it != memRegions.end(); ++it) {
		getMotherBoard().getCPUInterface().unregisterMemDevice(
			*this, ps, ss, it->first, it->second);
	}
}

static void getPorts(const XMLElement& config,
                     vector<byte>& inPorts, vector<byte>& outPorts)
{
	XMLElement::Children ios;
	config.getChildren("io", ios);
	for (XMLElement::Children::const_iterator it = ios.begin();
	     it != ios.end(); ++it) {
		unsigned base = StringOp::stringToInt((*it)->getAttribute("base"));
		unsigned num  = StringOp::stringToInt((*it)->getAttribute("num"));
		string type = (*it)->getAttribute("type", "IO");
		if (((base + num) > 256) || (num == 0) ||
		    ((type != "I") && (type != "O") && (type != "IO"))) {
			throw FatalError("Invalid IO port specification");
		}
		for (unsigned i = 0; i < num; ++i) {
			if ((type == "I") || (type == "IO")) {
				inPorts.push_back(base + i);
			}
			if ((type == "O") || (type == "IO")) {
				outPorts.push_back(base + i);
			}
		}
	}
}

void MSXDevice::registerPorts(const XMLElement& config)
{
	vector<byte> inPorts;
	vector<byte> outPorts;
	getPorts(config, inPorts, outPorts);

	for (vector<byte>::iterator it = inPorts.begin();
	     it != inPorts.end(); ++it) {
		getMotherBoard().getCPUInterface().register_IO_In(*it, this);
	}
	for (vector<byte>::iterator it = outPorts.begin();
	     it != outPorts.end(); ++it) {
		getMotherBoard().getCPUInterface().register_IO_Out(*it, this);
	}
}

void MSXDevice::unregisterPorts(const XMLElement& config)
{
	vector<byte> inPorts;
	vector<byte> outPorts;
	getPorts(config, inPorts, outPorts);

	for (vector<byte>::iterator it = inPorts.begin();
	     it != inPorts.end(); ++it) {
		getMotherBoard().getCPUInterface().unregister_IO_In(*it, this);
	}
	for (vector<byte>::iterator it = outPorts.begin();
	     it != outPorts.end(); ++it) {
		getMotherBoard().getCPUInterface().unregister_IO_Out(*it, this);
	}
}


void MSXDevice::reset(const EmuTime& /*time*/)
{
	// nothing
}

void MSXDevice::powerDown(const EmuTime& /*time*/)
{
	// nothing
}

void MSXDevice::powerUp(const EmuTime& time)
{
	reset(time);
}

string MSXDevice::getName() const
{
	return deviceConfig.getId();
}


byte MSXDevice::readIO(word port, const EmuTime& /*time*/)
{
	if (port); // avoid warning
	PRT_DEBUG("MSXDevice::readIO (0x" << std::hex << (int)(port & 0xFF)
	          << std::dec << ") : No device implementation.");
	return 0xFF;
}

void MSXDevice::writeIO(word port, byte value, const EmuTime& /*time*/)
{
	if (port); // avoid warning
	if (value); // avoid warning
	PRT_DEBUG("MSXDevice::writeIO(port 0x" << std::hex << (int)(port & 0xFF)
	          << std::dec << ",value " << (int)value
	          << ") : No device implementation.");
	// do nothing
}

byte MSXDevice::peekIO(word /*port*/, const EmuTime& /*time*/) const
{
	return 0xFF;
}


byte MSXDevice::readMem(word address, const EmuTime& /*time*/)
{
	if (address); // avoid warning
	PRT_DEBUG("MSXDevice: read from unmapped memory " << std::hex <<
	          (int)address << std::dec);
	return 0xFF;
}

const byte* MSXDevice::getReadCacheLine(word /*start*/) const
{
	return NULL;	// uncacheable
}

void MSXDevice::writeMem(word address, byte /*value*/,
                            const EmuTime& /*time*/)
{
	if (address); // avoid warning
	PRT_DEBUG("MSXDevice: write to unmapped memory " << std::hex <<
	          (int)address << std::dec);
	// do nothing
}

byte MSXDevice::peekMem(word address, const EmuTime& /*time*/) const
{
	word base = address & CPU::CACHE_LINE_HIGH;
	const byte* cache = getReadCacheLine(base);
	if (cache) {
		word offset = address & CPU::CACHE_LINE_LOW;
		return cache[offset];
	} else {
		PRT_DEBUG("MSXDevice: peek not supported for this device");
		return 0xFF;
	}
}

byte* MSXDevice::getWriteCacheLine(word /*start*/) const
{
	return NULL;	// uncacheable
}

} // namespace openmsx
