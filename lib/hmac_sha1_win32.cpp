//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include "hmac_sha1.hpp"

#include <cstring>
#include <vector>

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#define NOMINMAX
#include <windows.h>
#undef ERROR
// Note: wincrypt.h must be included after windows.h.
#include <wincrypt.h>

namespace us3 {

result_t<hmac_sha1_t> hmac_sha1(const char* key, const char* data) {
  unsigned char raw_digest[hmac_sha1_t::HMAC_SHA1_RAW_SIZE];
  status_t::status_enum_t return_status = status_t::ERROR;

  // Windows implementation inspired by:
  // https://docs.microsoft.com/en-us/windows/desktop/seccrypto/example-c-program--creating-an-hmac
  HCRYPTPROV crypt_prov = 0;
  HCRYPTKEY crypt_key = 0;
  HCRYPTHASH crypt_hash = 0;

  // Acquire a handle to the default RSA cryptographic service provider.
  if (CryptAcquireContext(&crypt_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
    // Import the key as a plain text key blob. This is dirty.
    // See: https://stackoverflow.com/a/32365048
    struct plain_text_key_blob_t {
      BLOBHEADER hdr;
      DWORD key_length;
    };

    const size_t key_size = std::strlen(key);
    std::vector<BYTE> key_blob(sizeof(plain_text_key_blob_t) + key_size);
    plain_text_key_blob_t* kb = reinterpret_cast<plain_text_key_blob_t*>(key_blob.data());
    std::memset(kb, 0, sizeof(plain_text_key_blob_t));
    kb->hdr.aiKeyAlg = CALG_RC2;
    kb->hdr.bType = PLAINTEXTKEYBLOB;
    kb->hdr.bVersion = CUR_BLOB_VERSION;
    kb->hdr.reserved = 0;
    kb->key_length = static_cast<DWORD>(key_size);
    std::memcpy(&key_blob[sizeof(plain_text_key_blob_t)], key, key_size);
    if (CryptImportKey(crypt_prov,
                       key_blob.data(),
                       static_cast<DWORD>(key_blob.size()),
                       0,
                       CRYPT_IPSEC_HMAC_KEY,
                       &crypt_key)) {
      if (CryptCreateHash(crypt_prov, CALG_HMAC, crypt_key, 0, &crypt_hash)) {
        HMAC_INFO hmac_info;
        ZeroMemory(&hmac_info, sizeof(hmac_info));
        hmac_info.HashAlgid = CALG_SHA1;
        if (CryptSetHashParam(
                crypt_hash, HP_HMAC_INFO, reinterpret_cast<const BYTE*>(&hmac_info), 0)) {
          if (CryptHashData(crypt_hash,
                            reinterpret_cast<const BYTE*>(data),
                            static_cast<DWORD>(std::strlen(data)),
                            0)) {
            DWORD hash_len = 0;
            if (CryptGetHashParam(crypt_hash, HP_HASHVAL, 0, &hash_len, 0)) {
              if (hash_len == hmac_sha1_t::HMAC_SHA1_RAW_SIZE) {
                if (CryptGetHashParam(crypt_hash,
                                      HP_HASHVAL,
                                      reinterpret_cast<BYTE*>(&raw_digest[0]),
                                      &hash_len,
                                      0)) {
                  return_status = status_t::SUCCESS;
                }
              }
            }
          }
        }
      }
    }
  }

  // Release resources.
  if (crypt_hash) {
    CryptDestroyHash(crypt_hash);
  }
  if (crypt_key) {
    CryptDestroyKey(crypt_key);
  }
  CryptReleaseContext(crypt_prov, 0);

  return make_result(hmac_sha1_t(raw_digest), return_status);
}

}  // namespace us3
