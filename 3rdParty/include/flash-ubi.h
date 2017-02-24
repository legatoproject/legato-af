/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FLASH_UBI_H_
#define _FLASH_UBI_H_

#include <sys/types.h>
#include <stdint.h>

struct __attribute__ ((packed)) ubifs_ch {
	uint32_t magic;
	uint32_t crc;
	uint64_t sqnum;
	uint32_t len;
#define UBIFS_SB_NODE	6
	uint8_t node_type;
	uint8_t group_type;
	uint8_t padding[2];
};

/* UBIFS superblock node */
struct __attribute__ ((packed)) ubifs_sb_node {
	struct ubifs_ch ch;
	uint8_t padding[2];
	uint8_t key_hash;
	uint8_t key_fmt;
#define UBIFS_FLG_SPACE_FIXUP  0x04
	uint32_t flags;
	uint32_t min_io_size;
	uint32_t leb_size;
	uint32_t leb_cnt;
	uint32_t max_leb_cnt;
	uint64_t max_bud_bytes;
	uint32_t log_lebs;
	uint32_t lpt_lebs;
	uint32_t orph_lebs;
	uint32_t jhead_cnt;
	uint32_t fanout;
	uint32_t lsave_cnt;
	uint32_t fmt_version;
	uint16_t default_compr;
	uint8_t padding1[2];
	uint32_t rp_uid;
	uint32_t rp_gid;
	uint64_t rp_size;
	uint32_t time_gran;
	uint8_t uuid[16];
	uint32_t ro_compat_version;
	uint8_t padding2[3968];
};

/* Erase counter header magic number (ASCII "UBI#") */
#define UBI_EC_HDR_MAGIC  0x55424923

#define UBI_MAGIC      "UBI#"
#define UBI_MAGIC_SIZE 0x04

#define UBI_VERSION 1
#define UBI_MAX_ERASECOUNTER 0x7FFFFFFF
#define UBI_IMAGE_SEQ_BASE 0x12345678
#define UBI_DEF_ERACE_COUNTER 0
#define UBI_CRC32_INIT 0xFFFFFFFFU
#define UBIFS_CRC32_INIT 0xFFFFFFFFU

/* SWISTART */
#ifdef SIERRA
#define UBIFS_MAGIC 0x31181006
#endif /* SIERRA */
/* SWISTOP */

/* Erase counter header fields */
struct __attribute__ ((packed)) ubi_ec_hdr {
	uint32_t  magic;
	uint8_t   version;
	uint8_t   padding1[3];
	uint64_t  ec; /* Warning: the current limit is 31-bit anyway! */
	uint32_t  vid_hdr_offset;
	uint32_t  data_offset;
	uint32_t  image_seq;
	uint8_t   padding2[32];
	uint32_t  hdr_crc;
};

/* SWISTART */
#define UBI_VID_HDR_MAGIC   0x55424921
 
#define UBI_VID_DYNAMIC 1
#define UBI_VID_STATIC 2
/* SWISTOP */

/* Volume identifier header fields */
struct __attribute__ ((packed)) ubi_vid_hdr {
	uint32_t  magic;
	uint8_t    version;
	uint8_t    vol_type;
	uint8_t    copy_flag;
	uint8_t    compat;
	uint32_t  vol_id;
	uint32_t  lnum;
	uint8_t    padding1[4];
	uint32_t  data_size;
	uint32_t  used_ebs;
	uint32_t  data_pad;
	uint32_t  data_crc;
	uint8_t    padding2[4];
	uint64_t  sqnum;
	uint8_t    padding3[12];
	uint32_t  hdr_crc;
};

#define UBI_EC_HDR_SIZE  sizeof(struct ubi_ec_hdr)
#define UBI_VID_HDR_SIZE  sizeof(struct ubi_vid_hdr)
#define UBI_EC_HDR_SIZE_CRC  (UBI_EC_HDR_SIZE  - sizeof(uint32_t))
#define UBI_VID_HDR_SIZE_CRC (UBI_VID_HDR_SIZE - sizeof(uint32_t))

