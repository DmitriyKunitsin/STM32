#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <sstream>
#include "aes.hpp"

struct Pair {
	std::vector<uint8_t> key;
	std::vector<uint8_t> iv;
};

int ChToHex(char ch) {
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	}
	if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	}
	if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	}
	throw std::invalid_argument("ChToHex: unexpected char");
}

Pair ParseKeyCStream(std::istream &is) {
	// SORRY GUYS
	Pair pair;
	std::array<std::vector<uint8_t> *, 2> arr = { &pair.key, &pair.iv };
	char ch;
	int modelen = 0;
	enum { waitopen, numprefix, numcore } mode = waitopen;
	uint8_t byteval = 0;
	for (int i = 0; i < arr.size(); ++i) {
		is >> std::skipws;
		while (true) {
			is >> ch;
			if (!is) {
				break;
			}
			switch (mode) {
			case waitopen: {
				if (ch == '{') {
					mode = numprefix;
					modelen = -1;
				}
				break;
			}
			case numprefix: {
				if (modelen == 0 && ch == '0') {
				}
				else if (modelen == 1 && ch == 'x') {
					mode = numcore;
					modelen = -1;
				}
				else {
					throw std::invalid_argument("unexpected char");
				}
				break;
			}
			case numcore: {
				if (modelen == 0 || modelen == 1) {
					byteval <<= 4;
					byteval |= ChToHex(ch);
				}
				else if (modelen == 2) {
					arr[i]->push_back(byteval);
					if (ch == ',') {
						mode = numprefix;
						modelen = -1;
					}
					else if (ch == '}') {
						mode = waitopen;
						modelen = -1;
					}
					else {
						throw std::invalid_argument("unexpected char");
					}
				}
				else {
					throw std::invalid_argument("unexpected char");
				}

				break;
			}
			default: throw std::invalid_argument("invalid state");
			}
			modelen++;
			if (mode == waitopen && modelen == 0) {
				break;
			}
		}		
	}

	if (mode != waitopen) {
		throw std::invalid_argument("unexpected EOF");
	}
	if (pair.key.size() != AES_BLOCKLEN || pair.iv.size() != AES_BLOCKLEN) {
		throw std::invalid_argument("invalid key or IV size");
	}
	return pair;
}

// если из входного стрима вытащили все байты, добивает до размера блока любыми значениями
off_t StreamEncrypt(AES_ctx *aesCtx, std::istream &is, std::ostream &os) {
	assert(aesCtx != nullptr);
	std::array<char, AES_BLOCKLEN> buf;
	off_t nBytesWritten = 0;
	while (is) {
		is.read(buf.data(), buf.size());
		int gcount = is.gcount();
		if (gcount > 0) {
			// могли прочитать и меньше, да и пофиг на мусор в конце массива
			AES_CBC_encrypt_buffer(aesCtx, reinterpret_cast<uint8_t*>(buf.data()), buf.size());
			os.write(buf.data(), buf.size());
			nBytesWritten += buf.size();
		}
	}
	return nBytesWritten;
}

// а эта функция кидает исключение, если ей не хватает байт.
off_t StreamDecrypt(AES_ctx *aesCtx, std::istream &is, std::ostream &os, off_t maxObytes = -1) {
	assert(aesCtx != nullptr);
	std::array<char, AES_BLOCKLEN> buf;
	off_t nDecryptedBytes = 0;
	while (is && (maxObytes == -1 || maxObytes > 0)) {
		is.read(buf.data(), buf.size());
		if (is.gcount() > 0) {
			if (is.gcount() != buf.size()) {
				throw std::invalid_argument("StreamDecrypt: truncated last block");
			}
			AES_CBC_decrypt_buffer(aesCtx, reinterpret_cast<uint8_t*>(buf.data()), buf.size());

			nDecryptedBytes += buf.size();
			const off_t nbytes = maxObytes == -1
				? buf.size()
				: std::min(maxObytes, static_cast<off_t>(buf.size()));
			os.write(buf.data(), nbytes);
			assert(maxObytes == -1 || nbytes <= maxObytes);
			maxObytes -= nbytes;
		}
	}
	return nDecryptedBytes;
}

template <typename Iter>
void off_t_to_LE(off_t offt, Iter lebegin, Iter leend) {
	std::fill(lebegin, leend, 0);
	while (offt) {
		assert(lebegin != leend);
		*lebegin = static_cast<uint8_t>(offt & 0xff);
		++lebegin;
		offt >>= 8;
	}
}

