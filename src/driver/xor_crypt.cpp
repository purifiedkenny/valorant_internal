#include "../../include/driver/xor_crypt.hpp"
#include "../../include/utils/logger.hpp"
#include <fstream>

namespace maxi::driver::crypt {

    std::vector<uint8_t> load_and_decrypt_file(const std::wstring& path) {

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            LOG_ERROR("XOR Crypt: Failed to open encrypted file");
            return {};
        }


        auto file_size = file.tellg();
        if (file_size <= 0) {
            LOG_ERROR("XOR Crypt: File is empty or unreadable");
            return {};
        }

        file.seekg(0, std::ios::beg);


        std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
        if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
            LOG_ERROR("XOR Crypt: Failed to read file contents");
            return {};
        }
        file.close();


        full_decrypt(buffer.data(), buffer.size());

        LOG_INFO("XOR Crypt: Decrypted " + std::to_string(buffer.size()) + " bytes from file");
        return buffer;
    }

    std::vector<uint8_t> load_and_decrypt_resource(HMODULE hModule, int resourceId) {

        HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
        if (!hRes) {
            LOG_ERROR("XOR Crypt: Failed to find embedded resource " + std::to_string(resourceId));
            return {};
        }


        HGLOBAL hData = LoadResource(hModule, hRes);
        if (!hData) {
            LOG_ERROR("XOR Crypt: Failed to load resource");
            return {};
        }


        void* pData = LockResource(hData);
        DWORD dataSize = SizeofResource(hModule, hRes);
        if (!pData || dataSize == 0) {
            LOG_ERROR("XOR Crypt: Resource is empty or lock failed");
            return {};
        }


        std::vector<uint8_t> buffer(dataSize);
        memcpy(buffer.data(), pData, dataSize);


        if (buffer.size() >= 2 && buffer[0] == 'M' && buffer[1] == 'Z') {
            LOG_INFO("Resource loaded as raw PE: " + std::to_string(buffer.size()) + " bytes");
            return buffer;
        }


        full_decrypt(buffer.data(), buffer.size());

        LOG_INFO("XOR Crypt: Decrypted " + std::to_string(buffer.size()) + " bytes from resource");
        return buffer;
    }

}
