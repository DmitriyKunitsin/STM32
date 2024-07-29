#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cassert>
#include <chrono>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <fstream>

template <typename TimePointT>
std::string GetStrTime(TimePointT timepoint)
{
	auto in_time_t = std::chrono::system_clock::to_time_t(timepoint);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
	return ss.str();
}

void GenVecDecl(std::ostream &os, const char *name, int size) {
	os << "extern const unsigned char " << name << "[" << size << "];" << std::endl;
}

void GenVecDef(std::ostream &os, const char *name, int size) {
	os << "const unsigned char " << name << "[" << size << "] = {" << std::endl;

	std::ios init(NULL);
	init.copyfmt(os);
	const int nPerLine = 16;
	for (int i = 0; i < size; ++i) {
		if (0 == i % nPerLine) {
			os << "  ";
		}
		os << "0x" << std::setfill('0') << std::setw(2) << std::hex << (rand() % 256);
		if (i < size - 1) {
			os << ",";
		}
		if (0 == (i + 1) % nPerLine || i == size - 1) {
			os << std::endl;
		}
	}
	os.copyfmt(init);

	os << "};" << std::endl;
}

void GenAESKeys(std::ostream &c_os, std::ostream &h_os, int nBits) {
	assert(nBits > 0 && nBits % 8 == 0);
	const int nBytes = nBits / 8;
	const std::string dneHeader =
		"// DO NOT EDIT! ALL CHANGES WILL BE REWRITTEN! GENERATED AUTOMATICALLY AT "
		+ GetStrTime(std::chrono::system_clock::now());
	const std::string ifdef =
		"__AES_KEYGEN_AES_KEY_" + std::to_string(nBits);
	
	h_os
		<< dneHeader << std::endl
		<< "#ifndef " << ifdef << std::endl;
	GenVecDecl(h_os, "_aes_keygen_aes_key", nBytes);
	GenVecDecl(h_os, "_aes_keygen_aes_cbc_iv", nBytes);
	h_os 
		<< "#define " << ifdef << std::endl
		<< "#endif // " << ifdef << std::endl;

	c_os
		<< dneHeader << std::endl;
	GenVecDef(c_os, "_aes_keygen_aes_key", nBytes);
	GenVecDef(c_os, "_aes_keygen_aes_cbc_iv", nBytes);
}

void Keygen(int nBits, const std::string &path) {
	if (nBits != 128 && nBits != 192 && nBits != 256) {
		throw std::invalid_argument("Unsupported nBits. Must be 128|192|256");
	}
	std::ofstream c_os(path + ".c");
	std::ofstream h_os(path + ".h");
	GenAESKeys(c_os, h_os, nBits);
}

int main(int argc, char **argv)
{
	int status = EXIT_SUCCESS;
	do {
		if (argc != 3) {
			std::cerr
				<< "Usage: " << argv[0] << " <128|192|256> <path>" << std::endl
				<< "Generates AES key and CBC IV of given bitness into <path>.c and <path>.h pair of files" << std::endl;
			return EXIT_FAILURE;
		}
		srand(std::chrono::system_clock::now().time_since_epoch().count());

		try {
			Keygen(std::stoi(argv[1]), argv[2]);
		}
		catch (const std::exception &ex) {
			std::cerr << "An error has occured: " << ex.what() << std::endl;
			status = EXIT_FAILURE;
			break;
		}
	} while (0);

	return status;
}