#define UBI_MAX_VOLUMES 128
#define UBI_INTERNAL_VOL_START (0x7FFFFFFF - 4096)
#define UBI_LAYOUT_VOLUME_ID     UBI_INTERNAL_VOL_START
#define UBI_FM_SB_VOLUME_ID	(UBI_INTERNAL_VOL_START + 1)

/**
 * struct ubi_scan_info - UBI scanning information.
 * @ec: erase counters or eraseblock status for all eraseblocks
 * @mean_ec: mean erase counter
 * @bad_cnt: count of bad eraseblocks
 * @good_cnt: count of non-bad eraseblocks
 * @empty_cnt: count of empty eraseblocks
 * @vid_hdr_offs: volume ID header offset from the found EC headers (%-1 means
 *                undefined)
 * @data_offs: data offset from the found EC headers (%-1 means undefined)
 * @image_seq: image sequence
 */
struct ubi_scan_info {
	uint64_t *ec;
	uint64_t mean_ec;
	int bad_cnt;
	int good_cnt;
	int empty_cnt;
	unsigned vid_hdr_offs;
	unsigned data_offs;
	uint32_t  image_seq;
};

/**
 * struct ubi_vtbl_record - a record in the volume table.
 * @reserved_pebs: how many physical eraseblocks are reserved for this volume
 * @alignment: volume alignment
 * @data_pad: how many bytes are unused at the end of the each physical
 * eraseblock to satisfy the requested alignment
 * @vol_type: volume type (%UBI_DYNAMIC_VOLUME or %UBI_STATIC_VOLUME)
 * @upd_marker: if volume update was started but not finished
 * @name_len: volume name length
 * @name: the volume name
 * @flags: volume flags (%UBI_VTBL_AUTORESIZE_FLG)
 * @padding: reserved, zeroes
 * @crc: a CRC32 checksum of the record
 *
 * The volume table records are stored in the volume table, which is stored in
 * the layout volume. The layout volume consists of 2 logical eraseblock, each
 * of which contains a copy of the volume table (i.e., the volume table is
 * duplicated). The volume table is an array of &struct ubi_vtbl_record
 * objects indexed by the volume ID.
 *
 * If the size of the logical eraseblock is large enough to fit
 * %UBI_MAX_VOLUMES records, the volume table contains %UBI_MAX_VOLUMES
 * records. Otherwise, it contains as many records as it can fit (i.e., size of
 * logical eraseblock divided by sizeof(struct ubi_vtbl_record)).
 *
 * The @upd_marker flag is used to implement volume update. It is set to %1
 * before update and set to %0 after the update. So if the update operation was
 * interrupted, UBI knows that the volume is corrupted.
 *
 * The @alignment field is specified when the volume is created and cannot be
 * later changed. It may be useful, for example, when a block-oriented file
 * system works on top of UBI. The @data_pad field is calculated using the
 * logical eraseblock size and @alignment. The alignment must be multiple to the
 * minimal flash I/O unit. If @alignment is 1, all the available space of
 * the physical eraseblocks is used.
 *
 * Empty records contain all zeroes and the CRC checksum of those zeroes.
 */
struct __attribute__ ((packed)) ubi_vtbl_record {
	uint32_t  reserved_pebs;
	uint32_t  alignment;
	uint32_t  data_pad;
	uint8_t    vol_type;
	uint8_t    upd_marker;
	uint16_t  name_len;
	uint8_t    name[UBI_MAX_VOLUMES];
	uint8_t    flags;
	uint8_t    padding[23];
	uint32_t  crc;
};
#define UBI_VTBL_RECORD_HDR_SIZE  sizeof(struct ubi_vtbl_record)

/* Size of the volume table record without the ending CRC */
#define UBI_VTBL_RECORD_SIZE_CRC (UBI_VTBL_RECORD_HDR_SIZE - sizeof(uint32_t))

#endif
