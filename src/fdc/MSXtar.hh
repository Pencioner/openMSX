// This code implements the functionality of my older msxtar program
// that could manipulate files and directories on dsk and ide-hd images
// Integrating it is seen as temporary bypassing of the need for a
// DirAsDisk2 that supports subdirs, partitions etc. since this class will
// of those functionalities although not on a dynamic base

#ifndef MSXTAR_HH
#define MSXTAR_HH

#include "MemBuffer.hh"
#include "DiskImageUtils.hh"
#include "string_view.hh"

namespace openmsx {

class SectorAccessibleDisk;

class MSXtar
{
public:
	explicit MSXtar(SectorAccessibleDisk& disk);
	~MSXtar();

	void chdir(string_view newRootDir);
	void mkdir(string_view newRootDir);
	std::string dir();
	std::string addFile(const std::string& filename);
	std::string addDir(string_view rootDirName);
	std::string getItemFromDir(string_view rootDirName, string_view itemName);
	void getDir(string_view rootDirName);

private:
	struct DirEntry {
		unsigned sector;
		unsigned index;
	};

	void writeLogicalSector(unsigned sector, const SectorBuffer& buf);
	void readLogicalSector (unsigned sector,       SectorBuffer& buf);

	unsigned clusterToSector(unsigned cluster);
	unsigned sectorToCluster(unsigned sector);
	void parseBootSector(const MSXBootSector& boot);
	unsigned readFAT(unsigned clnr) const;
	void writeFAT(unsigned clnr, unsigned val);
	unsigned findFirstFreeCluster();
	unsigned findUsableIndexInSector(unsigned sector);
	unsigned getNextSector(unsigned sector);
	unsigned getStartCluster(const MSXDirEntry& entry);
	unsigned appendClusterToSubdir(unsigned sector);
	DirEntry addEntryToDir(unsigned sector);
	unsigned addSubdir(const std::string& msxName,
	                   unsigned t, unsigned d, unsigned sector);
	void alterFileInDSK(MSXDirEntry& msxDirEntry, const std::string& hostName);
	unsigned addSubdirToDSK(const std::string& hostName,
	                   const std::string& msxName, unsigned sector);
	DirEntry findEntryInDir(const std::string& name, unsigned sector,
	                        SectorBuffer& sectorBuf);
	std::string addFileToDSK(const std::string& fullHostName, unsigned sector);
	std::string recurseDirFill(string_view dirName, unsigned sector);
	std::string condensName(const MSXDirEntry& dirEntry);
	void changeTime (const std::string& resultFile, const MSXDirEntry& dirEntry);
	void fileExtract(const std::string& resultFile, const MSXDirEntry& dirEntry);
	void recurseDirExtract(string_view dirName, unsigned sector);
	std::string singleItemExtract(string_view dirName, string_view itemName,
	                              unsigned sector);
	void chroot(string_view newRootDir, bool createDir);


	SectorAccessibleDisk& disk;
	MemBuffer<SectorBuffer> fatBuffer;

	unsigned maxCluster;
	unsigned sectorsPerCluster;
	unsigned sectorsPerFat;
	unsigned rootDirStart; // first sector from the root directory
	unsigned rootDirLast;  // last  sector from the root directory
	unsigned chrootSector;

	bool fatCacheDirty;
};

} // namespace openmsx

#endif
