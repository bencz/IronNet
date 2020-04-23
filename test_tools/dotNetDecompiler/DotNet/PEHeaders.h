#ifndef __DOTNET_PEHEADERS_H__
#define __DOTNET_PEHEADERS_H__

#include "IncFileNm.h"
#include HEADERBASE_H
#include STRINGBUILDER_H

namespace PEAnalyzer
{
	/// <summary>
	/// Immediately after the PE signature is the PE File header
	/// consisting of the following:
	/// </summary>
	class PEFileHeader : public HeaderBase
	{
	public:
		/// <summary>
		/// Always 0x14c (see Section 23.1).
		/// </summary>
		short Machine;

		/// <summary>
		/// Number of sections; indicates size of the Section Table,
		/// which immediately follows the headers.
		/// </summary>
		short NumberOfSections;

		/// <summary>
		/// Time and date the file was created in seconds since January
		/// 1st 1970 00:00:00 or 0.
		/// </summary>
		int TimeDateStamp;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		int PointerToSymbolTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		int NumberOfSymbols;

		/// <summary>
		/// Size of the optional header, the format is described below.
		/// </summary>
		short OptionalHeaderSize;

		/// <summary>
		/// Flags indicating attributes of the file, see
		/// Characteristics.
		/// </summary>
		short Characteristics;

		PEFileHeader()
		{
			InitializeInstanceFields();
			_title = "PE File Header";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			Machine = readInt16(0);
			NumberOfSections = readInt16(2);
			TimeDateStamp = readInt32(4);
			PointerToSymbolTable = readInt32(8);
			NumberOfSymbols = readInt32(12);
			OptionalHeaderSize = readInt16(16);
			Characteristics = readInt16(18);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			Machine = 0;
			NumberOfSections = 0;
			TimeDateStamp = 0;
			PointerToSymbolTable = 0;
			NumberOfSymbols = 0;
			OptionalHeaderSize = 0;
			Characteristics = 0;
		}
	};

	/// <summary>
	/// Immediately after the PE Header is the PE Optional Header.
	/// This header contains the following information:
	/// </summary>
	class PEOptionalHeader : public HeaderBase
	{
	public:
		PEOptionalHeader()
		{
			_title = "PE Optional Header";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

		}

		virtual void getInfos(StringBuilder* sb)
		{
			//HeaderBase::getInfos(sb);
		}
	};

	/// <summary>
	/// These fields are required for all PE files and contain the
	/// following information:
	/// </summary>
	class PEHeaderStandardFields : public HeaderBase
	{
	public:
		/// <summary>
		/// Always 0x10B (see Section 23.1).
		/// </summary>
		short Magic;

		/// <summary>
		/// Always 6 (see Section 23.1).
		/// </summary>
		unsigned char LMajor;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		unsigned char LMinor;

		/// <summary>
		/// Size of the code (text) section, or the sum of all code
		/// sections if there are multiple sections.
		/// </summary>
		int CodeSize;

		/// <summary>
		/// Size of the initialized data section, or the sum of all such
		/// sections if there are multiple data sections.
		/// </summary>
		int InitializedDataSize;

		/// <summary>
		/// Size of the uninitialized data section, or the sum of all
		/// such sections if there are multiple unitinitalized data
		/// sections.
		/// </summary>
		int UninitializedDataSize;

		/// <summary>
		/// RVA of entry point , needs to point to bytes 0xFF 0x25
		/// followed by the RVA in a section marked execute/read for
		/// EXEs or 0 for DLLs
		/// </summary>
		int EntryPointRVA;

		/// <summary>
		/// RVA of the code section, always 0x00400000 for exes and
		/// 0x10000000 for DLL.
		/// </summary>
		int BaseOfCode;

		/// <summary>
		/// RVA of the data section.
		/// </summary>
		int BaseOfData;

		PEHeaderStandardFields()
		{
			InitializeInstanceFields();
			_title = "PE Header Standard Fields";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			Magic = readInt16(0);
			LMajor = readByte(2);
			LMinor = readByte(3);
			CodeSize = readInt32(4);
			InitializedDataSize = readInt32(8);
			UninitializedDataSize = readInt32(12);
			EntryPointRVA = readInt32(16);
			BaseOfCode = readInt32(20);
			BaseOfData = readInt32(24);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			Magic = 0;
			LMajor = 0;
			LMinor = 0;
			CodeSize = 0;
			InitializedDataSize = 0;
			UninitializedDataSize = 0;
			EntryPointRVA = 0;
			BaseOfCode = 0;
			BaseOfData = 0;
		}
	};

