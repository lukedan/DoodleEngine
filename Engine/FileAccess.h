#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <cstdio>
#include <typeinfo>
#include <io.h>
#include <sys/stat.h>
// NOTE winapi usage
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

#include "String.h"
#include "Property.h"

namespace DE {
	namespace IO {
		// NOTE win32 API usage
		enum class FileAttribute {
			ReadOnly = FILE_ATTRIBUTE_READONLY,
			Hidden = FILE_ATTRIBUTE_HIDDEN,
			System = FILE_ATTRIBUTE_SYSTEM,
			Directory = FILE_ATTRIBUTE_DIRECTORY,
			Archive = FILE_ATTRIBUTE_ARCHIVE,
			Device = FILE_ATTRIBUTE_DEVICE,
			Normal = FILE_ATTRIBUTE_NORMAL,
			Temporary = FILE_ATTRIBUTE_TEMPORARY,
			Sparse = FILE_ATTRIBUTE_SPARSE_FILE,
			ReparsePoint = FILE_ATTRIBUTE_REPARSE_POINT,
			Compressed = FILE_ATTRIBUTE_COMPRESSED,
			Offline = FILE_ATTRIBUTE_OFFLINE,
			ContentNotIndexed = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
			Encrypted = FILE_ATTRIBUTE_ENCRYPTED,
			Virtual = FILE_ATTRIBUTE_VIRTUAL,
			Invalid = 0
		};
		struct FileInfo {
			public:
				FileInfo() = default;
				explicit FileInfo(const Core::String &fn) : FileInfo(fn, GetFileData(fn)) {
				}

				inline static void EnumerateDirectory(
					const Core::String &directory, const std::function<bool(const FileInfo&)> &callback
				) {
					Core::String dn = directory;
					if (dn.Length() > 0) {
						if (dn[dn.Length() - 1] != _TEXT('\\')) {
							dn += _TEXT('\\');
						}
					}
					WIN32_FIND_DATA data;
					HANDLE handle = FindFirstFile(*(dn + _TEXT('*')), &data);
					if (handle == INVALID_HANDLE_VALUE) {
						throw Core::SystemException(_TEXT("the directory cannot be enumerated"));
					}
					do {
						if (Core::String(_TEXT(".")) == data.cFileName || Core::String(_TEXT("..")) == data.cFileName) {
							continue;
						}
						if (!callback(FileInfo(data))) {
							FindClose(handle);
							return;
						}
					} while (FindNextFile(handle, &data));
					FindClose(handle);
					if (GetLastError() != ERROR_NO_MORE_FILES) {
						throw Core::SystemException(_TEXT("file enumeration terminated incorrectly"));
					}
				}

				bool HasAttribute(FileAttribute attr) {
					return static_cast<int>(*Type) & static_cast<int>(attr);
				}

				Core::ReferenceProperty<Core::String, Core::PropertyType::ReadOnly> Name;
				Core::ReferenceProperty<FileAttribute, Core::PropertyType::ReadOnly> Type = FileAttribute::Invalid;
				Core::ReferenceProperty<SYSTEMTIME, Core::PropertyType::ReadOnly> CreationTime, LastAccessTime, LastModifyTime;
				Core::ReferenceProperty<unsigned long long, Core::PropertyType::ReadOnly> Size;
			protected:
				FileInfo(const Core::String &fn, const WIN32_FILE_ATTRIBUTE_DATA &data) :
					Name(fn),
					Type(static_cast<FileAttribute>(data.dwFileAttributes)),
					CreationTime(TranslateFileTime(data.ftCreationTime)),
					LastAccessTime(TranslateFileTime(data.ftLastAccessTime)),
					LastModifyTime(TranslateFileTime(data.ftLastWriteTime)),
					Size(ComposeInt64(data.nFileSizeHigh, data.nFileSizeLow))
				{
				}
				explicit FileInfo(const WIN32_FIND_DATA &data) :
					Name(data.cFileName),
					Type(static_cast<FileAttribute>(data.dwFileAttributes)),
					CreationTime(TranslateFileTime(data.ftCreationTime)),
					LastAccessTime(TranslateFileTime(data.ftLastAccessTime)),
					LastModifyTime(TranslateFileTime(data.ftLastWriteTime)),
					Size(ComposeInt64(data.nFileSizeHigh, data.nFileSizeLow))
				{
				}

