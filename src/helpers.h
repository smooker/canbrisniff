#ifndef HELPERS_H
#define HELPERS_H

const char hex_asc_upper[] = "0123456789ABCDEF";

#define hex_asc_upper_lo(x) hex_asc_upper[((x) & 0x0F)]
#define hex_asc_upper_hi(x) hex_asc_upper[((x) & 0xF0) >> 4]

#define put_sff_id(buf, id) _put_id(buf, 2, id)
#define put_eff_id(buf, id) _put_id(buf, 7, id)

static inline void put_hex_byte(char *buf, __u8 byte)
{
    buf[0] = hex_asc_upper_hi(byte);
    buf[1] = hex_asc_upper_lo(byte);
}

static inline void _put_id(char *buf, int end_offset, canid_t id)
{
    /* build 3 (SFF) or 8 (EFF) digit CAN identifier */
    while (end_offset >= 0) {
        buf[end_offset--] = hex_asc_upper_lo(id);
        id >>= 4;
    }
}

void sprint_canframe(char *buf , struct canfd_frame *cf, int sep, int maxdlen)
{
    int i,offset;
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;

    if (cf->can_id & CAN_ERR_FLAG) {
        put_eff_id(buf, cf->can_id & (CAN_ERR_MASK|CAN_ERR_FLAG));
        buf[8] = '#';
        offset = 9;
    } else if (cf->can_id & CAN_EFF_FLAG) {
        put_eff_id(buf, cf->can_id & CAN_EFF_MASK);
        buf[8] = '#';
        offset = 9;
    } else {
        put_sff_id(buf, cf->can_id & CAN_SFF_MASK);
        buf[3] = '#';
        offset = 4;
    }

    /* standard CAN frames may have RTR enabled. There are no ERR frames with RTR */
    if (maxdlen == CAN_MAX_DLEN && cf->can_id & CAN_RTR_FLAG) {
        buf[offset++] = 'R';
        /* print a given CAN 2.0B DLC if it's not zero */
        if (cf->len && cf->len <= CAN_MAX_DLC)
            buf[offset++] = hex_asc_upper_lo(cf->len);

        buf[offset] = 0;
        return;
    }

    if (maxdlen == CANFD_MAX_DLEN) {
        /* add CAN FD specific escape char and flags */
        buf[offset++] = '#';
        buf[offset++] = hex_asc_upper_lo(cf->flags);
        if (sep && len)
            buf[offset++] = '.';
    }

    for (i = 0; i < len; i++) {
        put_hex_byte(buf + offset, cf->data[i]);
        offset += 2;
        if (sep && (i+1 < len))
            buf[offset++] = '.';
    }

    buf[offset] = 0;
}

#endif // HELPERS_H