	/// <summary>
	/// These fields are Windows NT specific:
	/// </summary>
	class PEHeaderWindowsNTSpecificFields : public HeaderBase
	{
	public:
		/// <summary>
		/// Always 0x400000 (see Section 23.1).
		/// </summary>
		int ImageBase;

		/// <summary>
		/// Always 0x2000 (see Section 23.1).
		/// </summary>
		int SectionAlignment;

		/// <summary>
		/// Either 0x200 or 0x1000.
		/// </summary>
		int FileAlignment;

		/// <summary>
		/// Always 4 (see Section 23.1).
		/// </summary>
		short OSMajor;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		short OSMinor;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		short UserMajor;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		short UserMinor;

		/// <summary>
		/// Always 4 (see Section 23.1).
		/// </summary>
		short SubSysMajor;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		short SubSysMinor;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		int Reserved;

		/// <summary>
		/// Size, in bytes, of image, including all headers and padding;
		/// shall be a multiple of Section Alignment.
		/// </summary>
		int ImageSize;

		/// <summary>
		/// Combined size of MS-DOS Header, PE Header, PE Optional
		/// Header and padding; shall be a multiple of the file
		/// alignment.
		/// </summary>
		int HeaderSize;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		int FileChecksum;

		/// <summary>
		/// Subsystem required to run this image. Shall be either
		/// IMAGE_SUBSYSTEM_WINDOWS_CE_GUI (0x3) or
		/// IMAGE_SUBSYSTEM_WINDOWS_GUI (0x2).
		/// </summary>
		short SubSystem;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		short DLLFlags;

		/// <summary>
		/// Always 0x100000 (1Mb) (see Section 23.1).
		/// </summary>
		int StackReserveSize;

		/// <summary>
		/// Always 0x1000 (4Kb) (see Section 23.1).
		/// </summary>
		int StackCommitSize;

		/// <summary>
		/// Always 0x100000 (1Mb) (see Section 23.1).
		/// </summary>
		int HeapReserveSize;

		/// <summary>
		/// Always 0x1000 (4Kb) (see Section 23.1).
		/// </summary>
		int HeapCommitSize;

		/// <summary>
		/// Always 0 (see Section 23.1)
		/// </summary>
		int LoaderFlags;

		/// <summary>
		/// Always 0x10 (see Section 23.1).
		/// </summary>
		int NumberOfDataDirectories;

		PEHeaderWindowsNTSpecificFields()
		{
			InitializeInstanceFields();
			_title = "PE Header Windows NT-Specific Fields";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			ImageBase = readInt32(28);
			SectionAlignment = readInt32(32);
			FileAlignment = readInt32(36);
			OSMajor = readInt16(40);
			OSMinor = readInt16(42);
			UserMajor = readInt16(44);
			UserMinor = readInt16(46);
			SubSysMajor = readInt16(48);
			SubSysMinor = readInt16(50);
			Reserved = readInt32(52);
			ImageSize = readInt32(56);
			HeaderSize = readInt32(60);
			FileChecksum = readInt32(64);
			SubSystem = readInt16(68);
			DLLFlags = readInt16(70);
			StackReserveSize = readInt32(72);
			StackCommitSize = readInt32(76);
			HeapReserveSize = readInt32(80);
			HeapCommitSize = readInt32(84);
			LoaderFlags = readInt32(88);
			NumberOfDataDirectories = readInt32(92);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			ImageBase = 0;
			SectionAlignment = 0;
			FileAlignment = 0;
			OSMajor = 0;
			OSMinor = 0;
			UserMajor = 0;
			UserMinor = 0;
			SubSysMajor = 0;
			SubSysMinor = 0;
			Reserved = 0;
			ImageSize = 0;
			HeaderSize = 0;
			FileChecksum = 0;
			SubSystem = 0;
			DLLFlags = 0;
			StackReserveSize = 0;
			StackCommitSize = 0;
			HeapReserveSize = 0;
			HeapCommitSize = 0;
			LoaderFlags = 0;
			NumberOfDataDirectories = 0;
		}
	};

	/// <summary>
	/// The optional header data directories give the address and
	/// size of several tables that appear in the sections of the PE
	/// file. Each data directory entry contains the RVA and Size of
	/// the structure it describes.
	/// The tables pointed to by the directory entries are stored in
	/// on of the PE file's sections; these sections themselves are
	/// described by section headers.
	/// </summary>
	class PEHeaderDataDirectories : public HeaderBase
	{
	public:
		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long ExportTable;

