//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++ (supporting code)
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013 - 2024 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

/*
This file implements a driver for American Fuzzy Lop (afl-fuzz). It relies on
an implementation of the `LLVMFuzzerTestOneInput` function which processes a
passed byte array.
*/

#include <vector>    // for vector
#include <cstdint>   // for uint8_t
#include <iostream>  // for cin


extern "C" int LLVMFuzzerTestOneInput_json(const uint8_t* data, size_t size);
extern "C" int LLVMFuzzerTestOneInput_ubjson(const uint8_t* data, size_t size);
extern "C" int LLVMFuzzerTestOneInput_msgpack(const uint8_t* data, size_t size);
extern "C" int LLVMFuzzerTestOneInput_bjdata(const uint8_t* data, size_t size);
extern "C" int LLVMFuzzerTestOneInput_bson(const uint8_t* data, size_t size);
extern "C" int LLVMFuzzerTestOneInput_cbor(const uint8_t* data, size_t size);



int main(int argc, char** argv)
{
#ifdef __AFL_HAVE_MANUAL_CONTROL
    while (__AFL_LOOP(1000))
    {
#endif
        // copy stdin to byte vector
        std::vector<uint8_t> vec;
        const char *filePath = argv[1];
        FILE* F = fopen(filePath, "r");
        if (F != nullptr) {
            // Read all bytes from the file
            int byte;
            while ((byte = fgetc(F)) != EOF) {
                vec.push_back(static_cast<uint8_t>(byte));
            }
            fclose(F);
        }
        else
        {
            return 0;
        }

        LLVMFuzzerTestOneInput_json (vec.data(), vec.size());
        LLVMFuzzerTestOneInput_ubjson(vec.data(), vec.size());
        LLVMFuzzerTestOneInput_msgpack(vec.data(), vec.size());
        LLVMFuzzerTestOneInput_bjdata(vec.data(), vec.size());
        LLVMFuzzerTestOneInput_bson(vec.data(), vec.size());
        LLVMFuzzerTestOneInput_cbor(vec.data(), vec.size());
#ifdef __AFL_HAVE_MANUAL_CONTROL
    }
#endif
}