				inline static WIN32_FILE_ATTRIBUTE_DATA GetFileData(const Core::String &fn) {
					WIN32_FILE_ATTRIBUTE_DATA data;
					if (!GetFileAttributesEx(*fn, GetFileExInfoStandard, &data)) {
						throw Core::SystemException(_TEXT("cannot get the attributes of the file"));
					}
					return data;
				}
				inline static SYSTEMTIME TranslateFileTime(const FILETIME &ft) {
					SYSTEMTIME st;
					if (!FileTimeToSystemTime(&ft, &st)) {
						throw Core::SystemException(_TEXT("cannot convert file time"));
					}
					return st;
				}
				inline static unsigned long long ComposeInt64(unsigned long long high, unsigned long long low) {
					return (high << (sizeof(unsigned long long) << 2)) | low;
				}
		};

		enum class FileAccessType {
			/*
			 * Read(r)		= 0x000001 = 1
			 * NewFile(w)	= 0x000010 = 2
			 * Append(a)	= 0x000011 = 3
			 * +			= 0x000100 = 4
			 * Binary(b)	= 0x001000 = 8
			 * Exclusive(x)	= 0x010000 = 16
			 */
			None = 0,
			ReadText = 1,						// r	= 0x000001
			ReadBinary = 9,						// rb	= 0x001001
			ReadWriteText = 5,					// r+	= 0x000101
			ReadWriteBinary = 13,				// rb+	= 0x001101
			NewWriteText = 2,					// w	= 0x000010
			NewWriteBinary = 10,				// wb	= 0x001010
			NewReadWriteText = 6,				// w+	= 0x000110
			NewReadWriteBinary = 14,			// wb+	= 0x001110
			AppendText = 3,						// a	= 0x000011
			AppendBinary = 11,					// ab	= 0x001011
			AppendReadText = 7,					// a+	= 0x000111
			AppendReadBinary = 15,				// ab+	= 0x001111
			ExclusiveNewWriteText = 18,			// wx	= 0x010010
			ExclusiveNewWriteBinary = 26,		// wbx	= 0x011010
			ExclusiveNewReadWriteText = 22,		// w+x	= 0x010110
			ExclusiveNewReadWriteBinary = 30	// wb+x	= 0x011110
		};
		class FileAccess {
			public:
				FileAccess() = default;
				FileAccess(const Core::AsciiString &fileName, FileAccessType type) : _type(type) {
					OpenFileWithMode(fileName, type);
				}
				FileAccess(const FileAccess&) = delete;
				FileAccess &operator =(const FileAccess&) = delete;
				virtual ~FileAccess() {
					Close();
				}

				virtual void Open(const Core::AsciiString &fileName, FileAccessType type) {
					if (_file) {
						Close();
					}
					OpenFileWithMode(fileName, type);
				}
				virtual void Flush() {
					if (_file) {
						if (fflush(_file) != 0) {
							throw Core::SystemException(_TEXT("cannot flush the file"));
						}
					}
				}
				virtual void Close() {
					if (_file) {
						if (fclose(_file) != 0) {
							throw Core::SystemException(_TEXT("cannot open the file"));
						}
						_file = nullptr;
					}
				}

				virtual void ResetPosition() {
					if (_file) {
						rewind(_file);
					}
				}
				virtual fpos_t GetPosition() const {
					if (!_file) {
						throw Core::InvalidOperationException(_TEXT("file not opened"));
					}
					fpos_t fp;
					if (fgetpos(_file, &fp)) {
						throw Core::SystemException(_TEXT("cannot get file position"));
					}
					return fp;
				}
				virtual void SetPosition(fpos_t pos) {
					if (!_file) {
						throw Core::InvalidOperationException(_TEXT("file not opened"));
					}
					if (fsetpos(_file, &pos)) {
						throw Core::SystemException(_TEXT("cannot set file position"));
					}
				}
				fpos_t GetSize() const {
					fpos_t curPos;
					if (fgetpos(_file, &curPos)) {
						throw Core::SystemException(_TEXT("cannot get file size"));
					}
					if (fseek(_file, 0, SEEK_END)) {
						throw Core::SystemException(_TEXT("cannot get file size"));
					}
					fpos_t result;
					if (fgetpos(_file, &result)) {
						throw Core::SystemException(_TEXT("cannot get file size"));
					}
					if (fsetpos(_file, &curPos)) {
						throw Core::SystemException(_TEXT("cannot get file size"));
					}
					return result;
				}