		/// <summary>
		/// RVA of Import Table, (see clause 24.3.1).
		/// </summary>
		long long ImportTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long ResourceTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long ExceptionTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long CertificateTable;

		/// <summary>
		/// Relocation Table, set to 0 if unused (see clause 24.3.1).
		/// </summary>
		long long BaseRelocationTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long Debug;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long Copyright;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long GlobalPtr;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long TLSTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long LoadConfigTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long BoundImport;

		/// <summary>
		/// RVA of Import Address Table, (see clause 24.3.1).
		/// </summary>
		long long IAT;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long DelayImportDescriptor;

		/// <summary>
		/// CLI Header with directories for runtime data, (see clause
		/// 24.3.1).
		/// </summary>
		long long CLIHeader;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long Reserved;

		PEHeaderDataDirectories()
		{
			InitializeInstanceFields();
			_title = "PE Header Data Directories";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			ExportTable = readInt64(96);
			ImportTable = readInt64(104);
			ResourceTable = readInt64(112);
			ExceptionTable = readInt64(120);
			CertificateTable = readInt64(128);
			BaseRelocationTable = readInt64(136);
			Debug = readInt64(144);
			Copyright = readInt64(152);
			GlobalPtr = readInt64(160);
			TLSTable = readInt64(168);
			LoadConfigTable = readInt64(176);
			BoundImport = readInt64(184);
			IAT = readInt64(192);
			DelayImportDescriptor = readInt64(200);
			CLIHeader = readInt64(208);
			Reserved = readInt64(216);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			ExportTable = 0;
			ImportTable = 0;
			ResourceTable = 0;
			ExceptionTable = 0;
			CertificateTable = 0;
			BaseRelocationTable = 0;
			Debug = 0;
			Copyright = 0;
			GlobalPtr = 0;
			TLSTable = 0;
			LoadConfigTable = 0;
			BoundImport = 0;
			IAT = 0;
			DelayImportDescriptor = 0;
			CLIHeader = 0;
			Reserved = 0;
		}
	};

	/// <summary>
	/// Immediately following the optional header is the Section
	/// Table, which contains a number of section headers. This
	/// positioning is required because the file header does not
	/// contain a direct pointer to the section table; the location
	/// of the section table is determined by calculating the
	/// location of the first byte after the headers.
	/// Each section header has the following format, for a total of
	/// 40 bytes per entry:
	/// </summary>
	class SectionHeaders : public HeaderBase
	{
	public:
		/// <summary>
		/// An 8-byte, null-padded ASCII string. There is no terminating
		/// null if the string is exactly eight characters long.
		/// </summary>
		long long Name;

		/// <summary>
		/// Total size of the section when loaded into memory in bytes
		/// rounded to Section Alignment. If this value is greater than
		/// Size of Raw Data, the section is zero-padded.
		/// </summary>
		int VirtualSize;

		/// <summary>
		/// For executable images this is the address of the first byte
		/// of the section, when loaded into memory, relative to the
		/// image base.
		/// </summary>
		int VirtualAddress;

		/// <summary>
		/// Size of the initialized data on disk in bytes, shall be a
		/// multiple of FileAlignment from the PE header. If this is
		/// less than VirtualSize the remainder of the section is zero
		/// filled. Because this field is rounded while the VirtualSize
		/// field is not it is possible for this to be greater than
		/// VirtualSize as well. When a section contains only
		/// uninitialized data, this field should be 0.
		/// </summary>
		int SizeOfRawData;

		/// <summary>
		/// Offset of section's first page within the PE file. This
		/// shall be a multiple of FileAlignment from the optional
		/// header. When a section contains only uninitialized data,
		/// this field should be 0.
		/// </summary>
		int PointerToRawData;

		/// <summary>
		/// RVA of Relocation section.
		/// </summary>
		int PointerToRelocations;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		int PointerToLinenumbers;

		/// <summary>
		/// Number of relocations, set to 0 if unused.
		/// </summary>
		short NumberOfRelocations;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		short NumberOfLinenumbers;

		/// <summary>
		/// Flags describing section's characteristics, see below.
		/// </summary>
		int Characteristics;