template <typename Iter>
off_t LE_to_off_t(Iter lebegin, Iter leend) {
	off_t res = 0;
	auto dist = std::distance(lebegin, leend);
	int n = 0;
	while (n < sizeof(res) && lebegin != leend) {
		res |= static_cast<uint8_t>(*lebegin & 0xff) << (n * 8);
		++lebegin;
		++n;
	}
	return res;
}

enum { kFileSizeBufLen = 16 };
static_assert(kFileSizeBufLen % AES_BLOCKLEN == 0, "");

off_t FileEncrypt(const Pair &aesKeyIv, off_t fileSize, std::istream &plainIn, std::ostream &cipherOut) {
	AES_ctx aesCtx;
	std::array<uint8_t, kFileSizeBufLen> buf;
	std::stringstream fileSizeSs(std::ios::in | std::ios::out | std::ios::binary);
	off_t nBytesWritten = 0;

	AES_init_ctx_iv(&aesCtx, aesKeyIv.key.data(), aesKeyIv.iv.data());
	off_t_to_LE(fileSize, buf.begin(), buf.end());
	fileSizeSs.write(reinterpret_cast<const char *>(buf.data()), buf.size());
	nBytesWritten += StreamEncrypt(&aesCtx, fileSizeSs, cipherOut);
	nBytesWritten += StreamEncrypt(&aesCtx, plainIn, cipherOut);
	return nBytesWritten;
}

off_t FileDecrypt(const Pair &aesKeyIv, std::istream &cipherIn, std::ostream &plainOut, bool canReadTruncFile = false) {
	AES_ctx aesCtx;
	std::array<char, kFileSizeBufLen> buf;
	std::stringstream fileSizeSs(std::ios::in | std::ios::out | std::ios::binary);
	off_t fileSize;
	off_t nDecryptedBytes = 0;

	AES_init_ctx_iv(&aesCtx, aesKeyIv.key.data(), aesKeyIv.iv.data());
	nDecryptedBytes = StreamDecrypt(&aesCtx, cipherIn, fileSizeSs, buf.size());
	if (nDecryptedBytes != buf.size()) {
		throw std::runtime_error("FileDecrypt: truncated stream. can't read file size.");
	}
	fileSizeSs.read(buf.data(), buf.size());
	fileSize = LE_to_off_t(buf.cbegin(), buf.cend());
	nDecryptedBytes = StreamDecrypt(&aesCtx, cipherIn, plainOut, fileSize);
	if (!canReadTruncFile && nDecryptedBytes < fileSize) {
		throw std::runtime_error("FileDecrypt: truncated file.");
	}
	return fileSize;
}

void MakeFw(const std::string &mode, const std::string &aesPropsPath, const std::string &inPath, const std::string &outPath) {
	std::ifstream aesPropsIfs(aesPropsPath);
	if (!aesPropsIfs) {
		throw std::runtime_error("Can't open file " + aesPropsPath);
	}
	std::ifstream inStream(inPath, std::ios::in | std::ios::binary);
	if (!inStream) {
		throw std::runtime_error("Can't open file " + inPath);
	}
	std::ofstream outStream(outPath, std::ios::out | std::ios::binary);
	if (!outStream) {
		throw std::runtime_error("Can't open file " + outPath);
	}
	const Pair p = ParseKeyCStream(aesPropsIfs);
	if (mode == "encrypt") {
		const off_t nb = FileEncrypt(p, std::filesystem::file_size(inPath), inStream, outStream);
		std::cerr << "Done. Bytes written: " << nb << std::endl;
	}
	else if (mode == "decrypt") {
		const off_t nb = FileDecrypt(p, inStream, outStream, false);
		std::cerr << "Done. Bytes decrypted: " << nb << std::endl;
	}
	else {
		throw std::invalid_argument("unsupported mode: " + mode);
	}
}

int main(int argc, char **argv) {
	if (argc != 5) {
		std::cerr
			<< "Usage: " << argv[0] << " <encrypt|decrypt> <AES_Key_IV.c> <in> <out>" << std::endl
			<< "Reads <in> file and <encrypt|decrypt>s it into <out> using keys generates by aes_keygen util." << std::endl
			<< "Prints logs into stderr." << std::endl;
		return EXIT_FAILURE;
	}

	try {
		MakeFw(argv[1], argv[2], argv[3], argv[4]);
	}
	catch (const std::exception &ex) {
		std::cerr << "An error has occured: " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}