				virtual bool Valid() const {
					return _file && !feof(_file);
				}
				virtual operator bool() const {
					return Valid();
				}
				virtual const FILE *operator *() const {
					return _file;
				}
				virtual FILE *operator *() {
					return _file;
				}

				static bool Exists(const Core::AsciiString &fileName) {
					WIN32_FIND_DATAA dummy; // NOTE don't know if this can be removed, not mentioned in the fucking document
					HANDLE handle = FindFirstFileA(*fileName, &dummy);
					if (handle == INVALID_HANDLE_VALUE) {
						return false;
					} else {
						FindClose(handle);
						return true;
					}
//					return (access(*fileName, F_OK) != -1);
				}
				static _off_t GetSize(const Core::AsciiString &fileName) { // TODO: check if this can use when the file is open
					if (Exists(fileName)) {
						struct _stat cres;
						if (_stat(*fileName, &cres)) {
							throw Core::SystemException(_TEXT("cannot get file size"));
						}
						return cres.st_size;
					}
					return 0;
				}

				virtual TCHAR ReadChar() {
					return _fgettc(_file);
				}
				virtual void WriteText(const Core::String &str) {
#ifdef UNICODE
					if (_ftprintf(_file, _TEXT("%ls"), *str) < 0) {
#else
					if (_ftprintf(_file, _TEXT("%s"), *str) < 0) {
#endif
//							if (_ftprintf(_file, _TEXT(
//#ifdef UNICODE
//					"%ls"
//#else
//					"%s"
//#endif
//					), *str) < 0) {
						throw Core::SystemException(_TEXT("cannot write the text"));
					}
				}
				// TODO: text mode functions

				template <typename T> T ReadBinaryObject() {
					T result;
					if (!ReadBinaryObjectByReference<T>(result)) {
                        throw Core::SystemException(_TEXT("cannot read"));
					}
					return result;
				}
				template <typename T> bool ReadBinaryObjectByReference(T &obj) {
					return ReadBinaryRaw(&obj, sizeof(T)) == sizeof(T);
				}
				template <typename Char> Core::StringBase<Char> ReadBinaryString() {
					size_t l = ReadBinaryObject<size_t>(), memSz = sizeof(Char) * (l + 1);
					Char *cs = (Char*)Core::GlobalAllocator::Allocate(memSz);
					ReadBinaryRaw(cs, memSz - sizeof(Char));
					cs[l] = 0;
					Core::StringBase<Char> result(cs);
					Core::GlobalAllocator::Free(cs);
					return result;
				}
				virtual size_t ReadBinaryRaw(void *ptr, size_t targetSize) {
					if (_file) {
						return fread(ptr, 1, targetSize, _file);
					} else {
						return 0;
					}
				}

				template <typename T> void WriteBinaryObject(const T &obj) {
					WriteBinaryRaw(&obj, sizeof(T));
				}
				template <typename Char> void WriteBinaryString(const Core::StringBase<Char> &str) {
					WriteBinaryObject<size_t>(str.Length());
					WriteBinaryRaw(*str, sizeof(Char) * str.Length());
				}
				virtual void WriteBinaryRaw(const void *ptr, size_t targetSize) {
					if (_file) {
						if (fwrite(ptr, 1, targetSize, _file) != targetSize) {
							throw Core::SystemException(_TEXT("cannot write data"));
						}
					}
				}

				virtual FILE *GetHandle() {
					return _file;
				}
				virtual const FILE *GetHandle() const {
					return _file;
				}

				FileAccessType GetAccessType() const {
					return _type;
				}
			protected:
				FileAccessType _type = FileAccessType::None;
				FILE *_file = nullptr;

				Core::AsciiString GetFileOpenIndicator(FileAccessType type) {
					Core::AsciiString result;
					switch ((int)type & 3) {
						case 1: {
							result = "r";
							break;
						}
						case 2: {
							result = "w";
							break;
						}
						case 3: {
							result = "a";
							break;
						}
					}
					if ((int)type & 8) {
						result += 'b';
					}
					if ((int)type & 4) {
						result += '+';
					}
					if ((int)type & 16) {
						result += 'x';
					}
					return result;
				}
				void OpenFileWithMode(const Core::AsciiString &fileName, FileAccessType type) {
					if (_file) {
						Close();
					}
					_file = fopen(*fileName, *GetFileOpenIndicator(type));
					if (_file == nullptr) {
						throw Core::SystemException(_TEXT("cannot open the file"));
					}
				}
		};
	}
}