		SectionHeaders()
		{
			InitializeInstanceFields();
			_title = "Section Headers";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			Name = readInt64(0);
			VirtualSize = readInt32(8);
			VirtualAddress = readInt32(12);
			SizeOfRawData = readInt32(16);
			PointerToRawData = readInt32(20);
			PointerToRelocations = readInt32(24);
			PointerToLinenumbers = readInt32(28);
			NumberOfRelocations = readInt16(32);
			NumberOfLinenumbers = readInt16(34);
			Characteristics = readInt32(36);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			Name = 0;
			VirtualSize = 0;
			VirtualAddress = 0;
			SizeOfRawData = 0;
			PointerToRawData = 0;
			PointerToRelocations = 0;
			PointerToLinenumbers = 0;
			NumberOfRelocations = 0;
			NumberOfLinenumbers = 0;
			Characteristics = 0;
		}
	};

	/// <summary>
	/// The Import Table and the Import Address Table (IAT) are used
	/// to import the _CorExeMain (for a .exe) or _CorDllMain (for a
	/// .dll) entries of the runtime engine (mscoree.dll). The
	/// Import Table directory entry points to a one element zero
	/// terminated array of Import Directory entries (in a general
	/// PE file there is one entry for each imported DLL):
	/// </summary>
	class ImportTable : public HeaderBase
	{
	public:
		/// <summary>
		/// RVA of the Import Lookup Table
		/// </summary>
		int ImportLookupTable;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		int DateTimeStamp;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		int ForwarderChain;

		/// <summary>
		/// RVA of null terminated ASCII string "mscoree.dll".
		/// </summary>
		int Name;

		/// <summary>
		/// RVA of Import Address Table (this is the same as the RVA of
		/// the IAT descriptor in the optional header).
		/// </summary>
		int ImportAddressTable;

		ImportTable()
		{
			InitializeInstanceFields();
			_title = "Import Table";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			ImportLookupTable = readInt32(0);
			DateTimeStamp = readInt32(4);
			ForwarderChain = readInt32(8);
			Name = readInt32(12);
			ImportAddressTable = readInt32(16);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			ImportLookupTable = 0;
			DateTimeStamp = 0;
			ForwarderChain = 0;
			Name = 0;
			ImportAddressTable = 0;
		}
	};

	/// <summary>
	/// In a pure CIL image, a single fixup of type
	/// IMAGE_REL_BASED_HIGHLOW (0x3) is required for the x86
	/// startup stub which access the IAT to load the runtime engine
	/// on down level loaders. When building a mixed CIL/native
	/// image or when the image contains embedded RVAs in user data,
	/// the relocation section contains relocations for these as
	/// well.
	/// The relocations shall be in their own section, named
	/// ".reloc", which shall be the final section in the PE file.
	/// The relocation section contains a Fix-Up Table. The fixup
	/// table is broken into blocks of fixups. Each block represents
	/// the fixups for a 4K page, and each block shall start on a
	/// 32-bit boundary.
	/// Each fixup block starts with the following structure:
	/// </summary>
	class Relocations : public HeaderBase
	{
	public:
		/// <summary>
		/// The RVA of the block in which the fixup needs to be applied.
		/// The low 12 bits shall be zero.
		/// </summary>
		int PageRVA;

		/// <summary>
		/// Total number of bytes in the fixup block, including the Page
		/// RVA and Block Size fields, as well as the Type/Offset fields
		/// that follow, rounded up to the next multiple of 4.
		/// </summary>
		int BlockSize;

		Relocations()
		{
			InitializeInstanceFields();
			_title = "Relocations";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			PageRVA = readInt32(0);
			BlockSize = readInt32(4);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			PageRVA = 0;
			BlockSize = 0;
		}
	};

	/// <summary>
	/// The CLI header contains all of the runtime-specific data
	/// entries and other information. The header should be placed
	/// in a read only, sharable section of the image. This header
	/// is defined as follows:
	/// </summary>
	class CLIHeader : public HeaderBase
	{
	public:
		/// <summary>
		/// Size of the header in bytes
		/// </summary>
		int Cb;

		/// <summary>
		/// The minimum version of the runtime required to run this
		/// program, currently 2.
		/// </summary>
		short MajorRuntimeVersion;

		/// <summary>
		/// The minor portion of the version, currently 0.
		/// </summary>
		short MinorRuntimeVersion;

		/// <summary>
		/// RVA and size of the physical meta data (see Chapter 23).
		/// </summary>
		long long MetaData;

