// $Id$

#include "MidiOutConnector.hh"
#include "MidiOutDevice.hh"
#include "DummyMidiOutDevice.hh"
#include "PluggingController.hh"


namespace openmsx {

MidiOutConnector::MidiOutConnector(const string &name)
	: Connector(name, new DummyMidiOutDevice())
{
	PluggingController::instance()->registerConnector(this);
}

MidiOutConnector::~MidiOutConnector()
{
	PluggingController::instance()->unregisterConnector(this);
}

const string& MidiOutConnector::getDescription() const
{
	static const string desc("Midi-OUT connector.");
	return desc;
}

const string& MidiOutConnector::getClass() const
{
	static const string className("midi out");
	return className;
}

void MidiOutConnector::setDataBits(DataBits bits)
{
	((MidiOutDevice*)pluggable)->setDataBits(bits);
}

void MidiOutConnector::setStopBits(StopBits bits)
{
	((MidiOutDevice*)pluggable)->setStopBits(bits);
}

void MidiOutConnector::setParityBit(bool enable, ParityBit parity)
{
	((MidiOutDevice*)pluggable)->setParityBit(enable, parity);
}

void MidiOutConnector::recvByte(byte value, const EmuTime& time)
{
	((MidiOutDevice*)pluggable)->recvByte(value, time);
}

} // namespace openmsx
