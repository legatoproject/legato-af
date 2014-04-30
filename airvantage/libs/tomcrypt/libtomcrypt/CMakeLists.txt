#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This building script is made available under the terms
# of the Eclipse Public License v1.0 which accompanies this distribution, and
# is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

PROJECT(LIB_TOMCRYPT)

INCLUDE_DIRECTORIES(${LIB_TOMCRYPT_SOURCE_DIR}/headers
    ${LIB_TOMCRYPT_SOURCE_DIR}/ciphers/aes
    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/sha2
    ${LIB_TOMMATH_SOURCE_DIR}
    )

SET(LIB_TOMCRYPT_SRC
    ${LIB_TOMCRYPT_SOURCE_DIR}/ciphers/aes/aes.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/helper/hash_file.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/helper/hash_filehandle.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/helper/hash_memory_multi.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/helper/hash_memory.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/sha2/sha256.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/sha2/sha224.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/hashes/sha2/sha512.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/hashes/sha2/sha384.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/md5.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/hashes/sha1.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_argchk.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_cipher_descriptor.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_cipher_is_valid.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_cipher_any.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_cipher_id.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_cipher.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_hash_any.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_hash_id.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_hash_oid.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_hash.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_find_prng.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_fsa.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_hash_descriptor.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_hash_is_valid.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_ltc_mp_descriptor.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_prng_descriptor.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_prng_is_valid.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_register_cipher.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_register_hash.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_register_prng.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_unregister_cipher.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_unregister_hash.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt_unregister_prng.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/crypt/crypt.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/burn_stack.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/error_to_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/misc/zeromem.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/mac/hmac/hmac_done.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/mac/hmac/hmac_file.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/mac/hmac/hmac_init.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/mac/hmac/hmac_memory_multi.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/mac/hmac/hmac_memory.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/mac/hmac/hmac_process.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/mac/hmac/hmac_test.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ecb/ecb_decrypt.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ecb/ecb_done.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ecb/ecb_encrypt.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ecb/ecb_start.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/cbc/cbc_decrypt.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/cbc/cbc_done.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/cbc/cbc_encrypt.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/modes/cbc/cbc_getiv.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/modes/cbc/cbc_setiv.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/cbc/cbc_start.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ctr/ctr_decrypt.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ctr/ctr_done.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ctr/ctr_encrypt.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/modes/ctr/ctr_getiv.c
    #${LIB_TOMCRYPT_SOURCE_DIR}/modes/ctr/ctr_setiv.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/modes/ctr/ctr_start.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/prngs/fortuna.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/prngs/rng_get_bytes.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/prngs/rng_make_prng.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_ansi_x963_export.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_ansi_x963_import.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_decrypt_key.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_encrypt_key.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_export.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_free.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_get_size.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_import.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_make_key.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_shared_secret.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_sign_hash.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_sizes.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_test.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ecc_verify_hash.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_is_valid_idx.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_map.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_mul2add.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_mulmod.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_mulmod_timing.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_points.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_projective_add_point.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/ecc/ltc_ecc_projective_dbl_point.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/math/ltm_desc.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/math/multi.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/sequence/der_decode_sequence_multi.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/sequence/der_decode_sequence_ex.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/octet/der_length_octet_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/utf8/der_length_utf8_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/utf8/der_decode_utf8_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/utf8/der_encode_utf8_string.c

    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/printable_string/der_decode_printable_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/printable_string/der_encode_printable_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/printable_string/der_length_printable_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/bit/der_decode_bit_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/bit/der_encode_bit_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/bit/der_length_bit_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/boolean/der_decode_boolean.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/boolean/der_encode_boolean.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/boolean/der_length_boolean.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/integer/der_decode_integer.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/integer/der_encode_integer.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/integer/der_length_integer.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/choice/der_decode_choice.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/object_identifier/der_length_object_identifier.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/ia5/der_decode_ia5_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/ia5/der_encode_ia5_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/ia5/der_length_ia5_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/set/der_encode_setof.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/set/der_encode_set.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/utctime/der_length_utctime.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/utctime/der_decode_utctime.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/utctime/der_encode_utctime.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/octet/der_encode_octet_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/octet/der_decode_octet_string.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/object_identifier/der_encode_object_identifier.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/object_identifier/der_decode_object_identifier.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/sequence/der_length_sequence.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/sequence/der_encode_sequence_multi.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/sequence/der_decode_sequence_flexi.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/sequence/der_sequence_free.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/sequence/der_encode_sequence_ex.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/short_integer/der_encode_short_integer.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/short_integer/der_length_short_integer.c
    ${LIB_TOMCRYPT_SOURCE_DIR}/pk/asn1/der/short_integer/der_decode_short_integer.c
    )

ADD_LIBRARY(lib_tomcrypt EXCLUDE_FROM_ALL SHARED ${LIB_TOMCRYPT_SRC})
SET_TARGET_PROPERTIES(lib_tomcrypt PROPERTIES
    OUTPUT_NAME tomcrypt
    COMPILE_FLAGS "-fPIC") #necessary so that the lib can be used linked in a static code or a shared library

TARGET_LINK_LIBRARIES(lib_tomcrypt lib_tommath)