		/// <summary>
		/// Flags describing this runtime image. (see clause 24.3.3.1).
		/// </summary>
		int Flags;

		/// <summary>
		/// Token for the MethodDef or File of the entry point for the
		/// image
		/// </summary>
		int EntryPointToken;

		/// <summary>
		/// Location of CLI resources. (See Partition V ).
		/// </summary>
		long long Resources;

		/// <summary>
		/// RVA of the hash data for this PE file used by the CLI loader
		/// for binding and versioning
		/// </summary>
		long long StrongNameSignature;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long CodeManagerTable;

		/// <summary>
		/// RVA of an array of locations in the file that contain an
		/// array of function pointers (e.g., vtable slots), see below.
		/// </summary>
		long long VTableFixups;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long ExportAddressTableJumps;

		/// <summary>
		/// Always 0 (see Section 23.1).
		/// </summary>
		long long ManagedNativeHeader;

		CLIHeader()
		{
			InitializeInstanceFields();
			_title = "CLI Header";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			Cb = readInt32(0);
			MajorRuntimeVersion = readInt16(4);
			MinorRuntimeVersion = readInt16(6);
			MetaData = readInt64(8);
			Flags = readInt32(16);
			EntryPointToken = readInt32(20);
			Resources = readInt64(24);
			StrongNameSignature = readInt64(32);
			CodeManagerTable = readInt64(40);
			VTableFixups = readInt64(48);
			ExportAddressTableJumps = readInt64(56);
			ManagedNativeHeader = readInt64(64);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			Cb = 0;
			MajorRuntimeVersion = 0;
			MinorRuntimeVersion = 0;
			MetaData = 0;
			Flags = 0;
			EntryPointToken = 0;
			Resources = 0;
			StrongNameSignature = 0;
			CodeManagerTable = 0;
			VTableFixups = 0;
			ExportAddressTableJumps = 0;
			ManagedNativeHeader = 0;
		}
	};

	/// <summary>
	/// Certain languages, which choose not to follow the common
	/// type system runtime model, may have virtual functions which
	/// need to be represented in a v-table. These v-tables are laid
	/// out by the compiler, not by the runtime. Finding the correct
	/// v-table slot and calling indirectly through the value held
	/// in that slot is also done by the compiler. The VtableFixups
	/// field in the runtime header contains the location and size
	/// of an array of Vtable Fixups (see clause 14.5.1). V-tables
	/// shall be emitted into a read-write section of the PE file.
	/// Each entry in this array describes a contiguous array of
	/// v-table slots of the specified size. Each slot starts out
	/// initialized to the metadata token value for the method they
	/// need to call. At image load time, the runtime Loader will
	/// turn each entry into a pointer to machine code for the CPU
	/// and can be called directly.
	/// </summary>
	class VtableFixup : public HeaderBase
	{
	public:
		/// <summary>
		/// RVA of Vtable
		/// </summary>
		int VirtualAddress;

		/// <summary>
		/// Number of entries in Vtable
		/// </summary>
		short Size;

		/// <summary>
		/// Type of the entries, as defined in table below
		/// </summary>
		short Type;

		VtableFixup()
		{
			InitializeInstanceFields();
			_title = "Vtable Fixup";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			VirtualAddress = readInt32(0);
			Size = readInt16(4);
			Type = readInt16(6);
		}

		virtual void getInfos(StringBuilder* sb)
		{
			//HeaderBase::getInfos(sb);
			//
			//sb->append(getInfo(0, VirtualAddress, "VirtualAddress: RVA of Vtable"));
			//sb->append(getInfo(4, Size, "Size: Number of entries in Vtable"));
			//sb->append(getInfo(6, Type, "Type: Type of the entries, as defined in table below"));
		}

	private:
		void InitializeInstanceFields()
		{
			VirtualAddress = 0;
			Size = 0;
			Type = 0;
		}
	};

	/// <summary>
	/// Exception handling clauses also come in small and fat
	/// versions.
	/// The layout of fat form of exception handling clauses is as
	/// follows:
	/// </summary>
	class ExceptionHandlingClauses : public HeaderBase
	{
	public:
		/// <summary>
		/// Flags, see below.
		/// </summary>
		int Flags;

		/// <summary>
		/// Offset in bytes of try block from start of the header.
		/// </summary>
		int TryOffset;

		/// <summary>
		/// Length in bytes of the try block
		/// </summary>
		int TryLength;

		/// <summary>
		/// Location of the handler for this try block
		/// </summary>
		int HandlerOffset;

