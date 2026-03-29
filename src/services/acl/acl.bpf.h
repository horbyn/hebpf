#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct ipv4_tuple {
  uint32_t saddr;
  uint32_t daddr;
  uint16_t sport;
  uint16_t dport;
  uint8_t protocol;
  uint8_t padding;
} __attribute__((packed));

#define ACL_ACTION_ALLOW 0
#define ACL_ACTION_DENY 1

#ifdef __cplusplus
}
#endif