		/// <summary>
		/// Size of the handler code in bytes
		/// </summary>
		int HandlerLength;

		/// <summary>
		/// Meta data token for a type-based exception handler
		/// </summary>
		int ClassToken;

		/// <summary>
		/// Offset in method body for filter-based exception handler
		/// </summary>
		int FilterOffset;

		ExceptionHandlingClauses()
		{
			InitializeInstanceFields();
			_title = "Exception Handling Clauses";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			Flags = readInt32(0);
			TryOffset = readInt32(4);
			TryLength = readInt32(8);
			HandlerOffset = readInt32(12);
			HandlerLength = readInt32(16);
			ClassToken = readInt32(20);
			FilterOffset = readInt32(20);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			Flags = 0;
			TryOffset = 0;
			TryLength = 0;
			HandlerOffset = 0;
			HandlerLength = 0;
			ClassToken = 0;
			FilterOffset = 0;
		}
	};

	/// <summary>
	/// The root of the physical metadata starts with a magic
	/// signature, several bytes of version and other miscellaneous
	/// information, followed by a count and an array of stream
	/// headers, one for each stream that is present. The actual
	/// encoded tables and heaps are stored in the streams, which
	/// immediately follow this array of headers.
	/// </summary>
	class MetadataRoot : public HeaderBase
	{
	public:
		/// <summary>
		/// Magic signature for physical metadata : 0x424A5342.
		/// </summary>
		int Signature;

		/// <summary>
		/// Major version, 1 (ignore on read)
		/// </summary>
		short MajorVersion;

		/// <summary>
		/// Minor version, 1 (ignore on read)
		/// </summary>
		short MinorVersion;

		/// <summary>
		/// Reserved, always 0 (see Section 23.1).
		/// </summary>
		int Reserved;

		/// <summary>
		/// Length of version string in bytes, say m (&lt;= 255), rounded
		/// up to a multiple of four.
		/// </summary>
		int Length;

		MetadataRoot()
		{
			InitializeInstanceFields();
			_title = "Metadata root";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			Signature = readInt32(0);
			MajorVersion = readInt16(4);
			MinorVersion = readInt16(6);
			Reserved = readInt32(8);
			Length = readInt32(12);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			Signature = 0;
			MajorVersion = 0;
			MinorVersion = 0;
			Reserved = 0;
			Length = 0;
		}
	};

	/// <summary>
	/// The "#~" streams contain the actual physical representations
	/// of the logical metadata tables (see Chapter 21). A "#~"
	/// stream has the following top-level structure:
	/// </summary>
	class SharpTildeStream : public HeaderBase
	{
	public:
		/// <summary>
		/// Reserved, always 0 (see Section 23.1).
		/// </summary>
		int Reserved1;

		/// <summary>
		/// Major version of table schemata, always 1 (see Section
		/// 23.1).
		/// </summary>
		unsigned char MajorVersion;

		/// <summary>
		/// Minor version of table schemata, always 0 (see Section
		/// 23.1).
		/// </summary>
		unsigned char MinorVersion;

		/// <summary>
		/// Bit vector for heap sizes.
		/// </summary>
		unsigned char HeapSizes;

		/// <summary>
		/// Reserved, always 1 (see Section 23.1).
		/// </summary>
		unsigned char Reserved2;

		/// <summary>
		/// Bit vector of present tables, let n be the number of bits
		/// that are 1.
		/// </summary>
		long long Valid;

		/// <summary>
		/// Bit vector of sorted tables.
		/// </summary>
		long long Sorted;

		SharpTildeStream()
		{
			InitializeInstanceFields();
			_title = "Sharp Tilde (#~) Stream";
		}

		virtual void readData(std::vector<unsigned char>& data, int offset)
		{
			HeaderBase::readData(data, offset);

			Reserved1 = readInt32(0);
			MajorVersion = readByte(4);
			MinorVersion = readByte(5);
			HeapSizes = readByte(6);
			Reserved2 = readByte(7);
			Valid = readInt64(8);
			Sorted = readInt64(16);
		}

		virtual void getInfos(StringBuilder* sb)
		{
		}

	private:
		void InitializeInstanceFields()
		{
			Reserved1 = 0;
			MajorVersion = 0;
			MinorVersion = 0;
			HeapSizes = 0;
			Reserved2 = 0;
			Valid = 0;
			Sorted = 0;
		}
	};
}


#endif // __DOTNET_PEHEADERS_H__